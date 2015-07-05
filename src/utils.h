#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <psp2/types.h>

//#include "chip-8.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RGBA8(r, g, b, a)      ((((a)&0xFF)<<24) | (((b)&0xFF)<<16) | (((g)&0xFF)<<8) | (((r)&0xFF)<<0))

#define SCREEN_W 960
#define SCREEN_H 544

#define RED   RGBA8(255,0,0,255)
#define GREEN RGBA8(0,255,0,255)
#define BLUE  RGBA8(0,0,255,255)
#define BLACK RGBA8(0,0,0,255)
#define WHITE RGBA8(255,255,255,255)

#ifdef __cplusplus
}
#endif

#endif
