/*
 * json.c <z64.me>
 * 
 * json export handler
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
#define OPEN_ONE { P("{\n"); ++indent; }
#define CLOSE_ONE { --indent; P((isLast) ? "}\n" : "},\n"); }
#define GENERIC_ISFIRST(X) \
	if (isFirst) \
	{ \
		P("\""X"\":\n"); \
		P("[\n"); \
		++indent; \
	}
#define GENERIC_ISLAST(COMMA) \
	if (isLast) \
	{ \
		--indent; \
		P("]\n"); \
		if (COMMA) \
			P(",\n"); \
	}

static void P(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
static void P(const char *fmt, ...)
{
	extern FILE *Exporter__out;
	FILE *out = Exporter__out;
	va_list ap;
	int i;
	
	if (!out)
		out = stdout;
	
	for (i = 0; i < indent; ++i)
		fprintf(out, "\t");
	va_start(ap, fmt);
	vfprintf(out, fmt, ap);
	va_end(ap);
}

static void capsule_begin(int sheets, int animations, int isFirst, int isLast)
{
	P("{\n");
	++indent;
	P("\"sheets\":%d,\n", sheets);
	P("\"animations\":%d,\n", animations);
	
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void capsule_end(int sheets, int animations, int isFirst, int isLast)
{
	--indent;
	P("}\n");
	
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
	GENERIC_ISFIRST("sheet");
	OPEN_ONE;
	
	snprintf(source, sizeof(source), "%s%s-%d.png", Exporter__path, Exporter__name, index);
	stbi_write_png(source, w, h, 4, rgba, w * 4);
	snprintf(source, sizeof(source), "%s-%d.png", Exporter__name, index);
	P("\"index\":%d,\n", index);
	P("\"width\":%d,\n", w);
	P("\"height\":%d,\n", h);
	P("\"source\":\"%s\"\n", source);
	
	UNUSED(isLast);
};

static void sheet_end(int index, const void *rgba, int w, int h, int isFirst, int isLast)
{
	CLOSE_ONE;
	GENERIC_ISLAST(1);
	
	UNUSED(index);
	UNUSED(rgba);
	UNUSED(w);
	UNUSED(h);
	UNUSED(isFirst);
};

static void animation_begin(const char *name, int frames, int ms, int isFirst, int isLast)
{
	GENERIC_ISFIRST("animation");
	OPEN_ONE;
	
	P("\"name\":\"%s\",\n", name);
	P("\"frames\":%d,\n", frames);
	P("\"ms\":%d,\n", ms);
	
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void animation_end(const char *name, int frames, int ms, int isFirst, int isLast)
{
	CLOSE_ONE;
	GENERIC_ISLAST(0);
	
	UNUSED(name);
	UNUSED(frames);
	UNUSED(ms);
	UNUSED(isFirst);
};

static void frame_begin(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast)
{
	GENERIC_ISFIRST("frame");
	OPEN_ONE;
	
	P("\"index\":%d,\n", index);
	P("\"sheet\":%d,\n", sheet);
	P("\"x\":%d,\n", x);
	P("\"y\":%d,\n", y);
	P("\"w\":%d,\n", w);
	P("\"h\":%d,\n", h);
	P("\"ox\":%d,\n", ox);
	P("\"oy\":%d,\n", oy);
	P("\"ms\":%d,\n", ms);
	P("\"rot\":%d\n", rot);
	
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void frame_end(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast)
{
	CLOSE_ONE;
	GENERIC_ISLAST(0);
	
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
const struct Exporter Exporter__json =
{
	.name = "json"
	, .longname = "JSON"
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
