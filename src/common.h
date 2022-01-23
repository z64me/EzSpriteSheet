/*
 * common.h <z64.me>
 * 
 * common EzSpriteSheet functions live here
 * 
 */

#ifndef EZSPRITESHEET_COMMON_H_INCLUDED
#define EZSPRITESHEET_COMMON_H_INCLUDED

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/* misc */
#define ARRAY_COUNT(X) (int)((sizeof((X)) / sizeof(*(X))))
#define UNUSED(X) (void)(X)

/* opaque data strutures */
struct EzSpriteSheetAnim;
struct EzSpriteSheetAnimList;
struct EzSpriteSheetAnimFrame;
struct EzSpriteSheetRect;
struct EzSpriteSheetRectList;
struct File;
struct FileList;

/* common */
FILE *fopen_safe(const char *fn, const char *mode);
void fclose_safe(FILE **fp);
void die(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void complain(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void logfile_set(FILE *handle);
void logfile_open(const char *fn);
void logfile_close(void);
void logfile_warnings(int warnings);
void info(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void alert(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void success(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void strncatf(char *dst, size_t n, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
void *malloc_safe(size_t size);
void *realloc_safe(void *ptr, size_t size);
void *calloc_safe(size_t nmemb, size_t size);
char *strdup_safe(const char *s);
char *strndup_safe(const char *s, size_t n);
void (free_safe)(void **ptr);
#define free_safe(X) (free_safe)((void**)X)
void *char2wchar(const char *fn);
void (char2wchar_free)(void **ptr);
#define char2wchar_free(X) (char2wchar_free)((void**)X)
int file_is_extension(const char *fn, const char *ext);
char *(my_strndup)(const char *s, size_t n);
char *(my_strcasestr)(const char *haystack, const char *needle);
//#define my_strndup strndup
//#define my_strcasestr strcasestr

/* file */
struct File *FileList_get_head(struct FileList *list);
struct File *File_get_next(struct File *file);
void *File_get_udata(struct File *file);
void File_set_udata(struct File *file, void *udata);
const char *File_get_extension(struct File *file);
const char *File_get_path(struct File *file);
struct FileList *FileList_new(const char *path);
void FileList_free(struct FileList **list_);
int FileList_get_count(struct FileList *list);

/* animation */
const struct EzSpriteSheetAnimFrame *EzSpriteSheetAnim_get_lastframe(
	const struct EzSpriteSheetAnim *anim
);
int EzSpriteSheetAnimList_get_count(const struct EzSpriteSheetAnimList *list);
struct EzSpriteSheetAnim *EzSpriteSheetAnimList_head(
	struct EzSpriteSheetAnimList *list
);
struct EzSpriteSheetAnimList *EzSpriteSheetAnimList_new(void);
void EzSpriteSheetAnimList_free(struct EzSpriteSheetAnimList **s);
struct EzSpriteSheetAnim *EzSpriteSheetAnimList_push(
	struct EzSpriteSheetAnimList *list
	, struct EzSpriteSheetAnim *item
);
struct EzSpriteSheetAnim *EzSpriteSheetAnim_new(const char *fn);
void EzSpriteSheetAnim_free(struct EzSpriteSheetAnim **s);
struct EzSpriteSheetAnimFrame *EzSpriteSheetAnim_each_frame(
	struct EzSpriteSheetAnim *s
);
struct EzSpriteSheetAnim *EzSpriteSheetAnim_get_next(
	struct EzSpriteSheetAnim *anim
);
void EzSpriteSheetAnimFrame_get_crop(
	const struct EzSpriteSheetAnimFrame *frame
	, int *x, int *y, int *w, int *h
);
void EzSpriteSheetAnimFrame_set_udata(struct EzSpriteSheetAnimFrame *frame
	, const void *udata
);
int EzSpriteSheetAnimFrame_get_width(const struct EzSpriteSheetAnimFrame *frame);
int EzSpriteSheetAnimFrame_get_height(const struct EzSpriteSheetAnimFrame *frame);
const void *EzSpriteSheetAnimFrame_get_pixels(
	const struct EzSpriteSheetAnimFrame *frame
);
int EzSpriteSheetAnimFrame_get_isPivotFrame(
	const struct EzSpriteSheetAnimFrame *frame
);
int EzSpriteSheetAnimFrame_get_isBlank(
	const struct EzSpriteSheetAnimFrame *frame
);
const struct EzSpriteSheetAnimFrame *EzSpriteSheetAnimFrame_get_isDuplicateOf(
	const struct EzSpriteSheetAnimFrame *frame
);
const void *EzSpriteSheetAnimFrame_get_udata(const struct EzSpriteSheetAnimFrame *s);
void EzSpriteSheetAnimFrame_get_pivot(
	const struct EzSpriteSheetAnimFrame *frame
	, int *x, int *y
);
const char *EzSpriteSheetAnim_get_name(const struct EzSpriteSheetAnim *s);
int EzSpriteSheetAnim_countRealFrames(const struct EzSpriteSheetAnim *anim);
int EzSpriteSheetAnimFrame_get_ms(
	const struct EzSpriteSheetAnimFrame *frame
);
int EzSpriteSheetAnim_get_loopMs(const struct EzSpriteSheetAnim *anim);

/* rectangles */
enum EzSpriteSheetRectSort
{
	EzSpriteSheetRectSort_Area /* sort by area */
	, EzSpriteSheetRectSort_Height /* sort by height */
	, EzSpriteSheetRectSort_Width /* sort by width */
};
enum EzSpriteSheetRectPack
{
	EzSpriteSheetRectPack_MaxRects
	, EzSpriteSheetRectPack_Guillotine
};
int EzSpriteSheetAnimList_each_findPivot(struct EzSpriteSheetAnimList *list
	, const uint32_t color
);
int EzSpriteSheetAnim_findPivot(struct EzSpriteSheetAnim *s
	, const uint32_t color
);
int EzSpriteSheetAnimList_each_findCrop(struct EzSpriteSheetAnimList *list);
int EzSpriteSheetAnim_findCrop(struct EzSpriteSheetAnim *s);
int EzSpriteSheetAnimList_each_clearPivot(struct EzSpriteSheetAnimList *list);
int EzSpriteSheetAnimList_each_findDuplicates(struct EzSpriteSheetAnimList *list);
int EzSpriteSheetAnimList_each_clearDuplicates(struct EzSpriteSheetAnimList *list);
int EzSpriteSheetAnim_clearPivot(struct EzSpriteSheetAnim *s);
struct EzSpriteSheetRectList *EzSpriteSheetRectList_new(void);
struct EzSpriteSheetRect *EzSpriteSheetRectList_push(
	struct EzSpriteSheetRectList *s
	, const void *udata, int width, int height
);
void EzSpriteSheetRectList_sort(struct EzSpriteSheetRectList *s
	, enum EzSpriteSheetRectSort mode
);
void EzSpriteSheetRectList_pack(struct EzSpriteSheetRectList *s
	, enum EzSpriteSheetRectPack mode
	, int width
	, int height
	, int rotate
	, int exhaustive
	, void progress(float unit_interval)
);
void EzSpriteSheetRectList_free(struct EzSpriteSheetRectList **s);
void EzSpriteSheetRectList_pageDebugOverlay(struct EzSpriteSheetRectList *s
	, int page, void *p_, int w, int h, uint8_t opacity
);
void *EzSpriteSheetRectList_page(struct EzSpriteSheetRectList *s
	, int page
	, void *p_
	, int *w
	, int *h
	, int *rects
	, float *occupancy
	, int pad
	, int trim
	, void progress(float unit_interval)
);
void EzSpriteSheetRectList_get_biggest_page(struct EzSpriteSheetRectList *s
	, int *w, int *h
);
int EzSpriteSheetRect_get_page(const struct EzSpriteSheetRect *s);
int EzSpriteSheetRect_get_rotated(const struct EzSpriteSheetRect *s);
void EzSpriteSheetRect_get_crop(
	const struct EzSpriteSheetRect *s
	, int *x, int *y, int *w, int *h
);
int EzSpriteSheetRectList_getPageCount(const struct EzSpriteSheetRectList *s);
int EzSpriteSheetAnimFrame_findDuplicates(struct EzSpriteSheetAnimFrame *frame);

#endif /* EZSPRITESHEET_COMMON_H_INCLUDED */
