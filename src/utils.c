#include <string.h>
#include "utils.h"
#include "draw.h"

int ffs(int i)
{
	int pos = 1;
	while (!(i & 0b1)) {
		i >>= 1;
		pos++;
	}
	return pos;
}

/*int chip8_loadrom_file(struct chip8_context *ctx, const char *path)
{
	return 0;
}

int chip8_loadrom_memory(struct chip8_context *ctx, const void *addr, unsigned int size)
{
	memcpy(&(ctx->RAM[0x200]), addr, size);
	return size;
}

void chip8_disp_to_buf(struct chip8_context *ctx, unsigned int *buffer)
{
	int x, y;
	unsigned int color;
	for (y = 0; y < ctx->disp_h; y++) {
		for (x = 0; x < ctx->disp_w; x++) {
			color = ((ctx->disp_mem[x/8 + (ctx->disp_w/8)*y]>>(7-x%8)) & 0b1) ? GREEN : BLUE;
			buffer[x + y*ctx->disp_w] = color;
		}
	}
}
*/
void blit_scale(unsigned int *buffer, int x, int y, int w, int h, int scale)
{
	int i, j;
	int i2, j2;
	unsigned int color;

	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			color = buffer[i + j*w];
			for (i2 = 0; i2 < scale; i2++) {
				for (j2 = 0; j2 < scale; j2++) {
					draw_pixel(x+i*scale + i2, y+j*scale + j2, color);
				}
			}
		}
	}
}

char *stpcpy(char *dest, const char *src)
{
	register char *d = dest;
	register const char *s = src;

	do {
		*d++ = *s;
	} while (*s++ != '\0');

	return d - 1;
}
