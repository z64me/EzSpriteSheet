/* 
 * ezspritesheet.c <z64.me>
 * 
 * This source file is EzSpriteSheet's main driver.
 * It is meant to be used like so:
 * 
 * EzSpriteSheet()
 *   invoke this function to update settings
 *   and re(process) sprite batches
 * 
 * EzSpriteSheet_export()
 *   this function exports whatever information came about
 *   from the most recent EzSpriteSheet() invocation
 * 
 * EzSpriteSheet_cleanup()
 *   use this one at program exit
 * 
 * If this were just for an ordinary command line application
 * that operates sequentially, this approach would border on
 * overengineered. I chose to design it this way so the same
 * code would work seamlessly across the command line and GUI
 * versions of EzSpriteSheet. When the user tweaks settings and
 * clicks the 'Refresh' button, the EzSpriteSheet() function is
 * invoked. A persistent state is maintained across invocations,
 * so there is no need to walk the file tree every time, or to
 * reload and reprocess images constantly. Doing it this way
 * keeps the GUI fast and responsive.
 * 
 * In a similar vein, EzSpriteSheet_export() can be reinvoked
 * on the same data set without having to reprocess everything
 * before every export.
 * 
 * As an ordinary command line application meant to be run once,
 * this approach borders on overengineering. I designed it this
 * way so it would integrate better into a GUI application. Each
 * time the user tweaks settings and clicks the 'Refresh' button,
 * the EzSpriteSheet() function is invoked. Data is reused across
 * invocations, so there is no need to walk the file tree every
 * time, or reload/reprocess images constantly. Doing it this way
 * keeps the GUI fast and responsive.
 * 
 * In a similar vein, EzSpriteSheet_export() can easily be reused
 * on the same data set without having to reprocess everything
 * before every export.
 * 
 */

#define _GNU_SOURCE /* strcasestr */
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "program.h"
#include "exporter.h"

#ifdef _WIN32
	#include <pcre2posix.h>
	#define regcomp pcre2_regcomp
	#define regexec pcre2_regexec
	#define regerror pcre2_regerror
	#define regfree pcre2_regfree
#else
	#include <regex.h>
#endif

#define OFFSTR "off"
#define ONSTR "on"
#define BOOL_ON_OFF(X) (X) ? ONSTR : OFFSTR

/* globals help reduce verbosity here */
static struct
{
	char *formats;
	char *expr;
	char *method;
	char *scheme;
	char *input;
	char *output;
	char *logfile;
	int warnings;
	int quiet;
	int exhaustive;
	int rotate;
	int trim;
	int doubles;
	int pad;
	int visual;
	int width;
	int height;
	int negate;
	int hasRegex;
	uint32_t color;
	struct EzSpriteSheetAnimList *animList;
	struct EzSpriteSheetRectList *rectList;
	struct FileList *fileList;
	struct
	{
		void *pix;
		unsigned maxpix;
	} page;
	regex_t regex;
} g = {0};

#define rectList g.rectList /* hello lazy */
#define animList g.animList
#define fileList g.fileList

/* logging gets turned on at the beginning of each
 * function: export, cleanup, and of course, the main driver
 */
static void logging_begin(void)
{
	/* user wishes to suppress all output */
	if (g.quiet)
		logfile_set(0);
	else
	{
		/* set warning level */
		logfile_warnings(g.warnings);
		
		/* user wishes to log misc output to file */
		if (g.logfile)
			logfile_open(g.logfile);
		/* fall back to stderr otherwise */
		else
			logfile_set(stderr);
	}
}

/* logging gets turned off at the end of each function */
static void logging_end(void)
{
	if (!g.quiet)
		logfile_close();
}

static void get_crop(
	const struct EzSpriteSheetAnimFrame *frame
	, int *x, int *y, int *w, int *h
)
{
	if (g.trim)
		EzSpriteSheetAnimFrame_get_crop(frame, x, y, w, h);
	else
	{
		*x = 0;
		*y = 0;
		*w = EzSpriteSheetAnimFrame_get_width(frame);
		*h = EzSpriteSheetAnimFrame_get_height(frame);
	}
}

/* returns if strings were neq (not equal) before making a duplicate */
static int neqdup(char **dst, const char *src)
{
	assert(dst);
	
	if (!*dst && !src)
		return 0;
	
	if (*dst && !src)
	{
		free_safe(dst);
		return 1;
	}
	
	/* empty */
	if (!*dst)
	{
		*dst = strdup_safe(src);
		return 1;
	}
	
	/* different */
	if (strcmp(*dst, src))
	{
		free_safe(dst);
		*dst = strdup_safe(src);
		return 1;
	}
	
	/* identical */
	return 0;
}

static void cleanup_regex(void)
{
	if (g.hasRegex)
	{
		regfree(&g.regex);
		g.hasRegex = 0;
	}
}

static void cleanup_files(void)
{
	FileList_free(&fileList);
}

static void cleanup_images(void)
{
	EzSpriteSheetAnimList_free(&animList);
}

static void cleanup_rectangles(void)
{
	EzSpriteSheetRectList_free(&rectList);
}

void EzSpriteSheet_setPopups(
	void die(const char *msg)
	, void complain(const char *msg)
	, void success(const char *msg)
)
{
	extern void (*die_overload)(const char *msg);
	extern void (*complain_overload)(const char *msg);
	extern void (*success_overload)(const char *msg);
	
	die_overload = die;
	complain_overload = complain;
	success_overload = success;
}

void EzSpriteSheet_cleanup(void)
{
	logging_begin();
	
	free_safe(&g.formats);
	free_safe(&g.expr);
	free_safe(&g.method);
	free_safe(&g.scheme);
	free_safe(&g.input);
	free_safe(&g.output);
	free_safe(&g.logfile);
	free_safe(&g.page.pix);
	
	cleanup_files();
	cleanup_images();
	cleanup_rectangles();
	cleanup_regex();
	
	logging_end();
	
	logfile_open(0);
}

/* count the number of sprite sheets */
int EzSpriteSheet_countPages(void)
{
	if (!rectList)
		return 0;
	
	return EzSpriteSheetRectList_getPageCount(rectList);
}

/* bakes a sprite sheet; instead of having all sprite sheets
 * in memory simultaneously, just get them one by one, as needed
 */
void *EzSpriteSheet_getPagePixels(
	int page
	, int *w
	, int *h
	, int *rects
	, float *occupancy
	, void progress(float unit_interval)
)
{
	void *p = g.page.pix;
	
	if (!rectList
		|| page < 0
		|| page >= EzSpriteSheetRectList_getPageCount(rectList)
	)
		return 0;
	
	EzSpriteSheetRectList_get_biggest_page(rectList, w, h);
	
	/* initial allocation */
	if (!p)
	{
		g.page.maxpix = *w * *h;
		
		p = malloc_safe(g.page.maxpix * sizeof(uint32_t));
	}
	/* subsequent resizes: fit 1.5x the data needed
	 * (reduces the frequency of realloc, and reduces fragmentation) */
	if ((unsigned)*w * *h > g.page.maxpix)
	{
		g.page.maxpix = *w * *h;
		g.page.maxpix += g.page.maxpix / 2;
		
		p = realloc_safe(p, g.page.maxpix * sizeof(uint32_t));
	}
	
	/* page containing sprites */
	EzSpriteSheetRectList_page(rectList, page, p, w, h, rects, occupancy, g.pad, g.trim, progress);
	
	/* overlay translucent debugging rectangles */
	if (g.visual)
		EzSpriteSheetRectList_pageDebugOverlay(rectList, page, p, *w, *h, 0xc0);
	
	/* reuse later */
	g.page.pix = p;
	
	return p;
}

const char *EzSpriteSheet_export(
	const char *output
	, const char *scheme
	, const char *prefix
	, int longnames
	, void progress(float unit_interval)
)
{
	const struct Exporter *exporter;
	struct EzSpriteSheetAnim *anim;
	int page;
	int inputLen;
	float occupancy;
	
	if (!prefix)
		prefix = "";
	
	if (!rectList)
		return "Empty rectangle list...";
	
	assert(scheme);
	assert(output);
	assert(g.input);
	
	/* for skipping the base path, so a long animation
	 * name such as '/home/user/game/gfx/spider/walk.gif'
	 * -> 'spider/walk.gif'
	 */
	inputLen = strlen(g.input);
	
	logging_begin();
	
	/* export process */
	exporter = Export_begin(scheme, output);
	exporter->capsule.begin(
		EzSpriteSheetRectList_getPageCount(rectList)
		, EzSpriteSheetAnimList_get_count(animList)
		, 0
		, 0
	);
	
	/* export sheets */
	for (page = 0
		; page < EzSpriteSheetRectList_getPageCount(rectList)
		; ++page
	)
	{
		void *p;
		int w;
		int h;
		int isFirst = page == 0;
		int isLast = (page + 1) == EzSpriteSheetRectList_getPageCount(rectList);
		int rects;
		
		p = EzSpriteSheet_getPagePixels(page, &w, &h, &rects, &occupancy, progress);
		
		assert(p);
		
		exporter->sheet.begin(page, p, w, h, isFirst, isLast);
		exporter->sheet.end(page, p, w, h, isFirst, isLast);
	}
	
	/* export info about each animation */
	for (anim = EzSpriteSheetAnimList_head(animList)
		; anim
		; anim = EzSpriteSheetAnim_get_next(anim)
	)
	{
		const char *name = EzSpriteSheetAnim_get_name(anim);
		const struct EzSpriteSheetAnimFrame *frame;
		char fmt[1024];
		int realFrames = EzSpriteSheetAnim_countRealFrames(anim);
		int animDur = EzSpriteSheetAnim_get_loopMs(anim);
		int frameIndex = 0;
		int isFirst = anim == EzSpriteSheetAnimList_head(animList);
		int isLast = !EzSpriteSheetAnim_get_next(anim);
		
		/* skip base path and redundant slashes */
		name += inputLen;
		while (*name == '\\' || *name == '/')
			++name;
		
		snprintf(fmt, sizeof(fmt), "%s%s", prefix, name);
		
		/* exclude file extensions if flag not enabled */
		if (!longnames)
		{
			char *ext = strrchr(fmt, '.');
			if (ext)
				*ext = '\0';
		}
		
		/*fprintf(stderr, "'%s'\n", name);
		fprintf(stderr, " -> %d frames\n", realFrames);
		fprintf(stderr, " -> %d ms\n", animDur);*/
		
		exporter->animation.begin(fmt, realFrames, animDur, isFirst, isLast);
		
		/* for each frame within animation */
		while ((frame = EzSpriteSheetAnim_each_frame(anim)))
		{
			const struct EzSpriteSheetRect *rect;
			int x;
			int y;
			int w;
			int h;
			int ox;
			int oy;
			int page;
			int dur;
			int isFirst = frameIndex == 0;
			int isLast = frame == EzSpriteSheetAnim_get_lastframe(anim);
			int rot;
			int hasPivot = 1;
			
			/* skip control frames */
			if (EzSpriteSheetAnimFrame_get_isPivotFrame(frame))
				continue;
			
			/* udata references the clipping rectangle on the final sheet */
			rect = EzSpriteSheetAnimFrame_get_udata(frame);
			
			/* possibly a duplicate */
			if (!rect)
			{
				const struct EzSpriteSheetAnimFrame *isDuplicateOf;
				isDuplicateOf = EzSpriteSheetAnimFrame_get_isDuplicateOf(frame);
				
				/* only if user wants duplicates accounted for */
				if (isDuplicateOf && g.doubles)
					rect = EzSpriteSheetAnimFrame_get_udata(isDuplicateOf);
			}
			
			/* get pivot relative to UL corner of sprite */
			EzSpriteSheetAnimFrame_get_pivot(frame, &ox, &oy);
			get_crop(frame, &x, &y, &w, &h);
			dur = EzSpriteSheetAnimFrame_get_ms(frame);
			if (ox < 0) /* unset */
				hasPivot = ox = oy = 0;
			
			/* offset */
			ox -= x;
			oy -= y;
			ox += g.pad;
			oy += g.pad;
			
			/* get clipping rect within sprite sheet */
			if (rect)
			{
				page = EzSpriteSheetRect_get_page(rect);
				rot = EzSpriteSheetRect_get_rotated(rect);
				EzSpriteSheetRect_get_crop(rect, &x, &y, &w, &h);
			}
			else
				rot = page = ox = oy = x = y = w = h = 0;
			
			/* simple output */
			exporter->frame.begin(frameIndex, page, x, y, w, h, ox, oy, dur, rot, isFirst, isLast);
			exporter->frame.end(frameIndex, page, x, y, w, h, ox, oy, dur, rot, isFirst, isLast);
			/*fprintf(stderr
				, " --%2d-> %d ms %d {%d,%d,%d,%d} {%d,%d}\n"
				, frameIndex, dur, page, x, y, w, h, ox, oy
			);*/
			
			++frameIndex;
			
			(void)hasPivot;
		}
		
		/* failsafe: write a blank frame for empty animation */
		if (!frameIndex)
		{
			exporter->frame.begin(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1);
			exporter->frame.end(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1);
		}
		
		exporter->animation.end(fmt, realFrames, animDur, isFirst, isLast);
	}
	exporter->capsule.end(
		EzSpriteSheetRectList_getPageCount(rectList)
		, EzSpriteSheetAnimList_get_count(animList)
		, 0
		, 0
	);
	
	if (progress)
		progress(2);
	
	Export_end();
	
	logging_end();
	
	return 0;
}

/* this multi-purpose function is able to be invoked multiple
 * times while the program (GUI or CLI) is running; it is
 * designed this way so it doesn't have to re-walk the file
 * tree or reload any images when minor settings are adjusted
 */
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
)
{
	const char *rval = 0;
	const char *regmatch = negate ? "nonmatching" : "matching";
	struct EzSpriteSheetAnim *anim;
	int doFileTree = 0; /* walk or re-walk file tree */
	int doImages = 0; /* load images within file tree */
	int doImageAll = 0; /* reprocess images in toto */
	int doRectangles = 0; /* generate new rectangle list */
	int pivotChanged = color != g.color;
	int formatsChanged = 0;
	int regexChanged = 0;
	
	assert(totalSprites);
	assert(totalDuplicates);
	
	if (!output)
		output = "unset";
	if (!scheme)
		scheme = "unset";
	
	neqdup(&g.logfile, logfile);
	g.quiet = quiet;
	g.warnings = warnings;
	
	logging_begin();
	
/* detect differences between current and previous invocation */
	
	/* misc */
	if (neqdup(&g.formats, formats)) /* format list changed */
		formatsChanged = 1;
	if (neqdup(&g.expr, expr)) /* regex changed */
	{
		/* clean up previously compiled regex */
		cleanup_regex();
		
		/* compile new regex (if one was specified) */
		if (expr)
		{
			if (regcomp(&g.regex, expr, 0))
				die("regex error");
			
			g.hasRegex = 1;
		}
		
		regexChanged = 1;
	}
	if (g.negate != negate) /* regex matching method changed */
		regexChanged = 1;
	
	/* reasons to reprocess the rectangles */
	if (neqdup(&g.method, method) /* selected new packing method */
		|| g.width != width /* new page dimensions */
		|| g.height != height
		|| g.pad != pad /* padded rects are larger, so repack */
		|| g.trim != trim /* trimmed rects are smaller, so repack */
		|| g.rotate != rotate /* rotation logic changes pack result */
		|| g.exhaustive != exhaustive /* so does exhaustive logic */
		|| g.doubles != doubles /* omitting duplicates saves space */
	)
		doRectangles = 1;
	
	/* reasons to reprocess the images */
	if (pivotChanged /* search for new pivot color */
		|| formatsChanged /* user wants to include/exclude new formats */
		|| regexChanged /* user wants to filter using new regex */
	)
		doImages = 1;
	
	/* reasons to reprocess the file tree */
	if (neqdup(&g.input, input)) /* selected different file tree */
		doFileTree = 1;
	
/* done */
	
	if (!expr)
		regmatch = "n/a";
	
	g.exhaustive = exhaustive;
	g.rotate = rotate;
	g.trim = trim;
	g.doubles = doubles;
	g.pad = pad;
	g.visual = visual;
	g.width = width;
	g.height = height;
	g.color = color;
	g.negate = negate;
	
	/* echo retrieved arguments back to user */
	info("The following selections were made:");
	info("  Input       '%s'", input);
	info("  Output      '%s'", output);
	info("  Area        '%dx%d'", width, height);
	info("  Scheme      '%s'", scheme);
	info("  Method      '%s'", method);
	info("  Formats     '%s'", formats);
	info("  RegEx       '%s' (%s)", (expr) ? expr : OFFSTR, regmatch);
	info("  Log         '%s'", (logfile) ? logfile : OFFSTR);
	info("  Doubles     '%s'", BOOL_ON_OFF(doubles));
	info("  Visual      '%s'", BOOL_ON_OFF(visual));
	info("  Exhaustive  '%s'", BOOL_ON_OFF(exhaustive));
	info("  Rotate      '%s'", BOOL_ON_OFF(rotate));
	info("  Trim        '%s'", BOOL_ON_OFF(trim));
	info("  Pad         '%s' (%d)", BOOL_ON_OFF(pad), pad);
	info("  Color       '%s' (%06x)", BOOL_ON_OFF(color), color);
	
	/* file tree refresh */
	if (doFileTree)
	{
		/* refreshing the file tree refreshes everything else */
		cleanup_files();
		cleanup_images();
		cleanup_rectangles();
		doImages = 1;
		doImageAll = 1;
		
		/* walk the file tree */
		fileList = FileList_new(input);
		
		/* brand new animation list */
		animList = EzSpriteSheetAnimList_new();
	}
	
	/* image list refresh */
	if (doImages || doImageAll)
	{
		struct File *file;
		int loaded = 0;
		int count = FileList_get_count(fileList);
		if (!count)
			goto emtyFileList;
		
		/* optimization: only clean up images with undesirable extensions
		 *               or those filtered by regex (mis)matches
		 */
		for (file = FileList_get_head(fileList)
			; file
			; file = File_get_next(file)
		)
		{
			const char *ext = File_get_extension(file);
			int match = 1;
			
			if (g.hasRegex)
			{
				const char *path = File_get_path(file);
				
				switch (regexec(&g.regex, path, 0, NULL, 0))
				{
					case 0:
						match = 1;
						break;
					
					case REG_NOMATCH:
						match = 0;
						break;
					
					default:
						die("regex error");
						break;
				}
				
				if (negate)
					match = !match;
			}
			
			/* the extension this file has either isn't among the formats
			 * the user wants, or it doesn't match the regular expression
			 */
			if (!my_strcasestr(formats, ext) || !match)
			{
				/* so release the memory occupied by the associated animation */
				if ((anim = File_get_udata(file)))
				{
					info("Clean up image file '%s'", File_get_path(file));
					EzSpriteSheetAnim_free(&anim);
					File_set_udata(file, 0);
				}
			}
			/* desirable file extension, so try loading the image */
			else
			{
				const char *fn = File_get_path(file);
				
				/* skip files who already have images loaded for them */
				if (File_get_udata(file))
					continue;
				
				/* load animation and associate with file */
				++loaded;
				info("Load image file '%s'", fn);
				anim = EzSpriteSheetAnim_new(fn);
				EzSpriteSheetAnimList_push(animList, anim);
				File_set_udata(file, anim);
			}
			
			/* report progress */
			if (load_progress)
				load_progress(((float)loaded) / count);
		}
		
		if (!loaded)
		{
		emtyFileList:
			complain(
				"No supported images found in the directory '%s'."
				, input
			);
			FileList_free(&fileList);
			free_safe(&g.input);
			return "Try a directory containing images.";
		}
		
		/* report progress complete */
		if (load_progress)
			load_progress(2);
		
		/* refreshing the image list also refreshes the rectangles */
		doRectangles = 1;
		
		/* derive helpful information about each; it doesn't
		 * hurt to invoke these this often, because images that
		 * have already been processed through each are skipped
		 * on subsequent invocations, except in the following
		 * cases where reprocessing can be beneficial:
		 *   -> all images are re-tested for duplicates when
		 *      new ones are loaded
		 *   -> pivot color has been changed or omitted
		 */
		EzSpriteSheetAnimList_each_findCrop(animList);
		
		if (doImageAll || formatsChanged)
		{
			EzSpriteSheetAnimList_each_clearDuplicates(animList);
			EzSpriteSheetAnimList_each_findDuplicates(animList);
		}
		
		if (doImageAll || pivotChanged)
		{
			/* pivot detection is one area where something unexpected
			 * can happen: if more than one pixel matching the pivot
			 * color is found, it doesn't know which pixel to count
			 * as the pivot, so it throws a warning to the user about
			 * which frame in which image is unexpected; in this
			 * situation, ignore all specified pivots, and still
			 * go on to pack the rectangles etc, just ask the user
			 * to do a new pivot
			 */
			if (EzSpriteSheetAnimList_each_findPivot(animList, color))
			{
				EzSpriteSheetAnimList_each_clearPivot(animList);
			}
		}
	}
	
	/* reprocess rectangles */
	if (doRectangles)
	{
		enum EzSpriteSheetRectPack packer = 0;
		int badsize = 0;
		*totalDuplicates = 0;
		*totalSprites = 0;
		
		/* clean up all rectangles; constructing new ones isn't costly */
		cleanup_rectangles();
		
		/* allocate and propagate rectangle list */
		rectList = EzSpriteSheetRectList_new();
		
		for (anim = EzSpriteSheetAnimList_head(animList)
			; anim
			; anim = EzSpriteSheetAnim_get_next(anim)
		)
		{
			struct EzSpriteSheetAnimFrame *frame;
			
			while ((frame = EzSpriteSheetAnim_each_frame(anim)))
			{
				struct EzSpriteSheetRect *rect = 0;
				int x;
				int y;
				int w;
				int h;
				
				/* skip blank frames, control frames, duplicate frames */
				if (EzSpriteSheetAnimFrame_get_isBlank(frame)
					|| (EzSpriteSheetAnimFrame_get_isPivotFrame(frame)
						&& g.color /* only if enabled */
					)
					|| (EzSpriteSheetAnimFrame_get_isDuplicateOf(frame)
						&& g.doubles /* only if enabled */
					)
				)
				{
					/* count duplicates for stats */
					if (EzSpriteSheetAnimFrame_get_isDuplicateOf(frame))
						*totalDuplicates += 1;
					
					EzSpriteSheetAnimFrame_set_udata(frame, 0);
					continue;
				}
				
				get_crop(frame, &x, &y, &w, &h);
				
				w += g.pad * 2;
				h += g.pad * 2;
				
	
				/* preprocessing step: complain if page size too small */
				if (w > width || h > height)
				{
					badsize = 1;
					complain(
						"'%s' contains a frame too big (%dx%d)"
						" to fit on designated sprite sheet (%dx%d)"
						, EzSpriteSheetAnim_get_name(anim)
						, w
						, h
						, width
						, height
					);
					rval = "Try increasing dimensions.";
					break;
				}
				
				//fprintf(stderr, "%s frame %d\n", fn, k - 1);
				rect = EzSpriteSheetRectList_push(rectList, frame, w, h);
				EzSpriteSheetAnimFrame_set_udata(frame, rect);
				*totalSprites += 1;
			}
			
			if (badsize)
				break;
		}
		
		if (badsize)
		{
			cleanup_rectangles();
		}
		else
		{
		#define METHOD(A, B) \
			if (!strcasecmp(method, A)) \
				packer = EzSpriteSheetRectPack_ ## B
			METHOD("maxrects", MaxRects);
			else METHOD("guillotine", Guillotine);
			else die("unknown method '%s'", method);
		#undef METHOD
			
			EzSpriteSheetRectList_sort(rectList, EzSpriteSheetRectSort_Area);
			EzSpriteSheetRectList_pack(rectList, packer, width, height, rotate, exhaustive, pack_progress);
		}
	}
	
	//info("wow");
	
	logging_end();
	
	return rval;
}

