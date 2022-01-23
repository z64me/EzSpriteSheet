/*
 * file.c <z64.me>
 * 
 * EzSpriteSheet's file list magic
 * 
 */

#include <assert.h>
#include <string.h>
#include "common.h"
#include "nftw_utf8.h"

struct File
{
	struct File  *next;   /* next in linked list */
	char         *path;   /* file path */
	const char   *ext;    /* extension (substring within path, 'gif') */
	void         *udata;  /* user data */
};

struct FileList
{
	struct File *head;
	int count;
};

/* global variable, since nftw doesn't have a udata parameter */
static struct FileList *active = 0;

static int each(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
	struct File *file;
	
	/* skip everything besides regular files */
	if (type != FTW_F)
		return 0;
	
	/* skip extensionless files */
	if (!strrchr(pathname, '.'))
		return 0;
	
	assert(active);
	
	/* allocate and link into list */
	file = calloc_safe(1, sizeof(*file));
	file->next = active->head;
	active->head = file;
	active->count += 1;
	
	/* stats for each */
	file->path = strdup_safe(pathname);
	file->ext = strrchr(file->path, '.') + 1;
	
	return 0;
	
	(void)sbuf;
	(void)ftwb;
}

/* clean up a file list */
void FileList_free(struct FileList **list_)
{
	struct FileList *list;
	struct File *file;
	struct File *next = 0;
	
	if (!list_ || !*list_)
		return;
	
	list = *list_;
	
	for (file = list->head; file; file = next)
	{
		next = file->next;
		
		if (file->path)
			free_safe(&file->path);
		
		free_safe(&file);
	}
	
	free_safe(list_);
}

/* generate a file list by walking the file tree in the specified path */
struct FileList *FileList_new(const char *path)
{
	struct FileList *list = calloc_safe(1, sizeof(*list));
	
	active = list;
	
	if (nftw_utf8(path, each, 64 /* max directory depth */, FTW_DEPTH) < 0)
		die("failed to walk file tree '%s'", path);
	
	return list;
}

/* get the head of a file list */
struct File *FileList_get_head(struct FileList *list)
{
	assert(list);
	
	return list->head;
}

/* get the next link when walking a file list */
struct File *File_get_next(struct File *file)
{
	assert(file);
	
	if (!file)
		return 0;
	
	return file->next;
}

/* get a file's path */
const char *File_get_path(struct File *file)
{
	assert(file);
	
	if (!file)
		return 0;
	
	return file->path;
}

/* get a file's udata */
void *File_get_udata(struct File *file)
{
	assert(file);
	
	if (!file)
		return 0;
	
	return file->udata;
}

/* set a file's udata */
void File_set_udata(struct File *file, void *udata)
{
	assert(file);
	
	if (!file)
		return;
	
	file->udata = udata;
}

/* get a file's extension */
const char *File_get_extension(struct File *file)
{
	assert(file);
	
	if (!file)
		return 0;
	
	return file->ext;
}

/* get number of files in file list */
int FileList_get_count(struct FileList *list)
{
	assert(list);
	
	if (!list)
		return 0;
	
	return list->count;
}

