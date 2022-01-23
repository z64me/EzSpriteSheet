/*
 * EzSprite.h <z64.me>
 * 
 * a highly minimalistic C implementation demonstrating
 * how to integrate EzSpriteSheet into a game engine
 * 
 */

#ifndef EZSPRITE_H_INCLUDED
#define EZSPRITE_H_INCLUDED

/* opaque types */
struct EzSpriteContext;
struct EzSpriteBank;
struct EzSprite;

/* initialization context; this is how the user provides a binding */
struct EzSpriteContextInit
{
	void *udata; /* main gameplay context */
	
	struct /* texture functions */
	{
		void* (*load)( /* load a texture */
			void *udata
			, const char *fn
			, int width
			, int height
		);
		void (*free)( /* free a texture */
			void *udata
			, void *texture
		);
		void (*draw)( /* display a texture to the screen */
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
		);
	} texture;
	
	/* general functions */
	unsigned long (*ticks)(void *udata); /* get elapsed game time */
};

/* context functions */
struct EzSpriteContext *EzSpriteContext_new(struct EzSpriteContextInit d);
void EzSpriteContext_addbank(struct EzSpriteContext *ctx, void *bank);
struct EzSpriteBank *EzSpriteBank_from_xml(const char *fn);
int EzSpriteContext_loaddeps(struct EzSpriteContext *ctx);
void EzSpriteContext_delete(struct EzSpriteContext *ctx);

/* sprite functions */
struct EzSprite *EzSprite_new(struct EzSpriteContext *ctx);
void EzSprite_set_anim(struct EzSprite *s, const char *name);
void EzSprite_set_anim_index(struct EzSprite *s, int index);
int EzSprite_get_anim_index(struct EzSprite *s);
const char *EzSprite_get_anim_name(struct EzSprite *s);
void EzSprite_set_mirror(struct EzSprite *s, int x, int y);
void EzSprite_draw(struct EzSprite *s, int x, int y);
void EzSprite_update(struct EzSprite *s);
void EzSprite_delete(struct EzSprite *s);

#endif /* EZSPRITE_H_INCLUDED */

