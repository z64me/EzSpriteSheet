/*
 * SDL_EzText.h <z64.me>
 * version 1.3.0
 *
 * a simple bitmap text blitting helper for SDL2
 *
 * https://gist.github.com/z64me/2e1728bef2d544c3310069fc87a43cc7
 *
 */

#include <SDL2/SDL.h>

/* draw text */
void SDL_EzText(SDL_Renderer *r, int x, int y, const char *str);

/* draw text with a shadow */
void SDL_EzTextShadow(SDL_Renderer *r, int x, int y, const char *str);

/* draw text with rectangle behind it */
void SDL_EzTextRect(SDL_Renderer *r, int x, int y, const char *str);

/* return width of string in pixels */
unsigned int SDL_EzTextStringWidth(const char *str);

/* return height of string in pixels */
unsigned int SDL_EzTextStringHeight(const char *str);

/* returns width of last-drawn string in pixels */
unsigned int SDL_EzTextLastWidth(void);

/* set text color */
void SDL_EzText_SetColor(Uint32 rgba);

#ifdef SDL_EZTEXT_IMPLEMENTATION

#include <string.h> /* strlen */
#include <stdlib.h> /* malloc */

/*
 *
 * private interface
 *
 */

static struct
{
	SDL_Texture *tex;
	Uint32 color; /* font color rgba8888 */
	Uint32 shadow; /* shadow color rgba8888 */
	Uint32 rect; /* rectangle color rgba8888 */
	Uint32 last_width; /* width of last-drawn string in pixels */
	int font_w;
	int font_h;
} SDL__EzText = {
	.color = -1 /* default font color = white */
	, .shadow = 0x000000ff /* default shadow color = black */
	, .rect = 0x000000ff /* default rectangle color = black */
};

static
void
SDL__EzText_Decode(Uint32 *dst, const Uint8 *src, int num)
{
	Uint32 arr[] = {0, 0xFFFFFFFF};
	num /= 8;
	while (num--)
	{
		dst[0] = arr[(*src>>7)&1];
		dst[1] = arr[(*src>>6)&1];
		dst[2] = arr[(*src>>5)&1];
		dst[3] = arr[(*src>>4)&1];
		dst[4] = arr[(*src>>3)&1];
		dst[5] = arr[(*src>>2)&1];
		dst[6] = arr[(*src>>1)&1];
		dst[7] = arr[(*src>>0)&1];
		src += 1;
		dst += 8;
	}
}


/* get pixels for font crisp */
static
Uint32 *
SDL__EzText_Crisp(int *surf_w, int *surf_h, int *char_w, int *char_h)
{
	static const int crisp_png_w = 768;
	static const int crisp_png_h = 16;
	static const Uint8 crisp_png_pix[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 0, 0, 0, 0, 8, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 36, 0, 8, 0, 0, 8, 4, 16, 0, 0, 0, 0, 0, 2, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 28, 64, 28, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 8, 48, 0, 0, 0, 8, 36
	, 0, 8, 32, 0, 8, 8, 8, 0, 0, 0, 0, 0, 2, 60, 8, 60, 60, 4, 126
	, 60, 126, 60, 60, 0, 0, 0, 0, 0, 28, 0, 60, 124, 30, 124, 126
	, 126, 30, 66, 62, 28, 65, 64, 65, 98, 28, 124, 28, 124, 62, 127
	, 65, 65, 65, 65, 65, 127, 16, 64, 4, 0, 0, 2, 0, 64, 0, 2, 0, 14
	, 0, 64, 8, 4, 64, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 8, 8, 8, 0, 0, 0, 8, 36, 20, 62, 81, 56, 8, 8, 8, 0, 0, 0, 0, 0
	, 4, 66, 56, 66, 66, 12, 64, 64, 66, 66, 66, 0, 0, 0, 0, 0, 34, 28
	, 66, 66, 33, 66, 64, 64, 33, 66, 8, 4, 66, 64, 65, 98, 34, 66, 34
	, 66, 65, 8, 65, 65, 65, 65, 65, 1, 16, 32, 4, 0, 0, 0, 0, 64, 0, 2
	, 0, 16, 0, 64, 8, 4, 64, 16, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0
	, 0, 0, 8, 8, 8, 0, 0, 0, 8, 0, 20, 73, 82, 68, 0, 16, 4, 0, 0, 0
	, 0, 0, 4, 70, 8, 2, 2, 20, 64, 64, 4, 66, 66, 8, 8, 4, 0, 32, 2
	, 34, 66, 66, 64, 65, 64, 64, 64, 66, 8, 4, 68, 64, 99, 82, 65, 66
	, 65, 66, 64, 8, 65, 65, 65, 34, 34, 2, 16, 32, 4, 0, 0, 0, 0, 64
	, 0, 2, 0, 16, 0, 64, 0, 0, 64, 16, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0
	, 0, 0, 0, 0, 8, 8, 8, 0, 0, 0, 8, 0, 127, 72, 36, 68, 0, 16, 4, 8
	, 8, 0, 0, 0, 8, 78, 8, 2, 2, 36, 124, 124, 4, 66, 66, 8, 8, 8, 0
	, 16, 2, 73, 66, 66, 64, 65, 64, 64, 64, 66, 8, 4, 72, 64, 99, 82
	, 65, 66, 65, 66, 64, 8, 65, 34, 65, 20, 20, 4, 16, 16, 4, 8, 0, 0
	, 60, 124, 60, 62, 60, 60, 62, 124, 56, 28, 68, 16, 118, 124, 60
	, 124, 62, 30, 60, 60, 66, 34, 65, 98, 66, 126, 8, 8, 8, 0, 0, 0
	, 8, 0, 20, 62, 8, 56, 0, 16, 4, 42, 8, 0, 0, 0, 8, 90, 8, 60, 28
	, 68, 2, 66, 8, 60, 66, 0, 0, 16, 62, 8, 4, 85, 66, 124, 64, 65
	, 124, 124, 64, 126, 8, 4, 112, 64, 85, 74, 65, 124, 65, 124, 62
	, 8, 65, 34, 73, 8, 8, 8, 16, 16, 4, 20, 0, 0, 2, 66, 66, 66, 66
	, 16, 66, 66, 8, 4, 72, 16, 73, 66, 66, 66, 66, 32, 64, 16, 66, 34
	, 65, 20, 66, 4, 8, 8, 8, 50, 0, 0, 8, 0, 20, 9, 18, 72, 0, 16, 4
	, 28, 62, 0, 62, 0, 16, 114, 8, 64, 2, 126, 2, 66, 8, 66, 62, 0, 0
	, 32, 0, 4, 8, 85, 126, 66, 64, 65, 64, 64, 71, 66, 8, 4, 72, 64
	, 85, 74, 65, 64, 65, 72, 1, 8, 65, 20, 73, 20, 8, 16, 16, 8, 4
	, 34, 0, 0, 62, 66, 64, 66, 126, 16, 66, 66, 8, 4, 112, 16, 73, 66
	, 66, 66, 66, 32, 60, 16, 66, 34, 73, 8, 66, 8, 48, 8, 6, 76, 0, 0
	, 0, 0, 127, 73, 37, 69, 0, 16, 4, 42, 8, 0, 0, 0, 16, 98, 8, 64
	, 2, 4, 2, 66, 16, 66, 2, 0, 0, 16, 62, 8, 0, 74, 66, 66, 64, 65
	, 64, 64, 65, 66, 8, 4, 68, 64, 73, 70, 65, 64, 65, 68, 1, 8, 65
	, 20, 73, 34, 8, 32, 16, 8, 4, 0, 0, 0, 66, 66, 64, 66, 64, 16, 66
	, 66, 8, 4, 72, 16, 73, 66, 66, 66, 66, 32, 2, 16, 66, 34, 73, 24
	, 66, 16, 8, 8, 8, 0, 0, 0, 8, 0, 20, 62, 69, 66, 0, 16, 4, 8, 8
	, 8, 0, 8, 32, 66, 8, 64, 66, 4, 66, 66, 16, 66, 2, 8, 8, 8, 0, 16
	, 8, 32, 66, 66, 33, 66, 64, 64, 33, 66, 8, 4, 66, 64, 73, 70, 34
	, 64, 34, 66, 65, 8, 34, 8, 73, 65, 8, 64, 16, 4, 4, 0, 0, 0, 66
	, 66, 66, 66, 66, 16, 66, 66, 8, 4, 68, 16, 73, 66, 66, 66, 66, 32
	, 66, 16, 66, 20, 73, 36, 66, 32, 8, 8, 8, 0, 0, 0, 8, 0, 20, 8, 2
	, 61, 0, 16, 4, 0, 0, 8, 0, 8, 32, 60, 62, 126, 60, 4, 60, 60, 16
	, 60, 60, 8, 8, 4, 0, 32, 8, 30, 66, 124, 30, 124, 126, 64, 30, 66
	, 62, 56, 65, 126, 65, 66, 28, 64, 28, 65, 62, 8, 28, 8, 54, 65
	, 8, 127, 16, 4, 4, 0, 126, 0, 62, 124, 60, 62, 60, 16, 62, 66, 62
	, 4, 66, 12, 73, 66, 60, 124, 62, 32, 60, 12, 62, 8, 54, 67, 62
	, 126, 8, 8, 8, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 8, 8, 0, 0, 16, 0, 0
	, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 16, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 4, 0, 0, 0, 0
	, 0, 64, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 8, 8, 8, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 8, 8, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 2, 4, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 2, 0, 0, 4, 0, 0, 0, 0, 0, 64, 2, 0, 0, 0, 0, 0, 0
	, 0, 2, 0, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 16, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 28, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 60, 0, 0, 56
	, 0, 0, 0, 0, 0, 64, 2, 0, 0, 0, 0, 0, 0, 0, 60, 0, 6, 8, 48, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	Uint32 *result = malloc(crisp_png_w * crisp_png_h * sizeof(Uint32));
	
	if (!result)
		return 0;
	
	SDL__EzText_Decode(result, crisp_png_pix, crisp_png_w * crisp_png_h);
	
	*surf_w = crisp_png_w;
	*surf_h = crisp_png_h;
	*char_w = 8;
	*char_h = 16;
	
	return result;
}

/* convert raw pixel data to a texture */
static
SDL_Texture *
SDL__EzTextTexture(SDL_Renderer *renderer, void *pix, int w, int h)
{
	SDL_Surface *surf;
	SDL_Texture *tex;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	const Uint32 fmt = SDL_PIXELFORMAT_RGBA32;
#else
	const Uint32 fmt = SDL_PIXELFORMAT_ABGR32;
#endif
	
	/* convert */
	surf = SDL_CreateRGBSurfaceWithFormatFrom(pix, w, h, 32, 4 * w, fmt);
	if (!surf)
		return 0;
	tex = SDL_CreateTextureFromSurface(renderer, surf);
	if (!tex)
	{
		SDL_FreeSurface(surf);
		return 0;
	}
	
	/* set flags */
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	
	/* cleanup */
	SDL_FreeSurface(surf);
	
	return tex;
}


/* returns zero if font texture is ready, non-zero on error */
static
int
SDL__EzTextLoad(SDL_Renderer *renderer)
{
	int surf_w;
	int surf_h;
	int font_w;
	int font_h;
	Uint32 *pix;
	
	/* font texture already loaded */
	if (SDL__EzText.tex)
		return 0;
	
	/* get decoded font image */
	pix = SDL__EzText_Crisp(&surf_w, &surf_h, &font_w, &font_h);
	if (!pix)
		return 1;
	
	/* convert to SDL_Texture */
	SDL__EzText.tex = SDL__EzTextTexture(renderer, pix, surf_w, surf_h);
	if (!SDL__EzText.tex)
		return 1;
	
	/* success */
	SDL__EzText.font_w = font_w;
	SDL__EzText.font_h = font_h;
	return 0;
}

/*
 *
 * public interface
 *
 */

/* return width of string in pixels */
unsigned int SDL_EzTextStringWidth(const char *str)
{
	if (!str)
		return 0;
	
	return strlen(str) * SDL__EzText.font_w;
}

/* return height of string in pixels */
unsigned int SDL_EzTextStringHeight(const char *str)
{
	unsigned int font_h = SDL__EzText.font_h;
	unsigned int y = font_h;
	unsigned int end;
	unsigned int a;
	
	if (!str)
		return 0;
	
	end = strlen(str);
	for (a = 0; a < end; ++a)
	{
		unsigned char c = str[a];
		
		/* utf8: make unknown characters display as '?' */
		if (0xf0 == (0xf8 & c)) {
			// 4-byte utf8 code point (began with 0b11110xxx)
			a += 3;
			c = '?';
		} else if (0xe0 == (0xf0 & c)) {
			// 3-byte utf8 code point (began with 0b1110xxxx)
			a += 2;
			c = '?';
		} else if (0xc0 == (0xe0 & c)) {
			// 2-byte utf8 code point (began with 0b110xxxxx)
			a += 1;
			c = '?';
		} else { // if (0x00 == (0x80 & *s)) {
			// 1-byte ascii (began with 0b0xxxxxxx)
		}
		
		if (c == '\n') // newline
			y += font_h;
	}
	
	return y;
}

/* returns width of last-drawn string in pixels */
unsigned int SDL_EzTextLastWidth(void)
{
	return SDL__EzText.last_width;
}

/* set text color */
void SDL_EzText_SetColor(Uint32 rgba)
{
	SDL__EzText.color = rgba;
}

/* draw text */
void SDL_EzText(SDL_Renderer *r, int x, int y, const char *str)
{
	int ox = x;
	//int oy = y;
	int end;
	SDL_Texture *tex = SDL__EzText.tex;
	Uint32 color = SDL__EzText.color;
	SDL__EzText.last_width = 0;
	
	SDL_SetTextureColorMod(tex, color >> 24, color >> 16, color >> 8);
	
	if (SDL__EzTextLoad(r))
		return;
	
	if (!str)
		return;
	
	end = strlen(str);
	for (int a = 0; a < end; ++a)
	{
		unsigned char c = str[a];
		int font_w = SDL__EzText.font_w;
		int font_h = SDL__EzText.font_h;
		SDL_Rect srcrect = {0, 0, font_w, font_h};
		SDL_Rect dstrect = {x, y, font_w, font_h};
		
		/* utf8: make unknown characters display as '?' */
		if (0xf0 == (0xf8 & c)) {
			// 4-byte utf8 code point (began with 0b11110xxx)
			a += 3;
			c = '?';
		} else if (0xe0 == (0xf0 & c)) {
			// 3-byte utf8 code point (began with 0b1110xxxx)
			a += 2;
			c = '?';
		} else if (0xc0 == (0xe0 & c)) {
			// 2-byte utf8 code point (began with 0b110xxxxx)
			a += 1;
			c = '?';
		} else { // if (0x00 == (0x80 & *s)) {
			// 1-byte ascii (began with 0b0xxxxxxx)
		}
		
		switch(c) {
			case '\n': // newline
				y += font_h;
				x = ox;
				continue;
				break;
			case 0xFF: // do nothing
			case '\r':
				break;
			default:   // text
				srcrect.x = font_w * (c - ' ');
				SDL_RenderCopy(r, tex, &srcrect, &dstrect);
				break;
		}
		x += font_w;
	}
	
	SDL__EzText.last_width = x - ox;
}

/* draw text with a shadow */
void SDL_EzTextShadow(SDL_Renderer *r, int x, int y, const char *str)
{
	SDL_Texture *tex = SDL__EzText.tex;
	Uint32 color = SDL__EzText.color;
	Uint32 shad = SDL__EzText.shadow;
	
	if (SDL__EzTextLoad(r))
		return;
	
	/* draw shadow text */
	SDL_SetTextureColorMod(tex, shad >> 24, shad >> 16, shad >> 8);
	SDL_EzText(r, x + 1, y + 1, str);
	
	/* draw main text */
	SDL_SetTextureColorMod(tex, color >> 24, color >> 16, color >> 8);
	SDL_EzText(r, x, y, str);
}

/* draw text with rectangle behind it */
void SDL_EzTextRect(SDL_Renderer *r, int x, int y, const char *str)
{
	Uint32 color = SDL__EzText.color;
	SDL_Rect rect;
	SDL_Texture *tex = SDL__EzText.tex;
	Uint32 rC = SDL__EzText.rect;
	Uint8 red, g, b, a;
	int font_w = SDL__EzText.font_w;
	int font_h = SDL__EzText.font_h;
	
	/* draw rectangle */
	rect.x = x - font_w;
	rect.w = SDL_EzTextStringWidth(str) + 2 * font_w;
	rect.y = y - font_h / 2;
	rect.h = font_h * 2;
	SDL_GetRenderDrawColor(r, &red, &g, &b, &a);
	SDL_SetRenderDrawColor(r, rC >> 24, rC >> 16, rC >> 8, rC);
	SDL_RenderFillRect(r, &rect);
	SDL_SetRenderDrawColor(r, red, g, b, a);
	
	/* draw main text */
	SDL_SetTextureColorMod(tex, color >> 24, color >> 16, color >> 8);
	SDL_EzText(r, x, y, str);
}

#endif /* SDL_EZTEXT_IMPLEMENTATION */

#ifdef SDL_EZTEXT_EXAMPLE
#undef SDL_EZTEXT_EXAMPLE

#include <SDL2/SDL.h>

#define SDL_EZTEXT_IMPLEMENTATION
#include "SDL_EzText.h"

#define WIN_W 512
#define WIN_H 512

int main(int argc, char *argv[])
{
	SDL_Renderer *renderer;
	SDL_Window  *window;
	int hasQuit = 0;
	
	if (SDL_Init(SDL_INIT_EVERYTHING))
		return -1;
	
	if (!(window = SDL_CreateWindow(
		"SDL_EzText Sample"
		, SDL_WINDOWPOS_CENTERED
		, SDL_WINDOWPOS_CENTERED
		, WIN_W
		, WIN_H
		, SDL_WINDOW_SHOWN
	)))
		return -1;
	
	if (!(renderer = SDL_CreateRenderer(
		window
		, -1
		, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	)))
		return -1;
	
	while (!hasQuit)
	{
		SDL_Event event;
		const char *str = "This is a sample string!";
		int x = (WIN_W - SDL_EzTextStringWidth(str)) / 2;
		int y = WIN_H / 2;
		
		/* exit condition */
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					hasQuit = 1;
					break;
				
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							hasQuit = 1;
							break;
					}
					break;
			}
		}
		
		/* clear background */
		SDL_SetRenderDrawColor(renderer, 0x80, 0x40, 0x40, -1);
		SDL_RenderClear(renderer);
		
		/* various text displays */
		SDL_EzText(renderer, x, y - 64, str);
		SDL_EzTextShadow(renderer, x, y, str);
		SDL_EzTextRect(renderer, x, y + 64, str);
		
		/* display result */
		SDL_RenderPresent(renderer);
	}
	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
	
	(void)argc;
	(void)argv;
}

#endif /* SDL_EZTEXT_EXAMPLE */

