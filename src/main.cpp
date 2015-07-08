/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/types.h>
#include <psp2/moduleinfo.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/audioout.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/memorymgr.h>

#include <vita2d.h>

#include "../lynx/system.h"
#include "../lynx/lynxdef.h"
#include "utils.h"
#include "font.h"
#include "audio.h"


PSP2_MODULE_INFO(0, 0, "handyvita")

const SceSize sceUserMainThreadStackSize = 8*1024*1024;

static uint8_t lynx_width = 160;
static uint8_t lynx_height = 102;

unsigned int *framebuffer;
vita2d_texture *framebufferTex;

int audioOutPort=0;
static unsigned char *snd_buffer8;

static int scale;
static int pos_x;
static int pos_y;

bool newFrame = false;
static bool initialized = false;
static bool endEmulation = false;



static CSystem *lynx = NULL;

struct map { unsigned psp2; unsigned lynx; };

static map btn_map_no_rot[] = {
  { PSP2_CTRL_CIRCLE, BUTTON_A },
  { PSP2_CTRL_CROSS, BUTTON_B },
  { PSP2_CTRL_RIGHT, BUTTON_RIGHT },
  { PSP2_CTRL_LEFT, BUTTON_LEFT },
  { PSP2_CTRL_UP, BUTTON_UP },
  { PSP2_CTRL_DOWN, BUTTON_DOWN },
  { PSP2_CTRL_LTRIGGER, BUTTON_OPT1 },
  { PSP2_CTRL_RTRIGGER, BUTTON_OPT2 },
  { PSP2_CTRL_START, BUTTON_PAUSE },
};

/*void lynx_sound_stream_update(unsigned short *buffer, int buf_lenght){
  uint16_t left;
  for(int i = 0; i < buf_lenght; i++){
    left=(snd_buffer8[i]<<8)-32768;
    *buffer = left;
    ++buffer;
    *buffer = left;
    ++buffer;
  }
  lynx->gAudioBufferPointer=0;
}*/

/*static int sound_thread(SceSize args, void *argp){

  printf("Audiothread running");
  CSystem *lynx_aux = *(CSystem**)argp;
  unsigned short soundBuffer[4096*8];
  while (1) {
    if(initialized&&lynx_aux->gAudioBufferPointer > 0)
    {

        lynx_sound_stream_update(soundBuffer,lynx_aux->gAudioBufferPointer);
        sceAudioOutOutput(audioOutPort,(const void*)soundBuffer);
    }
  }
}*/

void AudioCallback(void *buffer, unsigned int *length, void *userdata)
{
  PspMonoSample *OutBuf = (PspMonoSample*)buffer;
  int i;
  int len = *length >> 1;

  if(((int)lynx->gAudioBufferPointer >= len)
    && (lynx->gAudioBufferPointer != 0) && (!lynx->gSystemHalt) )
  {
    for (i = 0; i < len; i++)
    {
      short sample = (short)(((int)lynx->gAudioBuffer[i] << 8) - 32768);
      (OutBuf++)->Channel = sample;
      (OutBuf++)->Channel = sample;
    }
    lynx->gAudioBufferPointer = 0;
  }
  else
  {
    *length = 64;
    for (i = 0; i < (int)*length; i+=2)
    {
      (OutBuf++)->Channel = 0;
      (OutBuf++)->Channel = 0;
    }
  }
}

unsigned char* lynx_display_callback(ULONG objref)
{
    //printf("lynx_display_callback %d",initialized);
    if(!initialized)
    {
        framebufferTex = vita2d_create_empty_texture(160, 102);
        framebuffer = (unsigned int *)vita2d_texture_get_datap(framebufferTex);
        snd_buffer8 = (unsigned char *) &(lynx->gAudioBuffer);
        lynx->gAudioEnabled = true;
        /*audioOutPort=sceAudioOutOpenPort(PSP2_AUDIO_OUT_PORT_TYPE_MAIN,4096,48000,PSP2_AUDIO_OUT_MODE_STEREO);
        printf("%x",audioOutPort);
        SceUID sound_thid = sceKernelCreateThread("Sound thread",sound_thread,0x10000100,0x10000,0,0,NULL);
        printf("%x",sound_thid);
        sceKernelStartThread(sound_thid,sizeof(*lynx),lynx);*/
        pspAudioSetChannelCallback(0, AudioCallback, 0);

        return (UBYTE*)framebuffer;
    }

    //video_cb(framebuffer, lynx_width, lynx_height, 160*2);
    vita2d_draw_texture_lcd3x(framebufferTex, pos_x, pos_y, scale*1.0f, scale*1.0f);



    newFrame = true;
    return (UBYTE*)framebuffer;
}

unsigned get_lynx_input(SceCtrlData pad, SceCtrlData old_pad)
{
    unsigned res = 0;
    unsigned int keys_down = pad.buttons & ~old_pad.buttons;
    //unsigned int keys_up = ~pad.buttons & old_pad.buttons;

    for (unsigned i = 0; i < sizeof(btn_map_no_rot) / sizeof(map); ++i)
    {
        res |= pad.buttons & btn_map_no_rot[i].psp2 ? btn_map_no_rot[i].lynx : 0;
    }

    if (keys_down & PSP2_CTRL_SQUARE) {
			scale--;
			if (scale < 1) scale = 1;
			pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
			pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
		} else if (keys_down & PSP2_CTRL_TRIANGLE) {
			scale++;
			if (scale > 5) scale = 5;
			pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
			pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
		} else if (keys_down & PSP2_CTRL_SELECT) {
			endEmulation=true;
		}

    return res;
}

int emulate(char *path){
  char *system_rom
    = (char*)malloc(sizeof(char) * (strlen("cache0:/VitaDefilerClient/Documents/") + 13));
  sprintf(system_rom, "%slynxboot.img", "cache0:/VitaDefilerClient/Documents/");



 //printf("Loading lynx.... %s",path);

  lynx = new CSystem(path, system_rom);
  pspAudioInit(4096, 0);
 //printf("Lynx loaded: %p",lynx);
  int pause = 0;

  SceCtrlData pad, old_pad;
  ULONG rot = MIKIE_NO_ROTATE;
  lynx_width = 160;
  lynx_height = 102;

	lynx->DisplaySetAttributes(rot, MIKIE_PIXEL_FORMAT_32BPP, 160*sizeof(uint32_t), lynx_display_callback, (ULONG)0);

	//chip8_init(&chip8, 64, 32);
	//chip8_loadrom_memory(&chip8, PONG2_bin, PONG2_bin_size);

	scale = 5;
	pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
	pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
	initialized = true;

	while (!endEmulation) {

		sceCtrlPeekBufferPositive(0, (SceCtrlData *)&pad, 1);

    lynx->SetButtonData(get_lynx_input(pad,old_pad));

    vita2d_start_drawing();
    vita2d_clear_screen();



		while(!newFrame&&!pause&&!endEmulation)
    {
        lynx->Update();
    }

    //printf("ending frame");
    newFrame = false;




		old_pad = pad;

    vita2d_end_drawing();
		vita2d_swap_buffers();

	}
  initialized = false;
  endEmulation = false;
  lynx->gSystemHalt=true;
  pspAudioShutdown();
  delete lynx;
  return 0;

}



#define ROOT_PATH "cache0:/"
#define START_PATH "cache0:/VitaDefilerClient/Documents/"
// Default Start Path

// Current Path
static char cwd[1024];

// Generic Definitions
#define CLEAR 1
#define KEEP 0

// GUI Definitions
#define FONT_SIZE 7
#define PADDING 3
#define FONT_COLOR 0xFFFFFFFF
#define FONT_SELECT_COLOR 0xFFFFD800
#define BACK_COLOR 0xFFBCAD94
#define CENTER_X(s) (240 - ((strlen(s) * FONT_SIZE) >> 1))
#define RIGHT_X(s) (480 - (strlen(s) * FONT_SIZE))
#define CENTER_Y 132
#define FILES_PER_PAGE 18

// Button Test Macro
#define PRESSED(b, a, m) (((b & m) == 0) && ((a & m) == m))

// File Information Structure
typedef struct File
{
	// Next Item
	struct File * next;

	// Folder Flag
	int isFolder;

	// File Name
	char name[256];
} File;

// Menu Position
int position = 0;

// Number of Files
int filecount = 0;

// File List
File * files = NULL;

void printoob(const char * text, int x, int y, unsigned int color);
void updateList(int clearindex);
void paintList(int withclear);
int navigate(void);
File * findindex(int index);
void recursiveFree(File * node);




int main()
{
  // Set Start Path
	strcpy(cwd, START_PATH);

	// Initialize Screen Output
  vita2d_init();
  vita2d_init_lcd3x();



	// Update List
	updateList(CLEAR);

	// Paint List
	paintList(KEEP);

	// Last Buttons
	unsigned int lastbuttons = 0;

	// Input Loop
	while(1)
	{
		// Button Data
		SceCtrlData data;

		// Clear Memory
		memset(&data, 0, sizeof(data));

		// Read Button Data
		sceCtrlSetSamplingMode(1);
		sceCtrlPeekBufferPositive(0, &data, 1);

		// Other Commands
		if(filecount > 0)
		{
			// Start File
			if(PRESSED(lastbuttons, data.buttons, PSP2_CTRL_START))
			{
				// Start File
        File * file = findindex(position);
        // Not a valid file
        if(file == NULL || file->isFolder) continue;

        // File Path
	      char path[1024];

      	// Puzzle Path
      	strcpy(path, cwd);
      	strcpy(path + strlen(path), file->name);

        /*char *path
      		= (char*)malloc(sizeof(char) * (strlen("cache0:/VitaDefilerClient/Documents/rom.lnx")));
      	sprintf(path, "cache0:/VitaDefilerClient/Documents/rom.lnx");*/

        emulate(path);

        paintList(KEEP);
			}
      // Position Decrement
			else if(PRESSED(lastbuttons, data.buttons, PSP2_CTRL_UP))
			{
				// Decrease Position
				if(position > 0) position--;

				// Rewind Pointer
				else position = filecount - 1;

				// Paint List
				paintList(KEEP);
			}

			// Position Increment
			else if(PRESSED(lastbuttons, data.buttons, PSP2_CTRL_DOWN))
			{
				// Increase Position
				if(position < (filecount - 1)) position++;

				// Rewind Pointer
				else position = 0;

				// Paint List
				paintList(KEEP);
			}
      // Navigate to Folder
			else if(PRESSED(lastbuttons, data.buttons, PSP2_CTRL_CROSS))
			{
				// Attempt to navigate to Target
				if(navigate() == 0)
				{
					// Update List
					updateList(CLEAR);

					// Paint List
					paintList(CLEAR);
				}
			}
    }

		// Copy Buttons to Memory
		lastbuttons = data.buttons;

		// Delay Thread (~100FPS are enough)
		sceKernelDelayThread(10000);
	}

	vita2d_fini();

	return 0;
}

void printoob(const char * text, int x, int y, unsigned int color)
{
	// Convert screen coordinates from 480x272 to 960x544
	font_draw_string((x*2*95)/100, y*2, color, text);
}

// Update File List
void updateList(int clearindex)
{
	// Clear List
	recursiveFree(files);
	files = NULL;
	filecount = 0;

	// Open Working Directory
	int directory = sceIoDopen(cwd);

	// Opened Directory
	if(directory >= 0)
	{
		/* Add fake ".." entry except on root */
		if (strcmp(cwd, ROOT_PATH)) {

			// New List
			files = (File *)malloc(sizeof(File));

			// Clear Memory
			memset(files, 0, sizeof(File));

			// Copy File Name
			strcpy(files->name, "..");

			// Set Folder Flag
			files->isFolder = 1;

			filecount++;
		}

		// File Info Read Result
		int dreadresult = 1;

		// Iterate Files
		while(dreadresult > 0)
		{
			// File Info
			SceIoDirent info;

			// Clear Memory
			memset(&info, 0, sizeof(info));

			// Read File Data
			dreadresult = sceIoDread(directory, &info);

			// Read Success
			if(dreadresult >= 0)
			{
				// Ingore null filename
				if(info.d_name[0] == '\0') continue;

				// Ignore "." in all Directories
				if(strcmp(info.d_name, ".") == 0) continue;

				// Ignore ".." in Root Directory
				if(strcmp(cwd, ROOT_PATH) == 0 && strcmp(info.d_name, "..") == 0) continue;

				// Allocate Memory
				File * item = (File *)malloc(sizeof(File));

				// Clear Memory
				memset(item, 0, sizeof(File));

				// Copy File Name
				strcpy(item->name, info.d_name);

				// Set Folder Flag
				item->isFolder = PSP2_S_ISDIR(info.d_stat.st_mode);

				// New List
				if(files == NULL) files = item;

				// Existing List
				else
				{
					// Iterator Variable
					File * list = files;

					// Append to List
					while(list->next != NULL) list = list->next;

					// Link Item
					list->next = item;
				}

				// Increase File Count
				filecount++;
			}
		}

		// Close Directory
		sceIoDclose(directory);
	}

	// Attempt to keep Index
	if(!clearindex)
	{
		// Fix Position
		if(position >= filecount) position = filecount - 1;
	}

	// Reset Position
	else position = 0;
}

// Paint Picker List
void paintList(int withclear)
{
	// Clear Screen
	//if(withclear)
  vita2d_start_drawing();
  vita2d_clear_screen();

	// Paint Current Path
	printoob(cwd, 10, 10, FONT_COLOR);

	// Paint Current Runlevel
	/*char * strrunlevel = getModeStr(mode);
	printoob(strrunlevel, RIGHT_X(strrunlevel) - 10, 10, FONT_COLOR);
  */
  printoob("HandyVITA emulator by frangarcj", 10, 242, FONT_SELECT_COLOR);

  // Paint Controls
	printoob("[ UP & DOWN ]    File Selection", 10, 252, FONT_COLOR);
	printoob("[ LEFT & RIGHT ] Mode Selection", 10, 262, FONT_COLOR);
	printoob("[ START ]  RUN", 330, 252, FONT_COLOR);
	printoob("[ CROSS ] Navigate", 330, 262, FONT_COLOR);


	// File Iterator Variable
	int i = 0;

	// Print Counter
	int printed = 0;

	// Paint File List
	File * file = files;
	for(; file != NULL; file = file->next)
	{
		// Printed enough already
		if(printed == FILES_PER_PAGE) break;

		// Interesting File
		if(position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE))
		{
			// Default Font Color
			unsigned int color = FONT_COLOR;

			// Selected File Font Color
			if(i == position) color = FONT_SELECT_COLOR;

			// Print Node Type
			printoob((file->isFolder) ? ("D") : ("F"), 10, 30 + (FONT_SIZE + PADDING) * printed, FONT_COLOR);

			char buf[64];

			strncpy(buf, file->name, sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';
			int len = strlen(buf);
			len = 40 - len;

			while(len -- > 0)
			{
				strcat(buf, " ");
			}

			// Print Filename
			printoob(buf, 20, 30 + (FONT_SIZE + PADDING) * printed, color);

			// Increase Print Counter
			printed++;
		}

		// Increase Counter
		i++;
	}

  vita2d_end_drawing();
  vita2d_swap_buffers();
}

int navigate(void)
{
	// Find File
	File * file = findindex(position);

	// Not a Folder
	if(file == NULL || !file->isFolder) return -1;

	// Special Case ".."
	if(strcmp(file->name, "..") == 0)
	{
		// Slash Pointer
		char * slash = NULL;

		// Find Last '/' in Working Directory
		int i = strlen(cwd) - 2; for(; i >= 0; i--)
		{
			// Slash discovered
			if(cwd[i] == '/')
			{
				// Save Pointer
				slash = cwd + i + 1;

				// Stop Search
				break;
			}
		}

		// Terminate Working Directory
		slash[0] = 0;
	}

	// Normal Folder
	else
	{
		// Append Folder to Working Directory
		strcpy(cwd + strlen(cwd), file->name);
		cwd[strlen(cwd) + 1] = 0;
		cwd[strlen(cwd)] = '/';
	}

	// Return Success
	return 0;
}

// Find File Information by Index
File * findindex(int index)
{
	// File Iterator Variable
	int i = 0;

	// Find File Item
	File * file = files; for(; file != NULL && i != index; file = file->next) i++;

	// Return File
	return file;
}

void recursiveFree(File * node)
{
	// End of List
	if(node == NULL) return;

	// Nest Further
	recursiveFree(node->next);

	// Free Memory
	free(node);
}
