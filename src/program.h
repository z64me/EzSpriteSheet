#ifndef EZSPRITESHEET_PROGRAM_H_INCLUDED

#define EZSPRITESHEET_PROGRAM_H_INCLUDED

#include <stdint.h>

#define PROGNAME        "EzSpriteSheet"
#define PROGNAME_LOWER  "ezspritesheet"
#define PROGVER         "v1.0.0"
#define PROGATTRIB      "<z64.me>"

void *EzSpriteSheet_getPagePixels(
	int page
	, int *w
	, int *h
	, int *rects
	, float *occupancy
	, void progress(float unit_interval)
);
void EzSpriteSheet_setPopups(
	void die(const char *msg)
	, void complain(const char *msg)
	, void success(const char *msg)
);
int EzSpriteSheet_countPages(void);
void EzSpriteSheet_cleanup(void);
const char *EzSpriteSheet_export(
	const char *output
	, const char *scheme
	, const char *prefix
	, int longnames
	, void progress(float unit_interval)
);

const char *EzSpriteSheet(
	const char *formats
	, const char *expr
	, const char *method
	, const char *scheme
	, const char *input
	, const char *output
	, const char *logfile
	, int warnings
	, int quiet
	, int exhaustive
	, int rotate
	, int trim
	, int doubles
	, int pad
	, int visual
	, int width
	, int height
	, int negate
	, uint32_t color
	, int *totalSprites
	, int *totalDuplicates
	, void pack_progress(float unit_interval)
	, void load_progress(float unit_interval)
);

#endif /* EZSPRITESHEET_PROGRAM_H_INCLUDED */

