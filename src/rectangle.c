/*
 * rectangle.c <z64.me>
 * 
 * handles EzSpriteSheet's rectangle lists, sorting, and packing
 * 
 */

#include "common.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <RectangleBinPack/GuillotineBinPack.h>
#include <RectangleBinPack/MaxRectsBinPack.h>

/*
 * 
 * private interface
 * 
 */

struct EzSpriteSheetRect
{
	struct EzSpriteSheetRect *next;
	struct EzSpriteSheetRect *nextInPage;
	const void *udata;
	int width;
	int height;
	int x;
	int y;
	int rotated;
	int page;
};

struct EzSpriteSheetRectList
{
	struct EzSpriteSheetRect *head;
	struct EzSpriteSheetRect **page;
	int count;
	int pageCount;
	int pageMax;
	int pageWidth;
	int pageHeight;
};

struct Packer
{
	union
	{
		GuillotineBinPackC *guillotine;
		MaxRectsBinPackC *maxrects;
		void *ptr;
	} handler;
	enum EzSpriteSheetRectPack mode;
};

static void Packer_Free(struct Packer *p)
{
	assert(p);
	assert(p->handler.ptr);
	
	if (!p)
		return;
	
	/* clean up handler */
	switch (p->mode)
	{
		case EzSpriteSheetRectPack_MaxRects:
			delete_MaxRectsBinPack(p->handler.maxrects);
			break;
		case EzSpriteSheetRectPack_Guillotine:
			delete_GuillotineBinPack(p->handler.guillotine);
			break;
		default:
			die("unsupported packer");
			break;
	}
	
	p->handler.ptr = 0;
}

static int Packer_Push(struct Packer *p, struct EzSpriteSheetRect *r)
{
	RectC rv = {0};
	
	assert(p);
	
	if (!p)
		return 1;
	
	switch (p->mode)
	{
		case EzSpriteSheetRectPack_MaxRects:
			rv = MaxRectsBinPack_Insert(
				p->handler.maxrects
				, r->width
				, r->height
				, RectBestShortSideFit
			);
			break;
		case EzSpriteSheetRectPack_Guillotine:
			rv = GuillotineBinPack_Insert(
				p->handler.guillotine
				, r->width
				, r->height
				, 0
				, RectBestAreaFit
				, SplitShorterLeftoverAxis
			);
			break;
		default:
			die("unsupported packer");
			break;
	}
	
	if (!rv.width)
		return 1;
	
	if (rv.width != r->width)
		r->rotated = 1;
	else
		r->rotated = 0;
	r->x = rv.x;
	r->y = rv.y;
	r->width = rv.width;
	r->height = rv.height;
	
	return 0;
}

static void Packer_Init(struct Packer *p, enum EzSpriteSheetRectPack mode
	, int width, int height, int rotate
)
{
	assert(p);
	assert(!p->handler.ptr);
	
	if (!p)
		return;
	
	p->mode = mode;
	
	switch (mode)
	{
		case EzSpriteSheetRectPack_MaxRects:
			p->handler.maxrects = new_MaxRectsBinPack(width, height, rotate);
			break;
		case EzSpriteSheetRectPack_Guillotine:
			p->handler.guillotine = new_GuillotineBinPack(width, height, rotate);
			break;
		default:
			die("unknown packer");
			break;
	}
}

/*
 * 
 * public interface
 * 
 */

/* initialize a rectangle list */
struct EzSpriteSheetRectList *EzSpriteSheetRectList_new(void)
{
	struct EzSpriteSheetRectList *s = calloc_safe(1, sizeof(*s));
	
	s->pageMax = 64;
	s->page = calloc_safe(s->pageMax, sizeof(*s->page));
	
	return s;
}

/* push a new rectangle into a rectangle list */
struct EzSpriteSheetRect *EzSpriteSheetRectList_push(
	struct EzSpriteSheetRectList *s
	, const void *udata, int width, int height
)
{
	struct EzSpriteSheetRect *r = calloc_safe(1, sizeof(*r));
	
	assert(s);
	
	r->udata = udata;
	r->width = width;
	r->height = height;
	
	r->next = s->head;
	s->head = r;
	s->count++;
	
	return r;
}



/* sort a rectangle list (optional, but may improve packing speed/ratio)
 * adapted from the bubble sort example on GeeksforGeeks:
 * https://www.geeksforgeeks.org/c-program-bubble-sort-linked-list/
 * adapted from list_bubble_sort example here https://stackoverflow.com/a/21390410
 */
void EzSpriteSheetRectList_sort(struct EzSpriteSheetRectList *s
	, enum EzSpriteSheetRectSort mode
)
{
	struct EzSpriteSheetRect *r;
	int done = 0;
	
	if (!s || !s->head || s->count <= 1)
		return;
	
	/* XXX you will have to rerun pack() after sorting */
	for (r = s->head; r; r = r->next)
		r->nextInPage = 0;
	
	while (!done)
	{
		struct EzSpriteSheetRect **pv = &s->head;
		struct EzSpriteSheetRect *nd = s->head;
		struct EzSpriteSheetRect *nx = s->head->next;
		
		done = 1;
		
		while (nx)
		{
			struct EzSpriteSheetRect *p1 = nd;
			struct EzSpriteSheetRect *p2 = nx;
			int swap = 0;
			
			switch (mode)
			{
				case EzSpriteSheetRectSort_Area:
					if (p1->width * p1->height < p2->width * p2->height)
						swap = 1;
					break;
				case EzSpriteSheetRectSort_Height:
					if (p1->height < p2->height)
						swap = 1;
					break;
				case EzSpriteSheetRectSort_Width:
					if (p1->width < p2->width)
						swap = 1;
					break;
			}
			if (swap)
			{
				nd->next = nx->next;
				nx->next = nd;
				*pv = nx;
				done = 0;
			}
			pv = &nd->next;
			nd = nx;
			nx = nx->next;
		}
	}
	
#if 0
	/* print sorted area */
	for (struct EzSpriteSheetRect *r = s->head; r; r = r->next)
		fprintf(stderr, "area = %d\n", r->width * r->height);
	fprintf(stderr, "%d rects\n", s->count);
	exit(EXIT_FAILURE);
#endif
}

/* pack a rectangle list into a series of pages */
void EzSpriteSheetRectList_pack(struct EzSpriteSheetRectList *s
	, enum EzSpriteSheetRectPack mode
	, int width
	, int height
	, int rotate
	, int exhaustive
	, void progress(float unit_interval)
)
{
	struct EzSpriteSheetRect tail = {0};
	struct EzSpriteSheetRect *r;
	struct Packer p = {0};
	int used_exhaustive = 0;
	int num_packed = 0;
	int hasPacked = 0;
	int i;
	
	assert(s->page);
	
	if (!s || !s->head || !s->count)
		return;
	
	s->pageWidth = width;
	s->pageHeight = height;
	
	/* set each as having not been packed yet */
	for (r = s->head; r; r = r->next)
		r->nextInPage = 0;
	
	/* initialize each page in list as tail */
	s->pageCount = 0;
	for (i = 0; i < s->pageMax; ++i)
		s->page[i] = &tail;
	
	/* initialize packer */
	Packer_Init(&p, mode, width, height, rotate);
	
	while (num_packed < s->count)
	{
		for (r = s->head; r; r = r->next)
		{
	retry:
			/* rectangle already packed */
			if (r->nextInPage)
				continue;
			
			/* rectangle didn't fit */
			if (Packer_Push(&p, r))
			{
				int filled = 1;
				
				/* try smaller and smaller ones until there is a fit */
				if (exhaustive)
				{
					used_exhaustive = 1;
					
					for (r = r->next; r; r = r->next)
					{
						if (r->nextInPage)
							continue;
						
						if (!Packer_Push(&p, r))
						{
							filled = 0;
							break;
						}
					}
				}
				
				if (filled)
				{
					/* return to beginning of list to find largest unpacked rect */
					if (used_exhaustive)
					{
						r = s->head;
						used_exhaustive = 0;
					}
					
					s->pageCount++;
					if (s->pageCount >= s->pageMax)
					{
						int i;
						
						s->pageMax = s->pageCount * 2;
						s->page = realloc_safe(s->page, s->pageMax * sizeof(*s->page));
						for (i = s->pageCount; i < s->pageMax; ++i)
							s->page[i] = &tail;
					}
					
					/* reinitialize packer */
					Packer_Free(&p);
					hasPacked = 0;
					Packer_Init(&p, mode, width, height, rotate);
					
					/* try fitting this rectangle back into the page */
					goto retry;
				}
			}
			
			/* link into list */
			//fprintf(stderr, "link %p into page %d\n", r, s->pageCount);
			hasPacked = 1;
			r->page = s->pageCount;
			r->nextInPage = s->page[s->pageCount];
			s->page[s->pageCount] = r;
			
			/* report progress */
			if (progress)
				progress(((float)(num_packed++)) / s->count);
		}
	}
	
	/* clear list tails back to 0 */
	for (r = s->head; r; r = r->next)
		if (r->nextInPage == &tail)
			r->nextInPage = 0;
	
	if (hasPacked)
		s->pageCount++;
	
	//for (i = 0; i < s->pageCount; ++i)
	//	info("page[%d] head %p\n", i, s->page[i]);
	
	/* report progress complete */
	if (progress)
		progress(2);
	
	/* cleanup */
	Packer_Free(&p);
}

void EzSpriteSheetRectList_pageDebugOverlay(struct EzSpriteSheetRectList *s
	, int page, void *p_, int w, int h, uint8_t opacity
)
{
	struct EzSpriteSheetRect *r;
	uint8_t *p = p_;
	uint8_t fac = opacity;
	
	assert(p);
	assert(s);
	assert(page < s->pageCount);
	
	for (r = s->page[page]; r; r = r->nextInPage)
	{
		uint32_t c = rand() % 0xffffff;
		uint8_t *ul = p + r->y * w * 4 + r->x * 4;
		int y;
		uint8_t s[4] = { c >> 16, c >> 8, c, opacity };
		
		for (y = 0; y < r->height; ++y)
		{
			uint8_t *d = ul + y * w * 4;
			int x;
			
			/* blending */
			for (x = 0; x < r->width; ++x, d += 4)
			{
#define BLEND8(SRC, DST) (((SRC * fac) + (DST * (255 - fac))) / 255)
				/* blending equation:
				 * dstRGB = srcRGB * srcA + dstRGB * (1 - srcA)
				 * dstA = srcA + dstA * (1 - srcA)
				 */
				d[0] = BLEND8(s[0], d[0]);
				d[1] = BLEND8(s[1], d[1]);
				d[2] = BLEND8(s[2], d[2]);
				d[3] = s[3] + (d[3] * (255 - s[3])) / 255;
#undef BLEND8
			}
		}
	}
	
	UNUSED(h);
}

void EzSpriteSheetRectList_get_biggest_page(struct EzSpriteSheetRectList *s
	, int *w, int *h
)
{
	assert(s);
	assert(w);
	assert(h);
	
	/* TODO one eventual optimization might be to continually re-process
	 * the last page with increasingly smaller dimensions until nothing
	 * else fits, to minimize wasted pixels
	 */
	*w = s->pageWidth;
	*h = s->pageHeight;
}

void *EzSpriteSheetRectList_page(struct EzSpriteSheetRectList *s
	, int page
	, void *p_
	, int *w
	, int *h
	, int *rects
	, float *occupancy
	, int pad
	, int trim
	, void progress(float unit_interval)
)
{
	struct EzSpriteSheetRect *r;
	uint32_t *p = p_;
	static int copied = 0; /* num sprites copied so far */
	
	assert(s);
	assert(w);
	assert(h);
	assert(occupancy);
	assert(rects);
	assert(page < s->pageCount);
	
	/* reset counter on first page (progress bar hack) */
	if (!progress || page == 0)
		copied = 0;
	
	*w = s->pageWidth;
	*h = s->pageHeight;
	*occupancy = 0;
	*rects = 0;
	
	if (!p)
		p = malloc_safe(*w * *h * sizeof(*p));
	
	memset(p, 0, *w * *h * sizeof(*p));
	
	for (r = s->page[page]; r; r = r->nextInPage)
	{
		const struct EzSpriteSheetAnimFrame *frame = r->udata;
		const uint8_t *src8;
		const uint32_t *src32;
		int frameWidth;
		uint32_t *ul = p + r->y * *w + r->x; /* upper left dst image */
		int y;
		struct
		{
			int x;
			int y;
			int w;
			int h;
		} crop;
		
		EzSpriteSheetAnimFrame_get_crop(frame, &crop.x, &crop.y, &crop.w, &crop.h);
		frameWidth = EzSpriteSheetAnimFrame_get_width(frame);
		
		if (!trim)
		{
			crop.x = crop.y = 0;
			crop.w = frameWidth;
			crop.h = EzSpriteSheetAnimFrame_get_height(frame);
		}
		
		/* select upper left source image with respect to crop rect */
		src32 = EzSpriteSheetAnimFrame_get_pixels(frame);
		src32 += crop.y * frameWidth + crop.x;
		src8 = (void*)src32;
		
		/* reposition to account for padding */
		ul += pad * *w + pad;
		
		/* swap cropping width/height on rotated rectangles */
		if (r->rotated)
		{
			int tmp = crop.w;
			crop.w = crop.h;
			crop.h = tmp;
		}
		
		for (y = 0; y < crop.h; ++y)
		{
			uint32_t *d32 = ul + y * *w;
			uint8_t *d8 = (void*)d32;
			int x;
			
			/* rotate 90 degrees counter clockwise */
			if (r->rotated)
				for (x = 0; x < crop.w; ++x, ++d32)
					*d32 = src32[x * frameWidth + ((crop.h - 1) - y)];
			/* direct copy */
			else
				memcpy(d8, src8 + y * frameWidth * 4, crop.w * 4);
		}
		
		/* report progress */
		if (progress)
			progress(((float)(copied++)) / s->count);
		
		/* stats */
		*occupancy += crop.w * crop.h;
		*rects += 1;
	}
	
	*occupancy /= *w * *h;
	
	return p;
}

/* free a rectangle list */
void EzSpriteSheetRectList_free(struct EzSpriteSheetRectList **s)
{
	struct EzSpriteSheetRect *w;
	struct EzSpriteSheetRect *next = 0;
	struct EzSpriteSheetRectList *list;
	
	if (!s || !*s)
		return;
	
	list = *s;
	
	for (w = list->head; w; w = next)
	{
		next = w->next;
		free_safe(&w);
	}
	
	free_safe(&list->page);
	
	free_safe(s);
}

int EzSpriteSheetRectList_getPageCount(const struct EzSpriteSheetRectList *s)
{
	assert(s);
	
	return s->pageCount;
}

int EzSpriteSheetRect_get_page(const struct EzSpriteSheetRect *s)
{
	assert(s);
	
	return s->page;
}

int EzSpriteSheetRect_get_rotated(const struct EzSpriteSheetRect *s)
{
	assert(s);
	
	return s->rotated;
}

/* gets clipping rectangle of a frame graphic */
void EzSpriteSheetRect_get_crop(
	const struct EzSpriteSheetRect *s
	, int *x, int *y, int *w, int *h
)
{
	assert(s);
	assert(x);
	assert(y);
	assert(w);
	assert(h);
	
	*x = s->x;
	*y = s->y;
	*w = s->width;
	*h = s->height;
}
