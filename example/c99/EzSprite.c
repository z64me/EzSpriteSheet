/*
 * EzSprite.c <z64.me>
 * 
 * a highly minimalistic C implementation demonstrating
 * how to integrate EzSpriteSheet into a game engine
 * 
 */

#include "EzSprite.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 *
 * private types matching EzSpriteSheet's c99 output
 * (it is important that they match!)
 *
 */
struct EzSpriteSheet
{
	const char *source;
	int width;
	int height;
};

struct EzSpriteFrame
{
	int sheet;
	int x;
	int y;
	int w;
	int h;
	int ox;
	int oy;
	unsigned ms;
	int rot;
};

struct EzSpriteAnimation
{
	struct EzSpriteFrame *frame;
	const char *name;
	int frames;
	unsigned ms;
};

struct EzSpriteBank
{
	struct EzSpriteSheet *sheet;
	struct EzSpriteAnimation *animation;
	int animations;
	int sheets;
};

/*
 *
 * and then here's a smaller API built on top of those types
 *
 */
struct EzSpriteBankList
{
	struct EzSpriteBankList *next;
	const struct EzSpriteBank *bank;
	void **sheetData;
};

struct EzSpriteContext
{
	struct EzSpriteContextInit bind;
	struct EzSpriteBankList *bankList;
};

struct EzSprite
{
	struct EzSpriteContext *ctx;
	const struct EzSpriteAnimation *anim;
	const struct EzSpriteBankList *list;
	const struct EzSpriteFrame *frame;
	unsigned long start;
	unsigned anim_index;
	struct
	{
		unsigned x:1;
		unsigned y:1;
	} mirror;
};

/*
 *
 * EzSpriteContext functions
 *
 */

/* allocate a new EzSpriteContext structure */
struct EzSpriteContext *EzSpriteContext_new(struct EzSpriteContextInit i)
{
	struct EzSpriteContext *ctx = calloc(1, sizeof(*ctx));
	
	ctx->bind = i;
	
	return ctx;
}

/* free an existing EzSpriteContext structure */
void EzSpriteContext_delete(struct EzSpriteContext *ctx)
{
	struct EzSpriteBankList *next = 0;
	struct EzSpriteBankList *d;
	
	if (!ctx)
		return;
	
	for (d = ctx->bankList; d; d = next)
	{
		const struct EzSpriteBank *bank = d->bank;
		int sheets = bank->sheets;
		int i;
		next = d->next;
		
		for (i = 0; i < sheets; ++i)
			if (d->sheetData[i])
				ctx->bind.texture.free(ctx->bind.udata, d->sheetData[i]);
		
		free(d->sheetData);
		free(d);
	}
	
	free(ctx);
}

/* link an existing EzSpriteBank into an EzSpriteContext */
void EzSpriteContext_addbank(struct EzSpriteContext *ctx, void *bank)
{
	struct EzSpriteBankList *d = calloc(1, sizeof(*d));
	
	assert(ctx);
	
	d->bank = bank;
	d->next = ctx->bankList;
	ctx->bankList = d;
}

/* load dependencies (sprite sheets) for all banks */
int EzSpriteContext_loaddeps(struct EzSpriteContext *ctx)
{
	struct EzSpriteBankList *d;
	
	assert(ctx);
	
	for (d = ctx->bankList; d; d = d->next)
	{
		const struct EzSpriteBank *bank = d->bank;
		int sheets = bank->sheets;
		int i;
		
		d->sheetData = calloc(sheets, sizeof(void*));
		
		assert(d->sheetData);
		
		for (i = 0; i < sheets; ++i)
		{
			struct EzSpriteSheet *sheet = &bank->sheet[i];
			
			d->sheetData[i] = ctx->bind.texture.load(
				ctx->bind.udata
				, sheet->source
				, sheet->width
				, sheet->height
			);
			
			assert(d->sheetData[i]);
		}
	}
	
	return 0;
}

struct EzSpriteBank *EzSpriteBank_from_xml(const char *fn)
{
	/* TODO */
	(void)fn;
	
	return 0;
}

/*
 *
 * EzSprite functions
 *
 */

struct EzSprite *EzSprite_new(struct EzSpriteContext *ctx)
{
	struct EzSprite *s;
	
	assert(ctx);
	
	s = calloc(1, sizeof(*s));
	
	if (!s)
		return 0;
	
	s->ctx = ctx;
	
	return s;
}

void EzSprite_set_mirror(struct EzSprite *s, int x, int y)
{
	assert(s);
	
	s->mirror.x = x;
	s->mirror.y = y;
}

void EzSprite_set_anim(struct EzSprite *s, const char *name)
{
	struct EzSpriteContext *ctx;
	struct EzSpriteBankList *list;
	
	assert(s);
	assert(s->ctx);
	assert(name);
	
	ctx = s->ctx;
	
	for (list = ctx->bankList; list; list = list->next)
	{
		const struct EzSpriteBank *bank = list->bank;
		const struct EzSpriteAnimation *array = bank->animation;
		const struct EzSpriteAnimation *anim;
		int arrayNum = bank->animations;
		
		for (anim = array; anim < array + arrayNum; ++anim)
		{
			if (!strcmp(anim->name, name))
			{
				s->start = ctx->bind.ticks(ctx->bind.udata);
				s->frame = anim->frame;
				s->anim = anim;
				s->list = list;
				return;
			}
		}
	}
	
	fprintf(stderr, "failed to find animation '%s'\n", name);
	assert(list);
}

void EzSprite_set_anim_index(struct EzSprite *s, int index)
{
	assert(s);
	assert(s->ctx);
	
	struct EzSpriteContext *ctx = s->ctx;
	struct EzSpriteBankList *list = ctx->bankList;
	const struct EzSpriteBank *bank = list->bank;
	
	if (index < 0)
		index = bank->animations - 1;
	index %= bank->animations;
	
	s->anim_index = index;
	s->start = ctx->bind.ticks(ctx->bind.udata);
	s->anim = bank->animation + index;
	s->frame = s->anim->frame;
	s->list = list;
}

int EzSprite_get_anim_index(struct EzSprite *s)
{
	assert(s);
	
	return s->anim_index;
}

const char *EzSprite_get_anim_name(struct EzSprite *s)
{
	assert(s);
	assert(s->anim);
	
	return s->anim->name;
}

void EzSprite_update(struct EzSprite *s)
{
	const struct EzSpriteAnimation *anim;
	const struct EzSpriteFrame *frame;
	struct EzSpriteContext *ctx;
	unsigned long elapsed = 0;
	unsigned long now;
	
	assert(s);
	assert(s->ctx);
	assert(s->anim);
	
	ctx = s->ctx;
	anim = s->anim;
	
	now = ctx->bind.ticks(ctx->bind.udata);
	now -= s->start;
	now %= anim->ms;
	
	for (frame = anim->frame; frame < anim->frame + anim->frames; ++frame)
	{
		if (now >= elapsed && now <= elapsed + frame->ms)
			break;
		
		elapsed += frame->ms;
	}
	
	s->frame = frame;
}

void EzSprite_draw(struct EzSprite *s, int x, int y)
{
	const struct EzSpriteBankList *list;
	const struct EzSpriteFrame *frame;
	struct EzSpriteContext *ctx;
	int wOff;
	int hOff;
	
	assert(s);
	assert(s->ctx);
	
	ctx = s->ctx;
	list = s->list;
	frame = s->frame;
	
	/* swap mirror offset axes for rotated images */
	if (frame->rot)
	{
		wOff = frame->h;
		hOff = frame->w;
	}
	else
	{
		wOff = frame->w;
		hOff = frame->h;
	}
	
	/* x mirroring */
	if (s->mirror.x)
		x += frame->ox - wOff;
	else
		x -= frame->ox;
	
	/* y mirroring */
	if (s->mirror.y)
		y += frame->oy - hOff;
	else
		y -= frame->oy;
	
	ctx->bind.texture.draw(
		ctx->bind.udata
		, list->sheetData[frame->sheet]
		, x
		, y
		, s->mirror.x
		, s->mirror.y
		, frame->x
		, frame->y
		, frame->w
		, frame->h
		, frame->rot
	);
}

