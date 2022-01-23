/*
 * main.c <z64.me>
 * 
 * a highly minimalistic C implementation demonstrating
 * how to integrate EzSpriteSheet into a game engine
 * 
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>

#include "EzSprite.h"

#define SDL_EZTEXT_IMPLEMENTATION
#include "SDL_EzText.h"

#define VIEW_W 256
#define VIEW_H 256

#define WIN_W (int)(VIEW_W * 2.5f)
#define WIN_H (int)(VIEW_H)

struct Game
{
	struct
	{
		SDL_Window    *window;
		SDL_Renderer  *renderer;
	} SDL;
	struct
	{
		int x;
		int y;
		int press:1;
		int release:1;
	} mouse;
	struct EzSpriteContext *spriteCtx;
};

struct Texture
{
	struct
	{
		SDL_Texture  *main;
		SDL_Texture  *rotated;
		int width;
		int height;
	} SDL;
};

static void rotate(SDL_Surface *surf)
{
	uint32_t *s32;
	uint32_t *d32;
	int x;
	int y;
	int w;
	int h;
	
	assert(surf);
	
	/* setup */
	SDL_LockSurface(surf);
	s32 = surf->pixels;
	w = surf->w;
	h = surf->h;
	
	/* create a temporary buffer for rotated pixels */
	d32 = calloc(w * h, sizeof(*d32));
	assert(d32);
	
	/* rotate pixels 90 degrees clockwise */
	for (y = 0; y < h; ++y)
	{
		for (x = 0; x < w; ++x)
		{
			int x1 = x;
			int y1 = y;
			int x2 = (h - 1) - y;
			int y2 = x;
			
			d32[x2 + y2 * h] = s32[x1 + y1 * w];
		}
	}
	
	/* overwrite original surface with rotated pixels */
	surf->w = h;
	surf->h = w;
	surf->pitch = h * sizeof(*d32);
	SDL_memcpy(s32, d32, w * h * sizeof(*d32));
	
	/* cleanup */
	free(d32);
	SDL_UnlockSurface(surf);
}

/*
 * 
 * a light binding enabling EzSprite to communicate
 * 
 */
static void *texture_load(
	void *udata
	, const char *fn
	, int width
	, int height
)
{
	struct Game *game = udata;
	struct Texture *t = calloc(1, sizeof(*t));
	SDL_Texture *tex;
	SDL_Surface *surf;
	
	assert(game);
	assert(t);
	
	t->SDL.width = width;
	t->SDL.height = height;
	
	/* load image with SDL_image */
	if (!(surf = IMG_Load(fn)))
		return 0;
	
	/* convert surface to texture */
	if (!(tex = SDL_CreateTextureFromSurface(game->SDL.renderer, surf)))
	{
		SDL_FreeSurface(surf);
		return 0;
	}
	t->SDL.main = tex;
	
	/* SDL2's builtin sprite rotation isn't crisp, so I
	 * make a pre-rotated copy of the sprite sheet
	 */
	rotate(surf);
	if (!(tex = SDL_CreateTextureFromSurface(game->SDL.renderer, surf)))
	{
		SDL_FreeSurface(surf);
		return 0;
	}
	t->SDL.rotated = tex;
	
	/* cleanup */
	SDL_FreeSurface(surf);
	
	return t;
}

static void texture_free(
	void *udata
	, void *texture
)
{
	struct Game *game = udata;
	struct Texture *t = texture;
	
	(void)game;
	assert(t);
	
	if (t->SDL.main)
		SDL_DestroyTexture(t->SDL.main);
	if (t->SDL.rotated)
		SDL_DestroyTexture(t->SDL.rotated);
}

static void texture_draw(
	void *udata
	, void *texture
	, int x
	, int y
	, int xflip
	, int yflip
	, int cx
	, int cy
	, int cw
	, int ch
	, int rotate
)
{
	struct Texture *t = texture;
	struct Game *game = udata;
	SDL_Texture *which;
	SDL_Rect src = {cx, cy, cw, ch};
	SDL_Rect dst = {x, y, cw, ch};
	SDL_RendererFlip flip = 0;
	
	if (xflip)
		flip |= SDL_FLIP_HORIZONTAL;
	if (yflip)
		flip |= SDL_FLIP_VERTICAL;
	
	assert(t);
	which = t->SDL.main;
	
	if (rotate)
	{
		int cx2 = t->SDL.height - cy;
		int cy2 = cx;
		
		which = t->SDL.rotated;
		
		src = (SDL_Rect){cx2 - ch, cy2, ch, cw};
		dst = (SDL_Rect){x, y, ch, cw};
	}
	
	SDL_RenderCopyEx(game->SDL.renderer
		, which
		, &src
		, &dst
		, 0
		, 0
		, flip
	);
}

static unsigned long ticks(void *udata)
{
	(void)udata;
	
	return SDL_GetTicks();
}

/* simple hoverable text label
 * returns non-zero when clicked, 0 otherwise
 */
static int label(struct Game *game, int *x, int *y, const char *text)
{
	assert(game);
	assert(x);
	assert(y);
	assert(text);
	
	SDL_Renderer *ren = game->SDL.renderer;
	SDL_Rect rect = {*x, *y, SDL_EzTextStringWidth(text), SDL_EzTextStringHeight(text)};
	int yadv = 24;
	int rval = 0;
	
	/* mouse hover */
	if (game->mouse.x >= rect.x
		&& game->mouse.x <= rect.x + rect.w
		&& game->mouse.y >= rect.y
		&& game->mouse.y <= rect.y + rect.h
	)
	{
		int alpha = 8;
		
		/* mouse press */
		if (game->mouse.press)
		{
			alpha = 2;
			SDL_EzText_SetColor(-1);
		}
		
		if (game->mouse.release)
		{
			rval = 1;
			game->mouse.release = 0;
		}
		
		assert(alpha);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255 / alpha);
		SDL_RenderFillRect(ren, &rect);
	}
	SDL_EzText(ren, *x, *y, text);
	SDL_EzText_SetColor(0x000000ff);
	
	*y += yadv;
	
	return rval;
}

/* simple checkbox
 * returns non-zero when value changes, 0 otherwise
 */
static int checkbox(struct Game *game, int *x, int *y, const char *text, int *v)
{
	char fancy[512];
	
	snprintf(fancy, sizeof(fancy) / sizeof(*fancy), "[%c] %s", *v ? 'x' : ' ', text);
	
	if (label(game, x, y, fancy))
	{
		*v = !*v;
		return 1;
	}
	
	return 0;
}

int main(void)
{
	struct EzSpriteContext *ctx;
	struct EzSprite *sprite = 0;
	struct Game game = {0};
	int running = 1;
	int x_mirror = 0;
	int y_mirror = 0;
	int grid = 1;
	#define EZSPRITESHEET_NAME simple
	#include "sprites.h"
	
	/* initialization */
	SDL_Init(SDL_INIT_EVERYTHING);
	game.SDL.window = SDL_CreateWindow(
		"EzSpriteSheet Demo (C + SDL2)"
		, SDL_WINDOWPOS_CENTERED
		, SDL_WINDOWPOS_CENTERED
		, WIN_W
		, WIN_H
		, SDL_WINDOW_SHOWN
	);
	game.SDL.renderer = SDL_CreateRenderer(
		game.SDL.window
		, -1
		, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);
	SDL_SetRenderDrawBlendMode(game.SDL.renderer, SDL_BLENDMODE_BLEND);
	ctx = game.spriteCtx = EzSpriteContext_new((struct EzSpriteContextInit){
		.udata = &game
		, .texture = {
			.load = texture_load
			, .free = texture_free
			, .draw = texture_draw
		}
		, .ticks = ticks
	});
	EzSpriteContext_addbank(ctx, &simple);
	EzSpriteContext_loaddeps(ctx);
	
	sprite = EzSprite_new(ctx);
	EzSprite_set_anim_index(sprite, 0);
	
	while (running)
	{
		SDL_Event event;
		int gw = VIEW_W / 2; /* grid dimensions = 1/2 view dimensions */
		int gh = VIEW_H / 2;
		
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					running = 0;
					break;
				
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE)
						running = 0;
					break;
				
				case SDL_MOUSEMOTION:
					game.mouse.x = event.motion.x;
					game.mouse.y = event.motion.y;
					break;
				
				case SDL_MOUSEBUTTONDOWN:
					game.mouse.press = 1;
					break;
				
				case SDL_MOUSEBUTTONUP:
					game.mouse.press = 0;
					game.mouse.release = 1;
					break;
			}
		}
		
		SDL_SetRenderDrawColor(game.SDL.renderer, 0xff, 0xff, 0xff, -1);
		SDL_RenderClear(game.SDL.renderer);
		
		/* draw grid */
		if (grid)
		{
			SDL_SetRenderDrawColor(game.SDL.renderer, 0, 0, 0, 255 / 4);
			SDL_RenderFillRect(game.SDL.renderer, &(SDL_Rect){0, 0, gw, gh});
			SDL_RenderFillRect(game.SDL.renderer, &(SDL_Rect){gw, gh, gw, gh});
		}
		
		/* draw sprite */
		EzSprite_update(sprite);
		EzSprite_set_mirror(sprite, x_mirror, y_mirror);
		EzSprite_draw(sprite, gw, gh);
		
		/* draw stats */
		{
			SDL_Renderer *ren = game.SDL.renderer;
			const char *name = EzSprite_get_anim_name(sprite);
			const char *artist = "@DynoPunch";
			int pad = 16;
			int yadv = 24;
			int xbegin = VIEW_W + pad;
			int ybegin = pad;
			int x = xbegin;
			int y = ybegin;
			
			/* animation name */
			x = xbegin;
			y = ybegin;
			SDL_EzText(ren, x, y, "Animation Name: ");
			x += SDL_EzTextLastWidth();
			SDL_EzText(ren, x, y, name);
			SDL_EzText(ren, x + 1, y, name); /* bold */
			
			/* misc settings */
			x = xbegin;
			y += yadv;
			if (label(&game, &x, &y, "[ << Previous]"))
				EzSprite_set_anim_index(sprite, EzSprite_get_anim_index(sprite) - 1);
			if (label(&game, &x, &y, "[ Next >> ]"))
				EzSprite_set_anim_index(sprite, EzSprite_get_anim_index(sprite) + 1);
			checkbox(&game, &x, &y, "X Mirror", &x_mirror);
			checkbox(&game, &x, &y, "Y Mirror", &y_mirror);
			checkbox(&game, &x, &y, "Grid", &grid);
			
			/* art attribution, lower right of window */
			/* get full attribution text length */
			x = WIN_W;
			y = WIN_H - 24;
			SDL_EzText(ren, x, y, "Sprite art courtesy of  ");
			x += SDL_EzTextLastWidth();
			SDL_EzText(ren, x, y, artist);
			x += SDL_EzTextLastWidth();
			/* now write attribution text for real */
			x = WIN_W - (x - WIN_W);
			SDL_EzText(ren, x, y, "Sprite art courtesy of ");
			x += SDL_EzTextLastWidth();
			SDL_EzText(ren, x, y, artist);
			SDL_EzText(ren, x + 1, y, artist); /* bold */
		}
		SDL_RenderPresent(game.SDL.renderer);
	}
	
	/* cleanup */
	EzSpriteContext_delete(ctx);
	SDL_DestroyRenderer(game.SDL.renderer);
	SDL_DestroyWindow(game.SDL.window);
	return 0;
}
