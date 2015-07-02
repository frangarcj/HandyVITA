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

#include "../lynx/system.h"
#include "../lynx/lynxdef.h"
#include "utils.h"
#include "draw.h"

/*extern "C" void __cxa_pure_virtual()
{
	while (1);
}

extern "C" void *__dso_handle = NULL;*/

const SceSize sceUserMainThreadStackSize = 100*1024*1024;

static uint8_t lynx_width = 160;
static uint8_t lynx_height = 102;

static uint32_t framebuffer[160*102];

static int scale;
static int pos_x;
static int pos_y;

bool newFrame = false;
static bool initialized = false;


static CSystem *lynx = NULL;

unsigned char* lynx_display_callback(ULONG objref)
{
    printf("lynx_display_callback %d",initialized);
    if(!initialized)
    {
        return (UBYTE*)framebuffer;
    }

		//PINTAR FRAMEBUFFER

    //video_cb(framebuffer, lynx_width, lynx_height, 160*2);
    blit_scale(framebuffer,pos_x,pos_y,lynx_width,lynx_height,scale);
    printf("blit_scale done");

    /*if(gAudioBufferPointer > 0)
    {
        int f = gAudioBufferPointer;
        lynx_sound_stream_update(soundBuffer, gAudioBufferPointer);
        audio_batch_cb((const int16_t*)soundBuffer, f);
    }*/

    newFrame = true;
    return (UBYTE*)framebuffer;
}

PSP2_MODULE_INFO(0, 0, "handyvita")
int main()
{
	SceCtrlData pad, old_pad;
	//struct chip8_context chip8;
	int pause = 0;

	init_video();
	char *system_rom
    = (char*)malloc(sizeof(char) * (strlen("cache0:/VitaDefilerClient/Documents/") + 13));
  sprintf(system_rom, "%slynxboot.img", "cache0:/VitaDefilerClient/Documents/");

	char *path
		= (char*)malloc(sizeof(char) * (strlen("cache0:/VitaDefilerClient/Documents/rom.lnx")));
	sprintf(path, "cache0:/VitaDefilerClient/Documents/rom.lnx");

  printf("Loading lynx.... %s",path);

  lynx = new CSystem(path, system_rom);

  printf("Lynx loaded: %p",lynx);

  ULONG rot = MIKIE_NO_ROTATE;
  lynx_width = 160;
  lynx_height = 102;

	lynx->DisplaySetAttributes(rot, MIKIE_PIXEL_FORMAT_32BPP, 160*sizeof(uint32_t), lynx_display_callback, (ULONG)0);

	//chip8_init(&chip8, 64, 32);
	//chip8_loadrom_memory(&chip8, PONG2_bin, PONG2_bin_size);

	scale = 2;
	pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
	pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
	initialized = true;

	while (1) {
		clear_screen();

		sceCtrlPeekBufferPositive(0, (SceCtrlData *)&pad, 1);

		font_draw_stringf(10, 10, BLACK, "HANDY emulator by frangarcj");

		unsigned int keys_down = pad.buttons & ~old_pad.buttons;
		unsigned int keys_up = ~pad.buttons & old_pad.buttons;

		if (keys_down & PSP2_CTRL_UP) {
			//chip8_key_press(&chip8, 1);
		} else if (keys_up & PSP2_CTRL_UP) {
			//chip8_key_release(&chip8, 1);
		}
		if (keys_down & PSP2_CTRL_DOWN) {
			//chip8_key_press(&chip8, 4);
		} else if (keys_up & PSP2_CTRL_DOWN) {
			//chip8_key_release(&chip8, 4);
		}

		if (keys_down & PSP2_CTRL_TRIANGLE) {
			//chip8_key_press(&chip8, 0xC);
		} else if (keys_up & PSP2_CTRL_TRIANGLE) {
			//chip8_key_release(&chip8, 0xC);
		}
		if (keys_down & PSP2_CTRL_CROSS) {
			//chip8_key_press(&chip8, 0xD);
		} else if (keys_up & PSP2_CTRL_CROSS) {
			//chip8_key_release(&chip8, 0xD);
		}

		if (pad.buttons & PSP2_CTRL_LTRIGGER) {
			scale--;
			if (scale < 1) scale = 1;
			/* Re-center the image */
			pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
			pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
		} else if (pad.buttons & PSP2_CTRL_RTRIGGER) {
			scale++;
			/* Don't go outside of the screen! */
			if ((lynx_width*scale) > SCREEN_W) scale--;
			/* Re-center the image */
			pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
			pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
		}

		if (keys_down & PSP2_CTRL_START) {
			pause = !pause;
		}
    //printf("starting frame");
		while(!newFrame)
    {
        lynx->Update();
    }

    //printf("ending frame");
    newFrame = false;


		if (pause) {
			font_draw_stringf(SCREEN_W/2 - 40, SCREEN_H - 50, BLACK, "PAUSE");
		}

		old_pad = pad;
		swap_buffers();
		sceDisplayWaitVblankStart();
	}

	//chip8_fini(&chip8);
	end_video();
	return 0;
}
