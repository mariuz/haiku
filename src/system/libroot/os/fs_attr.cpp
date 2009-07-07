/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <fs_attr.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "dirent_private.h"
#include "syscalls.h"


// TODO: think about adding special syscalls for the read/write/stat functions
// to speed them up

#define RETURN_AND_SET_ERRNO(status) \
	{ \
		if (status < 0) { \
			errno = status; \
			return -1; \
		} \
		return status; \
	}


static DIR *
open_attr_dir(int file, const char *path)
{
	DIR *dir;

	int fd = _kern_open_attr_dir(file, path);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}

	/* allocate the memory for the DIR structure */
	if ((dir = (DIR *)malloc(DIR_BUFFER_SIZE)) == NULL) {
		errno = B_NO_MEMORY;
		_kern_close(fd);
		return NULL;
	}

	dir->fd = fd;
	dir->entries_left = 0;

	return dir;
}


//	#pragma mark -


extern "C" ssize_t
fs_read_attr(int fd, const char* attribute, uint32 /*type*/, off_t pos,
	void* buffer, size_t readBytes)
{
	int attr = _kern_open_attr(fd, attribute, O_RDONLY);
	if (attr < 0)
		RETURN_AND_SET_ERRNO(attr);

	ssize_t bytes = _kern_read(attr, pos, buffer, readBytes);
	_kern_close(attr);

	RETURN_AND_SET_ERRNO(bytes);
}


extern "C" ssize_t
fs_write_attr(int fd, const char* attribute, uint32 type, off_t pos,
	const void* buffer, size_t writeBytes)
{
	// NOTE: This call is deprecated in Haiku and has a number of problems:
	// On BeOS, it was documented that the "pos" argument is ignored.
	// However, a number of programs tried to use this call to write large
	// attributes in a loop anyways. These programs all relied on the broken
	// or at least inconsistent behaviour to truncate/clobber an existing
	// attribute. In other words, writing 5 bytes at position 0 into an
	// attribute that was already 10 bytes long resulted in an attribute of
	// only 5 bytes length.
	// The implementation of this function tries to stay compatible with
	// BeOS in that it clobbers the existing attribute when you write at offset
	// 0, but it also tries to support programs which continue to write more
	// chunks.
	// The new Haiku way is to use fs_open_attr() to get a regular file handle
	// and use that for writing, then use fs_close_attr() when done. As you
	// see from this implementation, it saves 2 syscalls per writing a chunk
	// of data.

	int attr = _kern_create_attr(fd, attribute, type,
		O_WRONLY | (pos != 0 ? 0 : O_TRUNC));
	if (attr < 0)
		RETURN_AND_SET_ERRNO(attr);

	ssize_t bytes = _kern_write(attr, pos, buffer, writeBytes);
	_kern_close(attr);

	RETURN_AND_SET_ERRNO(bytes);
}


extern "C" int
fs_remove_attr(int fd, const char* attribute)
{
	status_t status = _kern_remove_attr(fd, attribute);

	RETURN_AND_SET_ERRNO(status);
}


extern "C" int
fs_stat_attr(int fd, const char* attribute, struct attr_info* attrInfo)
{
	int attr = _kern_open_attr(fd, attribute, O_RDONLY);
	if (attr < 0)
		RETURN_AND_SET_ERRNO(attr);

	struct stat stat;
	status_t status = _kern_read_stat(attr, NULL, false, &stat,
		sizeof(struct stat));
	if (status == B_OK) {
		attrInfo->type = stat.st_type;
		attrInfo->size = stat.st_size;
	}
	_kern_close(attr);

	RETURN_AND_SET_ERRNO(status);
}


/*
int
fs_open_attr(const char *path, const char *attribute, uint32 type, int openMode)
{
	// TODO: implement fs_open_attr() - or remove it completely
	//	if it will be implemented, rename the current fs_open_attr() to fs_fopen_attr()
	return B_ERROR;
}
*/


extern "C" int
fs_open_attr(int fd, const char* attribute, uint32 type, int openMode)
{
	status_t status;

	if ((openMode & O_CREAT) != 0)
		status = _kern_create_attr(fd, attribute, type, openMode);
	else
		status = _kern_open_attr(fd, attribute, openMode);

	RETURN_AND_SET_ERRNO(status);
}


extern "C" int
fs_close_attr(int fd)
{
	status_t status = _kern_close(fd);

	RETURN_AND_SET_ERRNO(status);
}


extern "C" DIR*
fs_open_attr_dir(const char* path)
{
	return open_attr_dir(-1, path);
}


extern "C" DIR*
fs_fopen_attr_dir(int fd)
{
	return open_attr_dir(fd, NULL);
}


extern "C" int
fs_close_attr_dir(DIR* dir)
{
	int status = _kern_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


extern "C" struct dirent*
fs_read_attr_dir(DIR* dir)
{
	return readdir(dir);
}


extern "C" void
fs_rewind_attr_dir(DIR* dir)
{
	rewinddir(dir);
}
