/*
 * common.c <z64.me>
 * 
 * common EzSpriteSheet functions live here
 * 
 */

#define _GNU_SOURCE /* strndup */
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "common.h"

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#define ERRMSG_NO_MORE_MEMORY "ran out of memory"

#define QUICKOUT(PREFIX, HANDLE, OVERLOAD) if (HANDLE) { \
	fprintf(HANDLE, "[" PREFIX "] "); \
		va_start(ap, fmt); \
		vfprintf(HANDLE, fmt, ap); \
		va_end(ap); \
	fprintf(HANDLE,"\n"); \
} \
if (OVERLOAD) { \
	static char buf[1024]; \
		va_start(ap, fmt); \
		vsnprintf(buf, sizeof(buf), fmt, ap); \
		va_end(ap); \
	OVERLOAD(buf); \
}

static FILE *logfile = 0;
static int logfile_only_warnings = 0;

void (*die_overload)(const char *msg) = 0;
void (*complain_overload)(const char *msg) = 0;
void (*success_overload)(const char *msg) = 0;
void (*info_overload)(const char *msg) = 0;

//#ifdef _WIN32
#ifndef my_strndup
char *(my_strndup)(const char *s, size_t n)
{
	char *r;
	
	if (!s)
		return 0;
	
	if (strlen(s) < n)
		n = strlen(s);
	
	r = malloc(n + 1);
	if (!r)
		return 0;
	
	memcpy(r, s, n);
	r[n] = '\0';
	
	return r;
}
#endif

#ifndef my_strcasestr
char *(my_strcasestr)(const char *haystack, const char *needle)
{
	if (!needle || !haystack)
		return 0;
	
	while (strlen(needle) <= strlen(haystack))
	{
		if (!strncasecmp(haystack, needle, strlen(needle)))
			return (char*)haystack;
		++haystack;
	}
	
	return 0;
}
#endif
//#endif

/* fatal error message (suppressed only by --quiet) */
void die(const char *fmt, ...)
{
	va_list ap;
	
	if (!fmt)
		exit(EXIT_FAILURE);
	
	QUICKOUT("!", logfile, die_overload);
	
	logfile_close();
	
	exit(EXIT_FAILURE);
}

/* non-fatal warning (suppressed only by --quiet) */
void complain(const char *fmt, ...)
{
	va_list ap;
	
	if (!fmt)
		return;
	
	QUICKOUT("@", logfile, complain_overload);
}

/* generic info (suppressed by --quiet and --warnings) */
void success(const char *fmt, ...)
{
	va_list ap;
	
	if (!fmt)
		return;
	
	QUICKOUT("~", logfile, success_overload);
}

/* generic info (suppressed by --quiet and --warnings) */
void info(const char *fmt, ...)
{
	va_list ap;
	
	if (!fmt)
		return;
	
	if (logfile_only_warnings)
		return;
	
	QUICKOUT("~", logfile, info_overload);
}

void fclose_safe(FILE **fp)
{
	assert(fp);
	assert(*fp);
	
	if (!fp || !*fp)
		return;
	
	if (fclose(*fp))
		die("fclose error");
	
	*fp = 0;
}

FILE *fopen_safe(const char *fn, const char *mode)
{
	FILE *fp;
	
	assert(fn);
	assert(mode);
	
#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
	WCHAR *wfn = char2wchar(fn);
	WCHAR *wmode = char2wchar(mode);
	fp = _wfopen(wfn, wmode);
	char2wchar_free(&wfn);
	char2wchar_free(&wmode);
#else
	fp = fopen(fn, mode);
#endif
	
	if (!fp)
	{
		const char *rw = 0;
		
		if (strchr(mode, 'r'))
			rw = "reading";
		if (strchr(mode, 'w'))
		{
			if (rw)
				rw = "reading and writing";
			else
				rw = "writing";
		}
		else
			rw = "reading";
		die("failed to open file '%s' for %s", fn, rw);
	}
	
	return fp;
}

void strncatf(char *dst, size_t n, const char *fmt, ...)
{
	va_list ap;
	size_t dstlen;
	
	if (!dst || !n || !fmt)
		return;
	
	dstlen = strlen(dst);
	
	if (n < dstlen)
		return;
	
	/* jump to end of string */
	dst += dstlen;
	n -= dstlen;
	
	va_start(ap, fmt);
	vsnprintf(dst, n, fmt, ap);
	va_end(ap);
}

void *calloc_safe(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	
	if (!ptr)
		die(ERRMSG_NO_MORE_MEMORY);
	
	return ptr;
}

void *malloc_safe(size_t size)
{
	void *ptr = malloc(size);
	
	if (!ptr)
		die(ERRMSG_NO_MORE_MEMORY);
	
	return ptr;
}

void *realloc_safe(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	
	if (!ptr)
		die(ERRMSG_NO_MORE_MEMORY);
	
	return ptr;
}

char *strdup_safe(const char *s)
{
	void *ptr;
	
	if (!s)
		return 0;
	
	ptr = strdup(s);
	if (!ptr)
		die(ERRMSG_NO_MORE_MEMORY);
	
	return ptr;
}

char *strndup_safe(const char *s, size_t n)
{
	void *ptr;
	
	if (!s)
		return 0;
	
	ptr = my_strndup(s, n);
	if (!ptr)
		die(ERRMSG_NO_MORE_MEMORY);
	
	return ptr;
}

void (free_safe)(void **ptr)
{
	if (!ptr)
		return;
	
	if (!*ptr)
		return;
	
	free(*ptr);
	*ptr = 0;
}

void (char2wchar_free)(void **ptr)
{
	free_safe(ptr);
}

void *char2wchar(const char *fn)
{
#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
	void *out;
	wchar_t b[1024];
	assert(fn);
	if (!MultiByteToWideChar(CP_UTF8, 0, fn, -1, b, sizeof(b)/sizeof(*b)))
		die("string conversion error");
	out = _wcsdup(b);
	if (!out)
		die(ERRMSG_NO_MORE_MEMORY);
	return out;
#else
	assert(fn);
	return strdup_safe(fn);
#endif
}

int file_is_extension(const char *fn, const char *ext)
{
	/* also handles unlikely cases like "/home/username/.png/none" */
	
	assert(fn);
	assert(ext);
	
	if (!fn || !ext)
		return 0;
	
	if (!(fn = strrchr(fn, '.')))
		return 0;
	
	return !strcasecmp(fn + 1, ext);
}

void logfile_set(FILE *handle)
{
	logfile = handle;
}

void logfile_open(const char *fn)
{
	static char *prev = 0;
	const char *mode = "w";
	FILE *handle;
	
	/* cleanup logic */
	if (!fn)
	{
		if (prev)
			free_safe(&prev);
		return;
	}
	
	/* same log file as before, so keep appending to it */
	if (prev && !strcmp(fn, prev))
		mode = "a";
	else if (prev)
		free_safe(&prev);
	prev = strdup_safe(fn);
	
	if (!(handle = fopen_safe(fn, mode)))
		die("failed to open '%s' for logging", fn);
	
	logfile_set(handle);
}

void logfile_close(void)
{
	if (logfile && logfile != stderr && logfile != stdout)
		fclose_safe(&logfile);
}

void logfile_warnings(int warnings)
{
	logfile_only_warnings = warnings;
}

