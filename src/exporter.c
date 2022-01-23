/*
 * export.c <z64.me>
 * 
 * sprite bank exporting happens here
 * 
 */

#include "common.h"
#include "exporter.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

/* for writing PNGs */
#define STB_IMAGE_WRITE_IMPLEMENTATION
	#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
		#define STBIW_WINDOWS_UTF8
	#endif
	#define STBIW_MALLOC malloc_safe
	#define STBIW_REALLOC realloc_safe
	#define STBIW_FREE free
#include "stb_image_write.h"

/* private */
static char *writing_filename = 0; /* path to file being written */

/* just a few globals, to reduce code verbosity */
FILE *Exporter__out = 0;   /* file handle for writing data */
char *Exporter__path = 0;  /* directory containing output file */
char *Exporter__name = 0;  /* out filename w/o directory or extension */

/* bindings */
extern const struct Exporter Exporter__xml;
extern const struct Exporter Exporter__json;
extern const struct Exporter Exporter__c99;

/* sanitizes Windows paths 'C:\User\out.xml' -> 'C:/User/out.xml' */
static char *sanitize_path(const char *dirty)
{
	char *clean;
	char *c;
	
	if (!dirty)
		return 0;
	
	clean = strdup_safe(dirty);
	
	for (c = clean; *c; ++c)
		if (*c == '\\')
			*c = '/';
	
	return clean;
}

/* select export mode */
const struct Exporter *Export_begin(const char *name, const char *outfnDirty)
{
	const char *longname;
	const struct Exporter *arr[] =
	{
		&Exporter__xml
		, &Exporter__json
		, &Exporter__c99
	};
	const struct Exporter *e = 0;
	char *outfn = sanitize_path(outfnDirty);
	int i;
	
	assert(name);
	
	/* combo box expanded format e.g. 'C99 Header (.h)(*.h)' */
	longname = strstr(name, " (");
	
	/* select exporter */
	for (i = 0; i < ARRAY_COUNT(arr); ++i)
	{
		if (!strcasecmp(arr[i]->name, name)
			|| (longname
				&& !strncasecmp(arr[i]->longname, name, longname - name)
			)
		)
		{
			e = arr[i];
			break;
		}
	}
	
	/* unknown exporter selected */
	if (!e)
	{
		char buf[2048];
		int buflen = sizeof(buf);
		
		/* construct error message */
		snprintf(buf, buflen, "unknown exporter '%.*s'\n", 64, name);
		strncatf(buf, buflen, "supported exporters:");
		for (i = 0; i < ARRAY_COUNT(arr); ++i)
			strncatf(buf, buflen, "\n  -> %s  (%s)", arr[i]->name, arr[i]->longname);
		
		/* display error message and return error code */
		die("%s", buf);
		return 0;
	}
	
	/* given an input path like '/home/user/out.xml' or 'C:\User\out.xml',
	 * derive '/home/user/' (or 'C:\User\') and 'out'
	 * if input path is 'out.xml', derive './' and 'out'
	 */
	if (outfn)
	{
		char *slash;
		int hasExtension = 0;
		
		Exporter__path = strdup_safe(outfn);
		
		/* select last slash in path */
		slash = strrchr(Exporter__path, '/');
		
		/* '/out.xml' -> 'out' */
		if (slash)
		{
			char *next = slash + 1;
			
			Exporter__name = strndup_safe(next, strcspn(next, "."));
			hasExtension = strlen(Exporter__name) != strlen(next);
			*next = '\0';
		}
		
		/* there is no slash, implying same directory */
		else
		{
			/* 'out.xml' -> 'out' */
			Exporter__name = strndup_safe(outfn, strcspn(outfn, "."));
			hasExtension = strlen(Exporter__name) != strlen(outfn);
			free_safe(&Exporter__path);
			Exporter__path = strdup_safe("./");
		}
		
		/* corner case: path but no filename, '/home/user/game/' */
		if (!strlen(Exporter__name))
		{
			char tmp[2048];
			
			free_safe(&Exporter__name);
			Exporter__name = strdup_safe("output");
			snprintf(tmp, sizeof(tmp), "%s%s.%s", Exporter__path, Exporter__name, name);
			writing_filename = strdup_safe(tmp);
		}
		else
		{
			char tmp[2048];
			
			snprintf(tmp, sizeof(tmp), "%s", outfn);
			
			/* append extension if the user provided an extensionless filename */
			if (!hasExtension)
				strncatf(tmp, sizeof(tmp), ".%s", name);
			
			writing_filename = strdup_safe(tmp);
		}
		
		Exporter__out = fopen_safe(writing_filename, "wb");
		
		/* cleanup */
		free_safe(&outfn);
	}
	else
		Exporter__out = stdout;
	
	assert(e->capsule.begin);
	assert(e->capsule.end);
	
	assert(e->sheet.begin);
	assert(e->sheet.end);
	
	assert(e->animation.begin);
	assert(e->animation.end);
	
	assert(e->frame.begin);
	assert(e->frame.end);
	
	return e;
}

void Export_end(void)
{
	if (Exporter__path)
		free_safe(&Exporter__path);
	if (Exporter__name)
		free_safe(&Exporter__name);
	if (Exporter__out && Exporter__out != stdout)
		fclose_safe(&Exporter__out);
	if (writing_filename)
	{
		success("Wrote '%s' successfully!", writing_filename);
		free_safe(&writing_filename);
	}
}
