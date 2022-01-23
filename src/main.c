#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "program.h"
#include "common.h" /* logfile, info, die */

static int quiet = 0;

/* generic command line progress bar */
static void progress_generic(float unit_interval, const char *name)
{
	int width = 32;
	int progress = unit_interval * width;
	int i;
	
	if (quiet)
		return;
	
	fprintf(stderr, "\r" "%-12s [", name);
	
	for (i = 0; i < width; ++i)
		fprintf(stderr, "%c", (i < progress) ? '=' : ' ');
	
	fprintf(stderr, "]");
	
	if (progress > width)
		fprintf(stderr, "  Done!\n");
}

/* progress bar for packing progress */
static void progress_pack(float unit_interval)
{
	progress_generic(unit_interval, "Packing");
}

/* progress bar for exporting progress */
static void progress_export(float unit_interval)
{
	progress_generic(unit_interval, "Exporting");
}

/* print decorative program title */
static void title(FILE *stream, const char *str)
{
	int i;
	int len = strlen(str);
	FILE *p = stream;
	
	fprintf(p, "/");
	for (i = 0; i < len + 4; ++i)
		fprintf(p, "*");
	fprintf(p, "\n * %s *\n ", str);
	for (i = 0; i < len + 4; ++i)
		fprintf(p, "*");
	fprintf(p, "/\n");
}

static int usage(FILE *stream, int argc)
{
#define P(X) fprintf(stream, X "\n")
	P("--Necessary Arguments--------------------------------------------------");
	P("  -i, --input     specify input directory where assets are organized");
	P("  -o, --output    specify output image database file");
	P("  -s, --scheme    select which export scheme to use");
	P("                    supported export schemes:");
	P("                      xml");
	P("                      json");
	P("                      c99");
	P("  -m, --method    select packing method");
	P("                    supported packing methods:");
	P("                      guillotine  (fastest, worst)");
	//P("                      wowrects");
	P("                      maxrects    (slowest, best)");
	P("  -a, --area      specify area of generated sprite sheet");
	P("                  width by height, e.g. --area 1024x1024");
	P("--Optional Arguments----------------------------------------------------");
	P("  -h, --help      prints this usage information");
	P("  -e, --exhaust   exhaustive packing (doesn't consider a sprite");
	P("                  sheet ready until it has tried fitting every");
	P("                  remaining unpacked sprite into it)");
	P("  -r, --rotate    rotate sprites when doing so saves space");
	P("  -t, --trim      trim excess pixels from around sprites");
	P("  -d, --doubles   detect and omit duplicate sprites (doubles)");
	P("  -b, --border    add padding around each packed sprite");
	P("                  e.g. --border 8 (for 8 pixels)");
	P("  -c, --color     treat pixels matching hex color as animation pivots");
	P("                  e.g. --color 00ff00");
	P("                  (complains if multiple possible matches are found)");
	P("  -f, --formats   include only images of formats in comma-delimited");
	P("                  list, e.g. --formats \"gif,webp,png\"");
	P("  -p, --prefix    append a prefix to each sprite's name when exporting");
	P("                  e.g. --prefix \"extras/\"");
	P("  -z, --long      use each sprite's long name during export");
	P("                  e.g. write 'player/walk.gif' instead of 'player/walk'");
	P("  -x, --regex     include only images with a file path matching");
	P("                  an optional POSIX regular expression");
	P("  -n, --negate    include only images with a file path NOT matching");
	P("                  the provided --regex pattern");
	P("  -v, --visual    visualize sprite boundaries (debug feature)");
	P("                  (makes each sprite's background a random color)");
	P("  -l, --log       specify log file (stderr is used otherwise)");
	P("  -w, --warnings  log only errors and warnings");
	P("  -q, --quiet     don't log anything");
#ifdef _WIN32 /* give a hint to Windows users who double-clicked... */
	if (argc == 1)
	{
		P("");
		P("This application is meant to be run from the command line.");
		P("If you are looking for the graphical interface, try the");
		P("other program in this package. Press Return/Enter to exit.");
		getchar();
	}
#endif
#undef P
	(void)argc;
	
	return 1;
}

#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* converts wchar to utf8 */
static char *wstr2utf8(const void *wstr)
{
	if (!wstr)
		return 0;
#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
	const void *src = wstr;
	char *out = 0;
	size_t src_length = 0;
	int length;
	
	src_length = wcslen(src);
	length = WideCharToMultiByte(CP_UTF8, 0, src, src_length,
			0, 0, NULL, NULL);
	out = (malloc)((length+1) * sizeof(char));
	if (out) {
		WideCharToMultiByte(CP_UTF8, 0, src, src_length,
				out, length, NULL, NULL);
		out[length] = '\0';
	}
	return out;
#else
    return strdup(wstr);
#endif
}

/* argument abstraction: converts argv[] from wchar to char win32 */
static void *wow_conv_args(int argc, void *argv[])
{
	int i;
	for (i = 0; i < argc; ++i)
		argv[i] = wstr2utf8(argv[i]);
	return argv;
}

int wmain(int argc, wchar_t *Wargv[])
#else
int main(int argc, char *argv[])
#endif
{
	/* variables representing arguments, initialized with default values */
	const char *formats = "gif,webp,png";
	const char *expr = 0;
	const char *method = 0;
	const char *scheme = 0;
	const char *input = 0;
	const char *output = 0;
	const char *logfile = 0;
	const char *prefix = 0;
	int warnings = 0;
	int exhaustive = 0;
	int rotate = 0;
	int trim = 0;
	int doubles = 0;
	int pad = 0;
	int visual = 0;
	int width = 0;
	int height = 0;
	int negate = 0;
	int longnames = 0;
	uint32_t color = 0;
	/* misc */
	const char *errstr = 0;
	int i;
	int totalSprites;
	int totalDuplicates;
	
	#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
	char **argv = wow_conv_args(argc, (void*)Wargv);
	#endif
	
	logfile_set(stderr);
	
	/* quick check for --quiet and --warnings */
	for (i = 1; i < argc; ++i)
	{
		if (!strcasecmp(argv[i], "-q")
			|| !strcasecmp(argv[i], "--quiet")
		)
			quiet = 1;
		
		else if (!strcasecmp(argv[i], "-w")
			|| !strcasecmp(argv[i], "--warnings")
		)
			warnings = 1;
	}
	
	if (!quiet && !warnings)
		title(stderr, PROGNAME " " PROGVER " " PROGATTRIB);
	
	/* display usage information and exit */
	if (argc == 1)
		return usage(stderr, argc);
	
	/* complain about duplicate arguments/parameters
	 * (in case user accidentally provides two packing methods, etc)
	 */
	for (i = 0; i < argc; ++i)
	{
		int k;
		
		for (k = 0; k < argc; ++k)
		{
			/* do not test against self */
			if (k == i)
				continue;
			
			if (!strcasecmp(argv[i], argv[k]))
				die("duplicate argument or parameter '%s'", argv[i]);
		}
	}
	
	/* step through arguments */
	for (i = 1; i < argc; ++i)
	{
#define ARGMATCH(ALIAS, NAME) (!strcasecmp(this, "-" ALIAS) \
 || !strcasecmp(this, "--" NAME))
		const char *this = argv[i];
		const char *param = argv[i + 1];
		
	/* arguments not requiring additional parameters */
		
		/* display usage information and exit */
		if (ARGMATCH("h", "help")) return usage(stderr, argc);
		else if (ARGMATCH("r", "rotate")) { rotate = 1; continue; }
		else if (ARGMATCH("t", "trim")) { trim = 1; continue; }
		else if (ARGMATCH("d", "doubles")) { doubles = 1; continue; }
		else if (ARGMATCH("v", "visual")) { visual = 1; continue; }
		else if (ARGMATCH("q", "quiet")) { quiet = 1; continue; }
		else if (ARGMATCH("w", "warnings")) { warnings = 1; continue; }
		else if (ARGMATCH("e", "exhaust")) { exhaustive = 1; continue; }
		else if (ARGMATCH("n", "negate")) { negate = 1; continue; }
		else if (ARGMATCH("z", "long")) { longnames = 1; continue; }
		
	/* arguments requiring additional parameters */
		
		if (!param)
			die("argument '%s' missing parameter (run --help)", this);
		
		if (ARGMATCH("i", "input")) input = param;
		else if (ARGMATCH("o", "output")) output = param;
		else if (ARGMATCH("s", "scheme")) scheme = param;
		else if (ARGMATCH("m", "method")) method = param;
		else if (ARGMATCH("l", "log")) logfile = param;
		else if (ARGMATCH("f", "formats")) formats = param;
		else if (ARGMATCH("x", "regex")) expr = param;
		else if (ARGMATCH("p", "prefix")) prefix = param;
		else if (ARGMATCH("b", "border")) {
			if (sscanf(param, "%d", &pad) != 1
				|| pad <= 0
			) die("argument '%s' expects decimal integer > 0", this);
		}
		else if (ARGMATCH("c", "color")) {
			if (sscanf(param, "%x", &color) != 1
				|| color == 0
			) die("argument '%s' expects hexadecimal value != %06x", this, 0);
		}
		else if (ARGMATCH("a", "area")) {
			if (sscanf(param, "%dx%d", &width, &height) != 2
				|| width <= 0
				|| height <= 0
			) die("argument '%s' expects width by height e.g. 512x512", this);
		}
		else
			die("unknown argument '%s'", this);
		
		++i; /* skips parameter on next iteration */
		
#undef ARGMATCH
	}
	
	/* report missing arguments */
	if (!input || !output || !scheme || !method || !width || !height)
	{
		int area = (width && height);
#define REQUIRE(X) if (!X) info("  %s", #X)
		info("The following necessary arguments were not provided:");
		REQUIRE(input);
		REQUIRE(output);
		REQUIRE(scheme);
		REQUIRE(method);
		REQUIRE(area);
		return -1;
#undef REQUIRE
	}
	
	/* throw the retrieved arguments at the main driver */
	EzSpriteSheet(
		formats
		, expr
		, method
		, scheme
		, input
		, output
		, logfile
		, warnings
		, quiet
		, exhaustive
		, rotate
		, trim
		, doubles
		, pad
		, visual
		, width
		, height
		, negate
		, color
		, &totalSprites
		, &totalDuplicates
		, progress_pack
		, 0 /* load_progress */
	);
	
	/* export */
	if ((errstr = EzSpriteSheet_export(
		output
		, scheme
		, prefix
		, longnames
		, progress_export
	)))
		die("%s", errstr);
	
	/* cleanup */
	EzSpriteSheet_cleanup();
	
	return 0;
}

