/*
 * c99.c <z64.me>
 * 
 * c99 export handler
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
#define GENERIC_ISFIRST(X, Z) \
	if (isFirst) \
	{ \
		P("(struct "#Z"[])\n"); \
		P("{\n"); \
		++indent; \
	}
#define GENERIC_ISLAST(COMMA) \
	if (isLast) \
	{ \
		--indent; \
		if (COMMA) \
			P("},\n"); \
		else \
			P("}\n"); \
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
	const char *template =
"#ifndef EZSPRITESHEET_NAME /* a custom name for the output bank */\n\
#define EZSPRITESHEET_NAME EzSpriteBank_main /* default name */\n\
#endif\n\
\n\
#ifndef EZSPRITESHEET_TYPES\n\
#define EZSPRITESHEET_TYPES\n\
struct EzSpriteSheet\n\
{\n\
	const char *source;\n\
	int width;\n\
	int height;\n\
};\n\
\n\
struct EzSpriteFrame\n\
{\n\
	int sheet;\n\
	int x;\n\
	int y;\n\
	int w;\n\
	int h;\n\
	int ox;\n\
	int oy;\n\
	unsigned ms;\n\
	int rot;\n\
};\n\
\n\
struct EzSpriteAnimation\n\
{\n\
	struct EzSpriteFrame *frame;\n\
	const char *name;\n\
	int frames;\n\
	unsigned ms;\n\
};\n\
\n\
struct EzSpriteBank\n\
{\n\
	struct EzSpriteSheet *sheet;\n\
	struct EzSpriteAnimation *animation;\n\
	int animations;\n\
	int sheets;\n\
};\n\
#endif /* EZSPRITESHEET_TYPES */\n\
\n\
struct EzSpriteBank EZSPRITESHEET_NAME =";
	P("%s\n", template);
	P("{\n");
	++indent;
	
	UNUSED(isFirst);
	UNUSED(isLast);
	UNUSED(sheets);
	UNUSED(animations);
};

static void capsule_end(int sheets, int animations, int isFirst, int isLast)
{
	P("%d,\n", animations);
	P("%d\n", sheets);
	--indent;
	P("};\n\n");
	
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void sheet_begin(int index, const void *rgba, int w, int h, int isFirst, int isLast)
{
	extern char *Exporter__path;
	extern char *Exporter__name;
	char source[1024];
	
	GENERIC_ISFIRST("sheet", EzSpriteSheet);
	OPEN_ONE;
	
	snprintf(source, sizeof(source), "%s%s-%d.png", Exporter__path, Exporter__name, index);
	stbi_write_png(source, w, h, 4, rgba, w * 4);
	snprintf(source, sizeof(source), "%s-%d.png", Exporter__name, index);
	P("\"%s\",\n", source);
	P("%d,\n", w);
	P("%d\n", h);
	
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
	GENERIC_ISFIRST("animation", EzSpriteAnimation);
	OPEN_ONE;
	
	UNUSED(isFirst);
	UNUSED(isLast);
	UNUSED(frames);
	UNUSED(name);
	UNUSED(ms);
};

static void animation_end(const char *name, int frames, int ms, int isFirst, int isLast)
{
	P("\"%s\",\n", name);
	P("%d,\n", frames);
	P("%d\n", ms);
	
	CLOSE_ONE;
	
	GENERIC_ISLAST(1);
	
	UNUSED(isFirst);
};

static void frame_begin(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast)
{
	GENERIC_ISFIRST("frame", EzSpriteFrame);
	OPEN_ONE;
	
	P("%d,\n", sheet);
	P("%d,\n", x);
	P("%d,\n", y);
	P("%d,\n", w);
	P("%d,\n", h);
	P("%d,\n", ox);
	P("%d,\n", oy);
	P("%d,\n", ms);
	P("%d\n", rot);
	
	UNUSED(index);
	UNUSED(isFirst);
	UNUSED(isLast);
};

static void frame_end(int index, int sheet, int x, int y, int w, int h, int ox, int oy, int ms, int rot, int isFirst, int isLast)
{
	CLOSE_ONE;
	GENERIC_ISLAST(1);
	
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
const struct Exporter Exporter__c99 =
{
	.name = "c99"
	, .longname = "C99 Header"
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

