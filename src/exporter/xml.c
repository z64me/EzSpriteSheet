/*
 * xml.c <z64.me>
 * 
 * xml export handler
 * 
 */

#include "../exporter.h"

#include <stdarg.h>

/*
 * 
 * private interface
 * 
 */
static int indent = 0;

#define UNUSED(X) (void)X;

static void P(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
static void P(const char *fmt, ...)
{
	extern FILE *Exporter__out;
	FILE *out = Exporter__out;
	va_list ap;
	int i;
	
	if (!out)
		out = stdout;
	
	if (*fmt == '<')
		for (i = 0; i < indent; ++i)
			fprintf(out, "\t");
	va_start(ap, fmt);
	vfprintf(out, fmt, ap);
	va_end(ap);
}

static void capsule_begin(int sheets, int animations, int isFirst, int isLast)
{
	P("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	P("<ezspritebank sheets=\"%d\" animations=\"%d\">\n", sheets, animations);
	++indent;
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void capsule_end(int sheets, int animations, int isFirst, int isLast)
{
	--indent;
	P("</ezspritebank>\n");
	UNUSED(sheets);
	UNUSED(animations);
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void sheet_begin(int index, const void *rgba, int w, int h, int isFirst, int isLast)
{
	extern char *Exporter__path;
	extern char *Exporter__name;
	char source[1024];
	
	snprintf(source, sizeof(source), "%s%s-%d.png", Exporter__path, Exporter__name, index);
	stbi_write_png(source, w, h, 4, rgba, w * 4);
	snprintf(source, sizeof(source), "%s-%d.png", Exporter__name, index);
	P("<sheet index=\"%d\" width=\"%d\" height=\"%d\" source=\"%s\"", index, w, h, source);
	++indent;
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void sheet_end(int index, const void *rgba, int w, int h, int isFirst, int isLast)
{
	--indent;
	P("/>\n");
	UNUSED(index);
	UNUSED(rgba);
	UNUSED(w);
	UNUSED(h);
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void animation_begin(const char *name, int frames, int ms, int isFirst, int isLast)
{
	P("<animation name=\"%s\" frames=\"%d\" ms=\"%d\">\n", name, frames, ms);
	++indent;
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void animation_end(const char *name, int frames, int ms, int isFirst, int isLast)
{
	--indent;
	P("</animation>\n");
	UNUSED(name);
	UNUSED(frames);
	UNUSED(ms);
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void frame_begin(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast)
{
	P("<frame index=\"%d\" sheet=\"%d\" x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\" ox=\"%d\" oy=\"%d\" ms=\"%d\" rot=\"%d\""
		, index, sheet, x, y, w, h, ox, oy, ms, rot
	);
	++indent;
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void frame_end(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast)
{
	--indent;
	P("/>\n");
	UNUSED(index);
	UNUSED(sheet);
	UNUSED(x);
	UNUSED(y);
	UNUSED(w);
	UNUSED(h);
	UNUSED(ox);
	UNUSED(oy);
	UNUSED(ms);
	UNUSED(rot);
	UNUSED(isFirst);
	UNUSED(isLast);
};

/*
 * 
 * public binding
 * 
 */
const struct Exporter Exporter__xml =
{
	.name = "xml"
	, .longname = "XML"
	, .capsule =
	{
		.begin = capsule_begin
		, .end = capsule_end
	}
	, .sheet =
	{
		.begin = sheet_begin
		, .end = sheet_end
	}
	, .animation =
	{
		.begin = animation_begin
		, .end = animation_end
	}
	, .frame =
	{
		.begin = frame_begin
		, .end = frame_end
	}
};
