#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <psp2/types.h>

//#include "chip-8.h"

#ifdef __cplusplus
extern "C" {
#endif

int ffs(int i);

/*int chip8_loadrom_file(struct chip8_context *ctx, const char *path);
int chip8_loadrom_memory(struct chip8_context *ctx, const void *addr, unsigned int size);
void chip8_disp_to_buf(struct chip8_context *ctx, unsigned int *buffer);*/
void blit_scale(uint32_t *buffer, int x, int y, int w, int h, int scale);


#ifdef __cplusplus
}
#endif

#endif
