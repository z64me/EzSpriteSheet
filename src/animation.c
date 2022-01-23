/*
 * animation.c <z64.me>
 * 
 * EzSpriteSheet's animation loading/processing functions live here
 * 
 */

#include "common.h"

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
	#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
		#define STBI_WINDOWS_UTF8
	#endif
	#define STBI_MALLOC malloc_safe
	#define STBI_REALLOC realloc_safe
	#define STBI_FREE free
#include "stb_image.h"

#include "libwebp/examples/anim_util.h"
#include "libwebp/src/webp/decode.h"
#include "libwebp/imageio/image_enc.h"
#include "libwebp/examples/unicode.h"

#define PIVOT_UNSET -1
#define CROP_UNSET   -1

struct EzSpriteSheetAnimFrame
{
	struct EzSpriteSheetAnim *anim; /* animation containing this frame */
	struct EzSpriteSheetAnimFrame *isDuplicateOf; /* duplicate image data */
	const void *udata;
	void *pixels; /* raw pixel data in rgba8888 format */
	struct
	{
		int x;
		int y;
	} pivot;
	struct
	{
		int x;
		int y;
		int w;
		int h;
	} crop;
	int ms; /* duration in milliseconds */
	int isPivotFrame;
	int isBlank;
};

struct EzSpriteSheetAnim
{
	struct EzSpriteSheetAnimList   *list;    /* list containing this anim */
	struct EzSpriteSheetAnim       *prev;    /* prev in list */
	struct EzSpriteSheetAnim       *next;    /* next in list */
	char                           *name;    /* filename */
	AnimatedImage                  *anim;    /* libwebp animated image */
	
	struct EzSpriteSheetAnimFrame  *frame;
	int                             frameCount;
	int                             width;   /* animation canvas width ... */
	int                             height;  /* ... and height */
	
	int                             foundCrop; /* solved cropping rect */
};

struct EzSpriteSheetAnimList
{
	struct EzSpriteSheetAnim *head;
	int count;
};

/*
 * 
 * list functions
 * 
 */

/* instantiate an animation list */
struct EzSpriteSheetAnimList *EzSpriteSheetAnimList_new(void)
{
	struct EzSpriteSheetAnimList *s = calloc_safe(1, sizeof(*s));
	
	return s;
}

/* instantiate an animation list */
void EzSpriteSheetAnimList_free(struct EzSpriteSheetAnimList **s)
{
	struct EzSpriteSheetAnim *item;
	struct EzSpriteSheetAnim *next = 0;
	
	if (!s || !*s)
		return;
	
	for (item = (*s)->head; item; item = next)
	{
		next = item->next;
		EzSpriteSheetAnim_free(&item);
	}
	
	free_safe(s);
}

/* add an animation to an animation list */
struct EzSpriteSheetAnim *EzSpriteSheetAnimList_push(
	struct EzSpriteSheetAnimList *list
	, struct EzSpriteSheetAnim *item
)
{
	assert(list);
	assert(item);
	
	if (!list || !item)
		return item;
	
	item->list = list;
	
	if (list->head)
		list->head->prev = item;
	item->next = list->head;
	list->head = item;
	list->count += 1;
	
	return item;
}

/* get pointer to first animation in list */
struct EzSpriteSheetAnim *EzSpriteSheetAnimList_head(
	struct EzSpriteSheetAnimList *list
)
{
	assert(list);
	
	if (!list)
		return 0;
	
	return list->head;
}

/* determine pivot of each in list; if color == 0, pivots are cleared */
int EzSpriteSheetAnimList_each_findPivot(struct EzSpriteSheetAnimList *list
	, const uint32_t color
)
{
	struct EzSpriteSheetAnim *anim;
	
	assert(list);
	
	if (!list)
		return 1;
	
	if (EzSpriteSheetAnimList_each_clearPivot(list))
		return 1;
	
	/* color == 0, so don't solve for new pivot values */
	if (!color)
		return 0;
	
	for (anim = list->head; anim; anim = anim->next)
		if (EzSpriteSheetAnim_findPivot(anim, color))
			return 1;
	
	return 0;
}

/* clear pivot of each in list */
int EzSpriteSheetAnimList_each_clearPivot(struct EzSpriteSheetAnimList *list)
{
	struct EzSpriteSheetAnim *anim;
	
	assert(list);
	
	if (!list)
		return 1;
	
	for (anim = list->head; anim; anim = anim->next)
		if (EzSpriteSheetAnim_clearPivot(anim))
			return 1;
	
	return 0;
}

/* get cropping rectangle of each in list */
int EzSpriteSheetAnimList_each_findCrop(struct EzSpriteSheetAnimList *list)
{
	struct EzSpriteSheetAnim *anim;
	
	assert(list);
	
	if (!list)
		return 1;
	
	for (anim = list->head; anim; anim = anim->next)
	{
		/* skip animations which crop has already been solved for */
		if (anim->foundCrop)
			continue;
		
		if (EzSpriteSheetAnim_findCrop(anim))
			return 1;
		
		anim->foundCrop = 1;
	}
	
	return 0;
}

int EzSpriteSheetAnimFrame_findDuplicates(struct EzSpriteSheetAnimFrame *frame)
{
	struct EzSpriteSheetAnimList *list;
	struct EzSpriteSheetAnim *anim;
	uint32_t *frameUL;
	int frameWidth;
	int stride;
	
	assert(frame);
	assert(frame->anim);
	assert(frame->anim->list);
	
	if (!frame || !frame->anim || !frame->anim->list)
		return 1;
	
	/* already processed */
	if (frame->isDuplicateOf)
		return 0;
	
	/* empty */
	if (frame->crop.w <= 0 || frame->crop.h <= 0)
		return 0;
	
	/* upper left of pixel data */
	frameWidth = frame->anim->width;
	frameUL = frame->pixels;
	frameUL += frame->crop.y * frameWidth + frame->crop.x;
	
	/* length of a row of pixels in bytes */
	stride = frame->crop.w * 4;
	
	/* step through every animation in list */
	list = frame->anim->list;
	for (anim = list->head; anim; anim = anim->next)
	{
		struct EzSpriteSheetAnimFrame *comp;
		int compWidth = anim->width;
		
		/* step through every frame in every animation */
		for (comp = anim->frame; comp < anim->frame + anim->frameCount; ++comp)
		{
			uint32_t *compUL;
			int y;
			
			/* cannot be duplicate of self */
			if (comp == frame)
				continue;
			
			/* cannot be duplicate of duplicate */
			if (comp->isDuplicateOf)
				continue;
			
			/* cannot be duplicate of control/blank frame */
			if (comp->isPivotFrame || comp->isBlank)
				continue;
			
			/* cannot be duplicate if cropping rectangle sizes differ */
			if (comp->crop.w != frame->crop.w || comp->crop.h != frame->crop.h)
				continue;
			
			/* get upper left of pixel data */
			compUL = comp->pixels;
			compUL += comp->crop.y * compWidth + comp->crop.x;
			
			/* compare every row */
			for (y = 0; y < frame->crop.h; ++y)
				if (memcmp(compUL + y * compWidth, frameUL + y * frameWidth, stride))
					break;
			
			/* every row matched */
			if (y == frame->crop.h)
			{
				/*fprintf(stderr, "dup found frame %d == %d\n"
					, (int)(comp - anim->frame), (int)(frame - frame->anim->frame)
				);*/
				frame->isDuplicateOf = comp;
				return 0;
			}
		}
	}
	
	return 0;
}

/* find potential duplicates of each in list */
int EzSpriteSheetAnimList_each_findDuplicates(struct EzSpriteSheetAnimList *list)
{
	struct EzSpriteSheetAnim *anim;
	
	assert(list);
	
	if (!list)
		return 1;
	
	for (anim = list->head; anim; anim = anim->next)
	{
		struct EzSpriteSheetAnimFrame *w;
		
		for (w = anim->frame; w < anim->frame + anim->frameCount; ++w)
			if (EzSpriteSheetAnimFrame_findDuplicates(w))
				return 1;
	}
	
	return 0;
}

/* clear known duplicates of each in list */
int EzSpriteSheetAnimList_each_clearDuplicates(struct EzSpriteSheetAnimList *list)
{
	struct EzSpriteSheetAnim *anim;
	
	assert(list);
	
	if (!list)
		return 1;
	
	for (anim = list->head; anim; anim = anim->next)
	{
		struct EzSpriteSheetAnimFrame *w;
		
		for (w = anim->frame; w < anim->frame + anim->frameCount; ++w)
			w->isDuplicateOf = 0;
	}
	
	return 0;
}

/*
 * 
 * item functions
 * 
 */

/* load image from file */
struct EzSpriteSheetAnim *EzSpriteSheetAnim_new(const char *fn)
{
	struct EzSpriteSheetAnim *s = calloc_safe(1, sizeof(*s));
	int i;
	
	assert(fn);
	
	if (!fn)
		return 0;
	
	s->name = strdup_safe(fn);
	
	if (file_is_extension(fn, "webp") || file_is_extension(fn, "gif"))
	{
		W_CHAR *wfn = char2wchar(fn);
		s->anim = calloc_safe(1, sizeof(*s->anim));
		AnimatedImage *anim = s->anim;
		
		if (!ReadAnimatedImage((const char*)wfn, s->anim, 0, NULL))
		{
			die("Error decoding file: %s", fn);
			return 0;
		}
		
		char2wchar_free(&wfn);
		
		/* unified animation frame format */
		s->frameCount = s->anim->num_frames;
		s->frame = calloc_safe(s->frameCount, sizeof(*s->frame));
		for (i = 0; i < s->frameCount; ++i)
		{
			s->frame[i].pixels = anim->frames[i].rgba;
			s->frame[i].ms = anim->frames[i].duration;
		}
		s->width = anim->canvas_width;
		s->height = anim->canvas_height;
	}
	else
	{
		void *data;
		int c;
		
		data = stbi_load(fn, &s->width, &s->height, &c, STBI_rgb_alpha);
		if (!data)
		{
			die("Error reading or decoding file: %s", fn);
			return 0;
		}
		
		/* unified animation frame format */
		s->frameCount = 1;
		s->frame = calloc_safe(s->frameCount, sizeof(*s->frame));
		s->frame[0].pixels = data;
		s->frame[0].ms = 1;
	}
	
	/* initialize animation frame data */
	for (i = 0; i < s->frameCount; ++i)
	{
		struct EzSpriteSheetAnimFrame *f = s->frame + i;
		uint8_t *pix8 = f->pixels;
		int k;
		
		f->anim = s;
		
		/* mark uninitialized */
		f->pivot.x = PIVOT_UNSET;
		f->crop.x = CROP_UNSET;
		
		/* set invisible pixels of each frame to all one color */
		for (k = 0; k < s->width * s->height; ++k, pix8 += 4)
			if (!pix8[3])
				*(uint32_t*)pix8 = 0;
	}
	
	return s;
}

/* unlink one animation from the list it's in */
void EzSpriteSheetAnim_unlink(struct EzSpriteSheetAnim *a)
{
	/* unlink from list */
	struct EzSpriteSheetAnimList *list;
	struct EzSpriteSheetAnim *prev;
	struct EzSpriteSheetAnim *next;
	
	list = a->list;
	prev = a->prev;
	next = a->next;
	
	assert(list);
	
	/* visual aid for the unlinking process
	 *       /-----------v
	 *     prev -> a -> next
	 *       ^-----------/
	 * (note how a's link is bypassed)
	 */
	
	/* at list head */
	if (!prev)
		list->head = a->next;
	/* ordinary link in list */
	else
		prev->next = a->next;
	if (next)
		next->prev = prev;
	
	list->count -= 1;
}

/* free a struct allocated using EzSpriteSheetAnim_new */
void EzSpriteSheetAnim_free(struct EzSpriteSheetAnim **s)
{
	struct EzSpriteSheetAnim *a;
	
	if (!s || !*s)
		return;
	
	a = *s;
	
	EzSpriteSheetAnim_unlink(a);
	
	if (a->name)
		free_safe(&a->name);
	
	/* libwebp cleanup */
	if (a->anim)
	{
		ClearAnimatedImage(a->anim);
		free_safe(&a->anim);
	}
	/* stb_image cleanup */
	else
		stbi_image_free(a->frame[0].pixels);
	
	free_safe(&a->frame);
	
	free_safe(s);
}

/* get pointer to next animation in a list */
struct EzSpriteSheetAnim *EzSpriteSheetAnim_get_next(
	struct EzSpriteSheetAnim *anim
)
{
	assert(anim);
	
	if (!anim)
		return 0;
	
	return anim->next;
}

/* get details about each animation frame (for use in a loop)
 * returns non-zero if a frame is fetched properly
 * use it like this:
 * while ((frame = EzSpriteSheetAnim_each_frame(anim)))
 *   do_stuff();
 */
struct EzSpriteSheetAnimFrame *EzSpriteSheetAnim_each_frame(
	struct EzSpriteSheetAnim *s
)
{
	static struct EzSpriteSheetAnim *prev = 0;
	static int iter = 0;
	
	/* reset iterator on new animation */
	if (prev != s)
	{
		iter = 0;
		prev = s;
	}
	
	/* frame limit exceeded */
	if (iter >= s->frameCount)
	{
		iter = 0;
		return 0;
	}
	
	return s->frame + (iter++);
}

/* clear pivot of all frames within one animation */
int EzSpriteSheetAnim_clearPivot(struct EzSpriteSheetAnim *s)
{
	int i;
	
	assert(s);
	
	if (!s)
		return 1;
	
	for (i = 0; i < s->frameCount; ++i)
	{
		struct EzSpriteSheetAnimFrame *f = s->frame + i;
		
		f->pivot.x = PIVOT_UNSET;
		f->isPivotFrame = 0;
	}
	
	return 0;
}

/* determine pivot of all frames within one animation */
int EzSpriteSheetAnim_findPivot(struct EzSpriteSheetAnim *s
	, const uint32_t color
)
{
	int i;
	
	assert(s);
	
	if (!s)
		return 1;
	
	/* non-animated images can't use this feature */
	if (s->frameCount == 1)
		return 0;
	
	/* optimization: only the last frame can be an pivot frame
	 * (this is now part of the spec)
	 */
	for (i = s->frameCount - 1; i < s->frameCount; ++i)
	{
		struct EzSpriteSheetAnimFrame *f = s->frame + i;
		const uint32_t *pix32 = f->pixels;
		union {
			uint8_t rgba[4];
			uint32_t word;
		} c;
		int frameWidth = s->width;
		int y;
		
		/* optimization: scan only the cropping rectangle */
		pix32 += f->crop.y * frameWidth + f->crop.x;
		
		c.rgba[0] = color >> 16;
		c.rgba[1] = color >> 8;
		c.rgba[2] = color;
		c.rgba[3] = -1;
		
		f->pivot.x = PIVOT_UNSET;
		
		for (y = 0; y < f->crop.h; ++y, pix32 += frameWidth)
		{
			int x;
			
			for (x = 0; x < f->crop.w; ++x)
			{
				int j;
				
				/* skip pixels that don't match */
				if (pix32[x] != c.word)
					continue;
				
				/* pivot was already determined from another pixel */
				if (f->pivot.x != PIVOT_UNSET)
				{
					complain(
						"'%s' frame %d/%d has multiple pixels of pivot color #%06x!"
						, s->name
						, i + 1
						, s->frameCount
						, color
					);
					return 1;
				}
				f->pivot.x = x + f->crop.x;
				f->pivot.y = y + f->crop.y;
				f->isPivotFrame = 1;
				
				/* pivot pixel specifies pivot of all preceding frames */
				for (j = i - 1; j >= 0; --j)
				{
					struct EzSpriteSheetAnimFrame *prev = s->frame + j;
					
					/* accounts for multiple pivot frames */
					if (prev->pivot.x != PIVOT_UNSET)
						break;
					
					prev->pivot.x = f->pivot.x;
					prev->pivot.y = f->pivot.y;
				}
			}
		}
	}
	
	return 0;
}

/* determine cropping rectangle of all frames within one animation */
int EzSpriteSheetAnim_findCrop(struct EzSpriteSheetAnim *s)
{
	int i;
	
	assert(s);
	
	if (!s)
		return 1;
	
	for (i = 0; i < s->frameCount; ++i)
	{
		struct EzSpriteSheetAnimFrame *f = s->frame + i;
		const uint32_t *pix32 = f->pixels;
		int pixNum = s->width * s->height;
		int upper = -1;
		int lower = -1;
		int left = s->width;
		int right = -1;
		int k;
		
		f->crop.x = CROP_UNSET;
		f->isBlank = 0;
		
		/* get uppermost pixel */
		for (k = 0; k < pixNum; ++k)
			if (pix32[k])
			{
				upper = k / s->width;
				break;
			}
		
		/* optimization: blank frame */
		if (k == pixNum)
		{
			f->isBlank = 1;
			continue;
		}
		
		/* get lowermost pixel */
		for (k = pixNum - 1; k >= 0; --k)
			if (pix32[k])
			{
				lower = (k / s->width) + 1;
				break;
			}
		
		/* get leftmost and rightmost pixels */
		for (k = upper * s->width; k < lower * s->width; ++k)
		{
			int x;
			
			if (!pix32[k])
				continue;
			
			x = k % s->width;
			
			if (x < left)
				left = x;
			
			if (x >= right)
				right = x + 1;
		}
		
		assert(upper >= 0 && upper <= s->height);
		assert(lower >= 0 && lower <= s->height);
		assert(right >= 0 && right <= s->width);
		assert(left  >= 0 && left  <= s->width);
		
		f->crop.x = left;
		f->crop.y = upper;
		f->crop.w = right - left;
		f->crop.h = lower - upper;
	}
	
	return 0;
}

int EzSpriteSheetAnimFrame_get_isPivotFrame(
	const struct EzSpriteSheetAnimFrame *frame
)
{
	assert(frame);
	
	return frame->isPivotFrame;
}

int EzSpriteSheetAnimFrame_get_isBlank(
	const struct EzSpriteSheetAnimFrame *frame
)
{
	assert(frame);
	
	return frame->isBlank;
}

const struct EzSpriteSheetAnimFrame *EzSpriteSheetAnimFrame_get_isDuplicateOf(
	const struct EzSpriteSheetAnimFrame *frame
)
{
	assert(frame);
	
	/* duplicates can be duplicates, so resolve those cases */
	do
		frame = frame->isDuplicateOf;
	while (frame && frame->isDuplicateOf);
	
	return frame;
}

/* gets clipping rectangle of a frame graphic */
void EzSpriteSheetAnimFrame_get_crop(
	const struct EzSpriteSheetAnimFrame *frame
	, int *x, int *y, int *w, int *h
)
{
	assert(frame);
	assert(x);
	assert(y);
	assert(w);
	assert(h);
	
	*x = frame->crop.x;
	*y = frame->crop.y;
	*w = frame->crop.w;
	*h = frame->crop.h;
}

/* get pivot point of a frame graphic */
void EzSpriteSheetAnimFrame_get_pivot(
	const struct EzSpriteSheetAnimFrame *frame
	, int *x, int *y
)
{
	assert(frame);
	assert(x);
	assert(y);
	
	*x = frame->pivot.x;
	*y = frame->pivot.y;
}

void EzSpriteSheetAnimFrame_set_udata(struct EzSpriteSheetAnimFrame *frame
	, const void *udata
)
{
	assert(frame);
	
	frame->udata = udata;
}

const void *EzSpriteSheetAnimFrame_get_udata(const struct EzSpriteSheetAnimFrame *s)
{
	assert(s);
	
	return s->udata;
}

const char *EzSpriteSheetAnim_get_name(const struct EzSpriteSheetAnim *s)
{
	assert(s);
	
	return s->name;
}

int EzSpriteSheetAnimFrame_get_width(const struct EzSpriteSheetAnimFrame *frame)
{
	assert(frame);
	
	return frame->anim->width;
}

int EzSpriteSheetAnimFrame_get_height(const struct EzSpriteSheetAnimFrame *frame)
{
	assert(frame);
	
	return frame->anim->height;
}

const void *EzSpriteSheetAnimFrame_get_pixels(
	const struct EzSpriteSheetAnimFrame *frame
)
{
	assert(frame);
	
	return frame->pixels;
}

int EzSpriteSheetAnimFrame_get_ms(
	const struct EzSpriteSheetAnimFrame *frame
)
{
	assert(frame);
	
	return frame->ms;
}

int EzSpriteSheetAnim_countRealFrames(const struct EzSpriteSheetAnim *anim)
{
	int count = 0;
	int i;
	
	assert(anim);
	
	for (i = 0; i < anim->frameCount; ++i)
	{
		const struct EzSpriteSheetAnimFrame *frame = anim->frame + i;
		
		/* skip control frames */
		if (frame->isPivotFrame)
			continue;
		
		++count;
	}
	
	/* corner case: an animation containing only control frames */
	if (!count)
		count = 1;
	
	return count;
}

int EzSpriteSheetAnim_get_loopMs(const struct EzSpriteSheetAnim *anim)
{
	int ms = 0;
	int i;
	
	assert(anim);
	
	for (i = 0; i < anim->frameCount; ++i)
	{
		const struct EzSpriteSheetAnimFrame *frame = anim->frame + i;
		
		/* skip control frames */
		if (frame->isPivotFrame)
			continue;
		
		ms += frame->ms;
	}
	
	return ms;
}

int EzSpriteSheetAnimList_get_count(const struct EzSpriteSheetAnimList *list)
{
	assert(list);
	
	return list->count;
}

const struct EzSpriteSheetAnimFrame *EzSpriteSheetAnim_get_lastframe(
	const struct EzSpriteSheetAnim *anim
)
{
	const struct EzSpriteSheetAnimFrame *last = 0;
	int i;
	
	assert(anim);
	
	for (i = 0; i < anim->frameCount; ++i)
	{
		const struct EzSpriteSheetAnimFrame *frame = anim->frame + i;
		
		/* skip control frames */
		if (frame->isPivotFrame)
			continue;
		
		last = frame;
	}
	
	return last;
}
