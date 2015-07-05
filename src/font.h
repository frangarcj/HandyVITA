/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#ifndef FONT_H
#define FONT_H

#include <psp2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void font_draw_char(int x, int y, unsigned int color, char c);
void font_draw_string(int x, int y, unsigned int color, const char *string);
void font_draw_stringf(int x, int y, unsigned int color, const char *s, ...);

#ifdef __cplusplus
}
#endif

#endif
