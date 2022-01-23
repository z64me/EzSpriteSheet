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
 * and the musl implementation was a valuable reference:
 * http://git.musl-libc.org/cgit/musl/tree/src/misc/nftw.c
 *
 * known limitations:
 *  - symlinks and shortcuts don't work on Windows
 *  - does not support FTW_ACTIONRETVAL
 * 
 */

#ifndef NFTW_UTF8_H_INCLUDED
#define NFTW_UTF8_H_INCLUDED

#include <ftw.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if (defined(_UNICODE) || defined(UNICODE))
int nftw_utf8(
	const char *dirpath
	, int (*fn) (
		const char *fpath
		, const struct stat *sb
		, int typeflag
		, struct FTW *ftwbuf
	), int nopenfd
	, int flags
);
int _wnftw(
	const WCHAR *wpath
	, int (*wfn) (
		const WCHAR *wfpath
		, const struct stat *sb
		, int typeflag
		, struct FTW *ftwbuf
	), int fd_limit
	, int flags
);
#define  _tnftw     _wnftw
#else
#define  _tnftw     nftw
#define  nftw_utf8  nftw
#endif

#else /* linux */

#define  _wnftw     nftw
#define  _tnftw     nftw
#define  nftw_utf8  nftw

#endif

#endif /* NFTW_UTF8_H_INCLUDED */

