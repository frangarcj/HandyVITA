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
#include <vita2d.h>


/*extern "C" void __cxa_pure_virtual()
{
	while (1);
}

extern "C" void *__dso_handle = NULL;*/

const SceSize sceUserMainThreadStackSize = 8*1024*1024;

static uint8_t lynx_width = 160;
static uint8_t lynx_height = 102;

unsigned int *framebuffer;
vita2d_texture *framebufferTex;


static int scale;
static int pos_x;
static int pos_y;

bool newFrame = false;
static bool initialized = false;


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

unsigned char* lynx_display_callback(ULONG objref)
{
    //printf("lynx_display_callback %d",initialized);
    if(!initialized)
    {
        framebufferTex = vita2d_create_empty_texture(160, 102);
        framebuffer = (unsigned int *)vita2d_texture_get_datap(framebufferTex);


        return (UBYTE*)framebuffer;
    }

		//PINTAR FRAMEBUFFER

    //video_cb(framebuffer, lynx_width, lynx_height, 160*2);
    vita2d_draw_texture_scale(framebufferTex, pos_x, pos_y, scale*1.0f, scale*1.0f);

    /*if(gAudioBufferPointer > 0)
    {
        int f = gAudioBufferPointer;
        lynx_sound_stream_update(soundBuffer, gAudioBufferPointer);
        audio_batch_cb((const int16_t*)soundBuffer, f);
    }*/

    newFrame = true;
    return (UBYTE*)framebuffer;
}

unsigned get_lynx_input(SceCtrlData pad)
{
    unsigned res = 0;
    unsigned int keys_down = pad.buttons;

    for (unsigned i = 0; i < sizeof(btn_map_no_rot) / sizeof(map); ++i)
    {
        res |= keys_down & btn_map_no_rot[i].psp2 ? btn_map_no_rot[i].lynx : 0;
    }

    if (pad.buttons & PSP2_CTRL_SQUARE) {
			scale--;
			if (scale < 1) scale = 1;
			pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
			pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
		} else if (pad.buttons & PSP2_CTRL_TRIANGLE) {
			scale++;
			if (scale > 5) scale = 5;
			pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
			pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
		}

    return res;
}

PSP2_MODULE_INFO(0, 0, "handyvita")
int main()
{
	SceCtrlData pad, old_pad;
	//struct chip8_context chip8;
	int pause = 0;

	vita2d_init();


	char *system_rom
    = (char*)malloc(sizeof(char) * (strlen("cache0:/VitaDefilerClient/Documents/") + 13));
  sprintf(system_rom, "%slynxboot.img", "cache0:/VitaDefilerClient/Documents/");

	char *path
		= (char*)malloc(sizeof(char) * (strlen("cache0:/VitaDefilerClient/Documents/rom.lnx")));
	sprintf(path, "cache0:/VitaDefilerClient/Documents/rom.lnx");

 //printf("Loading lynx.... %s",path);

  lynx = new CSystem(path, system_rom);

 //printf("Lynx loaded: %p",lynx);

  ULONG rot = MIKIE_NO_ROTATE;
  lynx_width = 160;
  lynx_height = 102;

	lynx->DisplaySetAttributes(rot, MIKIE_PIXEL_FORMAT_32BPP, 160*sizeof(uint32_t), lynx_display_callback, (ULONG)0);

	//chip8_init(&chip8, 64, 32);
	//chip8_loadrom_memory(&chip8, PONG2_bin, PONG2_bin_size);

	scale = 1;
	pos_x = SCREEN_W/2 - (lynx_width/2)*scale;
	pos_y = SCREEN_H/2 - (lynx_height/2)*scale;
	initialized = true;

	while (1) {

		sceCtrlPeekBufferPositive(0, (SceCtrlData *)&pad, 1);

    lynx->SetButtonData(get_lynx_input(pad));

    vita2d_start_drawing();
    vita2d_clear_screen();

    font_draw_stringf(10, 10, WHITE, "HandyVITA emulator by frangarcj");

		while(!newFrame&&!pause)
    {
        lynx->Update();
    }

    //printf("ending frame");
    newFrame = false;


		if (pause) {
			font_draw_stringf(SCREEN_W/2 - 40, SCREEN_H - 50, WHITE, "PAUSE");
		}

		old_pad = pad;

		vita2d_end_drawing();
		vita2d_swap_buffers();
	}

	//chip8_fini(&chip8);
	vita2d_fini();
	return 0;
}
