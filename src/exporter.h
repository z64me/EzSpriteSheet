/*
 * exporter.h <z64.me>
 * 
 * data exporter handler
 * 
 */

#ifndef EZSPRITESHEET_EXPORTER_H_INCLUDED
#define EZSPRITESHEET_EXPORTER_H_INCLUDED

#include <stdio.h>
#include "stb_image_write.h"

struct Exporter
{
	const char *name;
	const char *longname;
	struct
	{
		void (*begin)(int sheets, int animations, int isFirst, int isLast);
		void (*end)(int sheets, int animations, int isFirst, int isLast);
	} capsule;
	struct
	{
		void (*begin)(int index, const void *rgba, int w, int h, int isFirst, int isLast);
		void (*end)(int index, const void *rgba, int w, int h, int isFirst, int isLast);
	} sheet;
	struct
	{
		void (*begin)(const char *name, int frames, int ms, int isFirst, int isLast);
		void (*end)(const char *name, int frames, int ms, int isFirst, int isLast);
	} animation;
	struct
	{
		void (*begin)(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast);
		void (*end)(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast);
	} frame;
};

const struct Exporter *Export_begin(const char *name, const char *outfnDirty);
void Export_end(void);

#endif /* EZSPRITESHEET_EXPORTER_H_INCLUDED */

