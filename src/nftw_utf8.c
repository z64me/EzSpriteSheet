/* 
 * nftw_utf8.c v1.0.0 <z64.me>
 * 
 * a win32 solution for nftw
 *
 * _wnftw() uses wchar_t regardless of _UNICODE status (on Windows)
 * _tnftw() uses _wnftw if Windows _UNICODE is defined, nftw otherwise
 * nftw_utf8() works like _tnftw internally, but inputs/outputs utf8
 * 
 * based on the documentation from the Linux programmer's manual:
 * https://man7.org/linux/man-pages/man3/ftw.3.html
 * 
 * win32-specific code adapted from this example on Rosetta Code:
 * https://rosettacode.org/wiki/Walk_a_directory/Recursively#Windows
 * 
 * the musl implementation was a valuable reference:
 * http://git.musl-libc.org/cgit/musl/tree/src/misc/nftw.c
 *
 * known limitations:
 *  - symlinks and shortcuts don't work on Windows
 *  - does not support FTW_ACTIONRETVAL
 * 
 */

#ifdef _WIN32

#include <ftw.h>
#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct history
{
	struct history *chain;
	dev_t dev;
	ino_t ino;
	int level;
	int base;
};

static int (*wfn_wrapper)(const char *, const struct stat *, int, struct FTW *);


static
char *
wstr2utf8(const void *wstr)
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


static
void *
utf82wstr(const char *str)
{
	if (!str)
		return 0;
#if defined(_WIN32) && (defined(_UNICODE) || defined(UNICODE))
	const char *src = str;
	wchar_t *out = 0;
	size_t src_length;
	int length;
	
	src_length = strlen(src);
	length = MultiByteToWideChar(CP_UTF8, 0, src, src_length, 0, 0);
	out = (malloc)((length+1) * sizeof(*out));
	if (out) {
		MultiByteToWideChar(CP_UTF8, 0, src, src_length, out, length);
		out[length] = L'\0';
	}
	return out;
#else
    return strdup(str);
#endif
}

int do_wfn(
	int (*wfn) (
		const WCHAR *wfpath
		, const struct stat *sb
		, int typeflag
		, struct FTW *ftwbuf
	)
	, const WCHAR *wfpath
	, const struct stat *sb
	, int typeflag
	, struct FTW *ftwbuf
)
{
	int rval = 0;
	
	if (wfn_wrapper)
	{
		char *fpath = wstr2utf8(wfpath);
		
		if (!fpath)
			return -1;
		
		rval = wfn_wrapper(fpath, sb, typeflag, ftwbuf);
		
		free(fpath);
	}
	else if (wfn)
		rval = wfn(wfpath, sb, typeflag, ftwbuf);
	
	return rval;
}

/* construct typeflag as described in the ftw manual */
static int construct_typeflag(const DWORD attrib, int flags)
{
	int typeflag = 0;
	
	if (attrib == INVALID_FILE_ATTRIBUTES)
		return -1;
	
	/* is a directory */
	if (attrib & FILE_ATTRIBUTE_DIRECTORY)
	{
		/* FTW_DEPTH was specified in flags */
		if (flags & FTW_DEPTH)
			typeflag = FTW_DP;
		else
			typeflag = FTW_D;
		
		/* is a directory which can't be read */
		if (attrib & FILE_ATTRIBUTE_READONLY)
			typeflag = FTW_DNR;
	}
	/* is a regular file */
	else
		typeflag = FTW_F;
	
	return typeflag;
};
 
/* Print "message: last Win32 error" to stderr. */
static void oops(const wchar_t *message)
{
	wchar_t *buf;
	DWORD error;
 
	buf = NULL;
	error = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL, error, 0, (wchar_t *)&buf, 0, NULL);
 
	if (buf) {
		fwprintf(stderr, L"%ls: %ls", message, buf);
		LocalFree(buf);
	} else {
		/* FormatMessageW failed. */
		fwprintf(stderr, L"%ls: unknown error 0x%x\n",
		    message, error);
	}
}

/* this function was adapted from a public domain Rosetta Code example:
 * https://rosettacode.org/wiki/Walk_a_directory/Recursively#Windows
 */
static int do_wnftw(
	WCHAR *wpath
	, int (*wfn)(const WCHAR *, const struct stat *, int, struct FTW *)
	, int fd_limit
	, int flags
	, struct history *h
)
{
	struct stack {
		wchar_t			*path;
		size_t			 pathlen;
		size_t			 slashlen;
		HANDLE			 ffh;
		WIN32_FIND_DATAW	 ffd;
		struct stack		*next;
	} *dir, dir0, *ndir;
	size_t patternlen;
	wchar_t *buf, c, *pattern;
	int rval = 0;
 
	dir0.path = wpath;
	dir0.pathlen = wcslen(dir0.path);
	pattern = L"*";
	patternlen = wcslen(pattern);

	//
	// Must put backslash between path and pattern, unless
	// last character of path is slash or colon.
	//
	//   'dir' => 'dir\*'
	//   'dir\' => 'dir\*'
	//   'dir/' => 'dir/*'
	//   'c:' => 'c:*'
	//
	// 'c:*' and 'c:\*' are different files!
	//
	c = dir0.path[dir0.pathlen - 1];
	if (c == ':' || c == '/' || c == '\\')
		dir0.slashlen = dir0.pathlen;
	else
		dir0.slashlen = dir0.pathlen + 1;
 
	/* Allocate space for path + backslash + pattern + \0. */
	buf = calloc(dir0.slashlen + patternlen + 1, sizeof buf[0]);
	if (buf == NULL) {
		perror("calloc");
		exit(1);
	}
	dir0.path = wmemcpy(buf, dir0.path, dir0.pathlen + 1);
 
	dir0.ffh = INVALID_HANDLE_VALUE;
	dir0.next = NULL;
	dir = &dir0;
 
	/* Loop for each directory in linked list. */
loop:
	while (dir) {
		/*
		 * At first visit to directory:
		 *   Print the matching files. Then, begin to find
		 *   subdirectories.
		 *
		 * At later visit:
		 *   dir->ffh is the handle to find subdirectories.
		 *   Continue to find them.
		 */
		if (dir->ffh == INVALID_HANDLE_VALUE) {
			/* Append backslash + pattern + \0 to path. */
			dir->path[dir->pathlen] = '\\';
			wmemcpy(dir->path + dir->slashlen,
			    pattern, patternlen + 1);
 
			/* Find all files to match pattern. */
			dir->ffh = FindFirstFileW(dir->path, &dir->ffd);
			if (dir->ffh == INVALID_HANDLE_VALUE) {
				/* Check if no files match pattern. */
				if (GetLastError() == ERROR_FILE_NOT_FOUND)
					goto subdirs;
 
				/* Bail out from other errors. */
				dir->path[dir->pathlen] = '\0';
				oops(dir->path);
				rval = 1;
				goto popdir;
			}
 
			/* Remove pattern from path; keep backslash. */
			dir->path[dir->slashlen] = '\0';
 
			/* Print all files to match pattern. */
			do {
				const wchar_t *thisname = dir->ffd.cFileName;
				WCHAR *i;
				WCHAR full[MAX_PATH];
				const DWORD attr = dir->ffd.dwFileAttributes;
				int typeflag = construct_typeflag(attr, flags);
				struct stat st;
				int r;
				
				struct FTW lev = {0};
				struct history new;
				new.chain = h;
				new.dev = st.st_dev;
				new.ino = st.st_ino;
				new.level = h ? h->level+1 : 0;
				
				/* TODO not recursive, so refactor this to make lev be
				 * function-scope and increment/decerment lev.level as
				 * directories are entered/exited; it will also know to
				 * exit when fd_limit is exceeded
				 */
				(void)fd_limit;
				lev.level = new.level;
				lev.base = wcslen(dir->path);
				
				/* skip directories named '.' and '..' */
				if (thisname[0] == L'.'
					&& (!thisname[1]
						|| (thisname[1] == L'.'
							&& !thisname[2]
						)
					)
				)
					continue;
				
				wstat(wpath, &st);
				wcscpy(full, dir->path);
				wcscat(full, thisname);
				
				/* 'C:\Assets\Player' -> 'C:/Assets/Player' */
				for (i = full; *i; ++i)
					if (*i == L'\\')
						*i = L'/';
				if ((r=do_wfn(wfn, full, &st, typeflag, &lev)))
					return -1;
				
				//wprintf(L"%ls%ls\n",
				//	dir->path, dir->ffd.cFileName);
			} while (FindNextFileW(dir->ffh, &dir->ffd) != 0);
			if (GetLastError() != ERROR_NO_MORE_FILES) {
				dir->path[dir->pathlen] = '\0';
				oops(dir->path);
				rval = 1;
			}
			FindClose(dir->ffh);
 
subdirs:
			/* Append * + \0 to path. */
			dir->path[dir->slashlen] = '*';
			dir->path[dir->slashlen + 1] = '\0';
 
			/* Find first possible subdirectory. */
			dir->ffh = FindFirstFileExW(dir->path,
			    FindExInfoStandard, &dir->ffd,
			    FindExSearchLimitToDirectories, NULL, 0);
			if (dir->ffh == INVALID_HANDLE_VALUE) {
				dir->path[dir->pathlen] = '\0';
				oops(dir->path);
				rval = 1;
				goto popdir;
			}
		} else {
			/* Find next possible subdirectory. */
			if (FindNextFileW(dir->ffh, &dir->ffd) == 0)
				goto closeffh;				
		}
 
		/* Enter subdirectories. */
		do {
			const wchar_t *fn = dir->ffd.cFileName;
			const DWORD attr = dir->ffd.dwFileAttributes;
			size_t buflen, fnlen;
 
			/*
			 * Skip '.' and '..', because they are links to
			 * the current and parent directories, so they
			 * are not subdirectories.
			 *
			 * Skip any file that is not a directory.
			 *
			 * Skip all reparse points, because they might
			 * be symbolic links. They might form a cycle,
			 * with a directory inside itself.
			 */
			if (wcscmp(fn, L".") == 0 ||
			    wcscmp(fn, L"..") == 0 ||
			    (attr & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
			    (attr & FILE_ATTRIBUTE_REPARSE_POINT))
				continue;
 
			ndir = malloc(sizeof *ndir);
			if (ndir == NULL) {
				perror("malloc");
				exit(1);
			}
 
			/*
			 * Allocate space for path + backslash +
			 *     fn + backslash + pattern + \0.
			 */
			fnlen = wcslen(fn);
			buflen = dir->slashlen + fnlen + patternlen + 2;
			buf = calloc(buflen, sizeof buf[0]);
			if (buf == NULL) {
				perror("malloc");
				exit(1);
			}
 
			/* Copy path + backslash + fn + \0. */
			wmemcpy(buf, dir->path, dir->slashlen);
			wmemcpy(buf + dir->slashlen, fn, fnlen + 1);
 
			/* Push dir to list. Enter dir. */
			ndir->path = buf;
			ndir->pathlen = dir->slashlen + fnlen;
			ndir->slashlen = ndir->pathlen + 1;
			ndir->ffh = INVALID_HANDLE_VALUE;
			ndir->next = dir;
			dir = ndir;
			goto loop; /* Continue outer loop. */
		} while (FindNextFileW(dir->ffh, &dir->ffd) != 0);
closeffh:
		if (GetLastError() != ERROR_NO_MORE_FILES) {
			dir->path[dir->pathlen] = '\0';
			oops(dir->path);
			rval = 1;
		}
		FindClose(dir->ffh);
 
popdir:
		/* Pop dir from list, free dir, but never free dir0. */
		free(dir->path);
		if ((ndir = dir->next))
			free(dir);
		dir = ndir;
	}
 
	return rval;
}


int _wnftw(
	const WCHAR *wpath
	, int (*wfn) (
		const WCHAR *wfpath
		, const struct stat *sb
		, int typeflag
		, struct FTW *ftwbuf
	), int fd_limit
	, int flags
)
{
	int r;
	size_t l;
	WCHAR pathbuf[PATH_MAX+1];
	WCHAR *i;

	if (fd_limit <= 0) return 0;

	l = wcslen(wpath);
	if (l > PATH_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}
	wcscpy(pathbuf, wpath);
	
	/* 'C:\Assets\Player' -> 'C:/Assets/Player' */
	for (i = pathbuf; *i; ++i)
		if (*i == L'\\')
			*i = L'/';
	
	r = do_wnftw(pathbuf, wfn, fd_limit, flags, NULL);
	
	return r;
}

int nftw_utf8(
	const char *path
	, int (*fn) (
		const char *fpath
		, const struct stat *sb
		, int typeflag
		, struct FTW *ftwbuf
	)
	, int nopenfd
	, int flags
)
{
	WCHAR *wpath = utf82wstr(path);
	int r;
	
	if (!wpath)
		return -1;
	
	wfn_wrapper = fn;
	r = _wnftw(wpath, 0, nopenfd, flags);
	wfn_wrapper = 0;
	
	free(wpath);
	return r ? -1 : 0;
}

#endif

