/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "cdda.h"
#include "cddb.h"
#include "Lock.h"

#include <FindDirectory.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>
#include <Mime.h>
#include <TypeConstants.h>

#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


//#define TRACE_CDDA
#ifdef TRACE_CDDA
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


class Attribute;
class Inode;
struct attr_cookie;
struct dir_cookie;

typedef DoublyLinkedList<Attribute> AttributeList;
typedef DoublyLinkedList<attr_cookie> AttrCookieList;

struct riff_header {
	uint32	magic;
	uint32	length;
	uint32	id;
} _PACKED;

struct riff_chunk {
	uint32	fourcc;
	uint32	length;
} _PACEKD;

struct wav_format_chunk : riff_chunk {
	uint16		format_tag;
	uint16		channels;
	uint32		samples_per_second;
	uint32		average_bytes_per_second;
	uint16		block_align;
	uint16		bits_per_sample;
} _PACKED;

struct wav_header {
	riff_header			header;
	wav_format_chunk	format;
	riff_chunk			data;
} _PACKED;

enum attr_mode {
	kDiscIDAttributes,
	kSharedAttributes,
	kDeviceAttributes
};

class Volume {
	public:
		Volume(mount_id id);
		~Volume();

		status_t	InitCheck();
		mount_id	ID() const { return fID; }
		uint32		DiscID() const { return fDiscID; }
		Inode		&RootNode() const { return *fRootNode; }

		status_t	Mount(const char* device);
		int			Device() const { return fDevice; }
		vnode_id	GetNextNodeID() { return fNextID++; }

		const char	*Name() const { return fName; }
		status_t	SetName(const char *name);

		Semaphore	&Lock();

		Inode		*Find(vnode_id id);
		Inode		*Find(const char *name);

		Inode		*FirstEntry() const { return fFirstEntry; }

		off_t		NumBlocks() const { return fNumBlocks; }
		size_t		BufferSize() const { return 32 * kFrameSize; }
			// TODO: for now

		static void	DetermineName(cdtext &text, char *name, size_t length);

	private:
		Inode		*_CreateNode(Inode *parent, const char *name,
						off_t start, off_t frames, int32 type);
		int			_OpenAttributes(int mode,
						enum attr_mode attrMode = kDiscIDAttributes);
		void		_RestoreAttributes();
		void		_StoreAttributes();
		void		_RestoreSharedAttributes();
		void		_StoreSharedAttributes();

		Semaphore	fLock;
		int			fDevice;
		mount_id	fID;
		uint32		fDiscID;
		Inode 		*fRootNode;
		vnode_id	fNextID;
		char		*fName;
		off_t		fNumBlocks;

		// root directory contents - we don't support other directories
		Inode		*fFirstEntry;
};

class Attribute : public DoublyLinkedListLinkImpl<Attribute> {
	public:
		Attribute(const char *name, type_code type);
		~Attribute();

		status_t InitCheck() const { return fName != NULL ? B_OK : B_NO_MEMORY; }
		status_t SetTo(const char *name, type_code type);
		void SetType(type_code type) { fType = type; }

		status_t ReadAt(off_t offset, uint8 *buffer, size_t *_length);
		status_t WriteAt(off_t offset, const uint8 *buffer, size_t *_length);
		void Truncate();
		status_t SetSize(off_t size);

		const char *Name() const { return fName; }
		size_t Size() const { return fSize; }
		type_code Type() const { return fType; }
		uint8 *Data() const { return fData; }

	private:
		char		*fName;
		type_code	fType;
		uint8		*fData;
		size_t		fSize;
};

class Inode {
	public:
		Inode(Volume *volume, Inode *parent, const char *name, off_t start,
			off_t frames, int32 type);
		~Inode();

		status_t	InitCheck();
		vnode_id	ID() const { return fID; }

		const char	*Name() const { return fName; }
		status_t	SetName(const char* name);

		int32		Type() const
						{ return fType; }
		gid_t		GroupID() const
						{ return fGroupID; }
		uid_t		UserID() const
						{ return fUserID; }
		time_t		CreationTime() const
						{ return fCreationTime; }
		time_t		ModificationTime() const
						{ return fModificationTime; }
		off_t		StartFrame() const
						{ return fStartFrame; }
		off_t		FrameCount() const
						{ return fFrameCount; }
		off_t		Size() const
						{ return fFrameCount * kFrameSize /* + WAV header */; }

		Attribute	*FindAttribute(const char *name) const;
		status_t	AddAttribute(Attribute *attribute, bool overwrite);
		status_t	AddAttribute(const char *name, type_code type,
						bool overwrite, const uint8 *data = 0,
						size_t length = 0);
		status_t	AddAttribute(const char *name, type_code type,
						const char *string);
		status_t	AddAttribute(const char *name, int32 value);
		status_t	RemoveAttribute(const char *name);

		void		AddAttrCookie(attr_cookie *cookie);
		void		RemoveAttrCookie(attr_cookie *cookie);
		void		RewindAttrCookie(attr_cookie *cookie);

		AttributeList::ConstIterator Attributes() const
						{ return fAttributes.GetIterator(); }

		const wav_header *WAVHeader() const { return &fWAVHeader; }

		Inode		*Next() const { return fNext; }
		void		SetNext(Inode *inode) { fNext = inode; }

	private:
		Inode		*fNext;
		vnode_id	fID;
		int32		fType;
		char		*fName;
		gid_t		fGroupID;
		uid_t		fUserID;
		time_t		fCreationTime;
		time_t		fModificationTime;
		off_t		fStartFrame;
		off_t		fFrameCount;
		AttributeList fAttributes;
		AttrCookieList fAttrCookies;
		wav_header	fWAVHeader;
};

struct dir_cookie {
	Inode		*current;
	int			state;	// iteration state
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

struct attr_cookie : DoublyLinkedListLinkImpl<attr_cookie> {
	Attribute	*current;
};

struct file_cookie {
	int			open_mode;
	off_t		buffer_offset;
	void		*buffer;
};

static const uint32 kMaxAttributeSize = 65536;
static const uint32 kMaxAttributes = 64;


//	#pragma mark helper functions


/*!
	Determines if the attribute is shared among all devices or among
	all CDs in a specific device.
	We use this to share certain Tracker attributes.
*/
static bool
is_special_attribute(const char *name, attr_mode attrMode)
{
	if (attrMode == kDeviceAttributes) {
		static const char *kAttributes[] = {
			"_trk/windframe",
			"_trk/pinfo",
			"_trk/pinfo_le",
			NULL,
		};

		for (int32 i = 0; kAttributes[i]; i++) {
			if (!strcmp(name, kAttributes[i]))
				return true;
		}
	} else if (attrMode == kSharedAttributes) {
		static const char *kAttributes[] = {
			"_trk/columns",
			"_trk/columns_le",
			"_trk/viewstate",
			"_trk/viewstate_le",
			NULL,
		};

		for (int32 i = 0; kAttributes[i]; i++) {
			if (!strcmp(name, kAttributes[i]))
				return true;
		}
	}

	return false;
}


static void
write_line(int fd, const char *line)
{
	if (line == NULL)
		line = "";

	size_t length = strlen(line);
	write(fd, line, length);
	write(fd, "\n", 1);
}


static void
write_attributes(int fd, Inode *inode, attr_mode attrMode = kDiscIDAttributes)
{
	// count attributes

	AttributeList::ConstIterator iterator = inode->Attributes();
	uint32 count = 0;
	while (iterator.HasNext()) {
		Attribute *attribute = iterator.Next();
		if (attrMode == kDiscIDAttributes
			|| is_special_attribute(attribute->Name(), attrMode))
			count++;
	}

	// we're artificially limiting the attribute count per inode
	if (count > kMaxAttributes)
		count = kMaxAttributes;

	count = B_HOST_TO_BENDIAN_INT32(count);
	write(fd, &count, sizeof(uint32));

	// write attributes

	iterator.Rewind();

	while (iterator.HasNext()) {
		Attribute *attribute = iterator.Next();
		if (attrMode != kDiscIDAttributes
			&& !is_special_attribute(attribute->Name(), attrMode))
			continue;

		uint32 type = B_HOST_TO_BENDIAN_INT32(attribute->Type());
		write(fd, &type, sizeof(uint32));

		uint8 length = strlen(attribute->Name());
		write(fd, &length, 1);
		write(fd, attribute->Name(), length);

		uint32 size = B_HOST_TO_BENDIAN_INT32(attribute->Size());
		write(fd, &size, sizeof(uint32));
		if (size != 0)
			write(fd, attribute->Data(), attribute->Size());

		if (--count == 0)
			break;
	}
}


static bool
read_line(int fd, char *line, size_t length)
{
	bool first = true;
	size_t pos = 0;
	char c;

	while (read(fd, &c, 1) == 1) {
		first = false;

		if (c == '\n')
			break;
		if (pos < length)
			line[pos] = c;

		pos++;
	}

	if (pos >= length - 1)
		pos = length - 1;
	line[pos] = '\0';

	return !first;
}


static bool
read_attributes(int fd, Inode *inode)
{
	uint32 count;
	if (read(fd, &count, sizeof(uint32)) != (ssize_t)sizeof(uint32))
		return false;

	count = B_BENDIAN_TO_HOST_INT32(count);
dprintf("inode %s read %lu attrs\n", inode->Name(), count);
	if (count > kMaxAttributes)
		return false;

	for (uint32 i = 0; i < count; i++) {
		char name[B_ATTR_NAME_LENGTH + 1];
		uint32 type, size;
		uint8 length;
		if (read(fd, &type, sizeof(uint32)) != (ssize_t)sizeof(uint32)
			|| read(fd, &length, 1) != 1
			|| read(fd, name, length) != length
			|| read(fd, &size, sizeof(uint32)) != (ssize_t)sizeof(uint32))
			return false;

		type = B_BENDIAN_TO_HOST_INT32(type);
		size = B_BENDIAN_TO_HOST_INT32(size);
		name[length] = '\0';
dprintf("  type %08lx, size %lu, name %s\n", type, size, name);

		Attribute *attribute = new Attribute(name, type);
		if (attribute->SetSize(size) != B_OK
			|| inode->AddAttribute(attribute, true) != B_OK) {
			delete attribute;
		} else
			read(fd, attribute->Data(), size);
	}

	return true;
}


static void
fill_stat_buffer(Volume *volume, Inode *inode, Attribute *attribute,
	struct stat &stat)
{
	stat.st_dev = volume->ID();
	stat.st_ino = inode->ID();

	if (attribute != NULL) {
		stat.st_size = attribute->Size();
		stat.st_mode = S_ATTR | 0666;
		stat.st_type = attribute->Type();
	} else {
		stat.st_size = inode->Size();
		stat.st_mode = inode->Type();
		stat.st_type = 0;
	}

	stat.st_nlink = 1;
	stat.st_blksize = 2048;

	stat.st_uid = inode->UserID();
	stat.st_gid = inode->GroupID();

	stat.st_atime = time(NULL);
	stat.st_mtime = stat.st_ctime = inode->ModificationTime();
	stat.st_crtime = inode->CreationTime();
}


//	#pragma mark - Volume class


Volume::Volume(mount_id id)
	:
	fLock("cdda"),
	fDevice(-1),
	fID(id),
	fRootNode(NULL),
	fNextID(1),
	fName(NULL),
	fNumBlocks(0),
	fFirstEntry(NULL)
{
}


Volume::~Volume()
{
	_StoreAttributes();
	_StoreSharedAttributes();

	close(fDevice);

	// put_vnode on the root to release the ref to it
	if (fRootNode)
		put_vnode(ID(), fRootNode->ID());

	delete fRootNode;

	Inode *inode, *next;

	for (inode = fFirstEntry; inode != NULL; inode = next) {
		next = inode->Next();
		delete inode;
	}

	free(fName);
}


status_t 
Volume::InitCheck()
{
	if (fLock.InitCheck() < B_OK)
		return B_ERROR;

	return B_OK;
}


/*static*/ void
Volume::DetermineName(cdtext &text, char *name, size_t length)
{
	if (text.artist != NULL && text.album != NULL)
		snprintf(name, length, "%s - %s", text.artist, text.album);
	else if (text.artist != NULL || text.album != NULL) {
		snprintf(name, length, "%s", text.artist != NULL
			? text.artist : text.album);
	} else
		strlcpy(name, "Audio CD", length);
}


status_t
Volume::Mount(const char* device)
{
	fDevice = open(device, O_RDONLY);
	if (fDevice < 0)
		return errno;

	scsi_toc_toc *toc = (scsi_toc_toc *)malloc(1024);
	if (toc == NULL)
		return B_NO_MEMORY;

	status_t status = read_table_of_contents(fDevice, toc, 1024);
	if (status < B_OK) {
		free(toc);
		return status;
	}

	fDiscID = compute_cddb_disc_id(*toc);

	// create the root vnode
	fRootNode = _CreateNode(NULL, "", 0, 0, S_IFDIR | 0777);
	if (fRootNode == NULL)
		status = B_NO_MEMORY;
	if (status >= B_OK)
		status = publish_vnode(ID(), fRootNode->ID(), fRootNode);
	if (status < B_OK) {
		free(toc);
		return status;
	}

	cdtext text;
	if (read_cdtext(fDevice, text) < B_OK)
		dprintf("CDDA: no CD-Text found.\n");

	int32 trackCount = toc->last_track + 1 - toc->first_track;
	off_t totalFrames = 0;
	char title[256];

	for (int32 i = 0; i < trackCount; i++) {
		scsi_cd_msf& next = toc->tracks[i + 1].start.time;
			// the last track is always lead-out
		scsi_cd_msf& start = toc->tracks[i].start.time;
		int32 track = i + 1;

		off_t startFrame = start.minute * kFramesPerMinute
			+ start.second * kFramesPerSecond + start.frame;
		off_t frames = next.minute * kFramesPerMinute
			+ next.second * kFramesPerSecond + next.frame
			- startFrame;

		if (text.titles[i] != NULL) {
			if (text.artists[i] != NULL) {
				snprintf(title, sizeof(title), "%02ld. %s - %s.wav", track,
					text.artists[i], text.titles[i]);
			} else {
				snprintf(title, sizeof(title), "%02ld. %s.wav", track,
					text.titles[i]);
			}
		} else
			snprintf(title, sizeof(title), "%02ld.wav", track);

		// remove '/' and '\n' from title
		for (int32 j = 0; title[j]; j++) {
			if (title[j] == '/')
				title[j] = '-';
			else if (title[j] == '\n')
				title[j] = ' ';
		}

		totalFrames += frames;

		Inode *inode = _CreateNode(fRootNode, title, startFrame, frames,
			S_IFREG | 0444);
		if (inode == NULL)
			continue;

		// add attributes

		inode->AddAttribute("Audio:Artist", B_STRING_TYPE,
			text.artists[i] != NULL ? text.artists[i] : text.artist);
		inode->AddAttribute("Audio:Title", B_STRING_TYPE, text.titles[i]);
		inode->AddAttribute("Audio:Genre", B_STRING_TYPE, text.genre);
		inode->AddAttribute("Audio:Track", track);

		snprintf(title, sizeof(title), "%02lu:%02lu",
			uint32(inode->FrameCount() / kFramesPerMinute),
			uint32((inode->FrameCount() % kFramesPerMinute) / kFramesPerSecond));
		inode->AddAttribute("Audio:Length", B_STRING_TYPE, title);
		inode->AddAttribute("BEOS:TYPE", B_MIME_STRING_TYPE, "audio/x-wav");
	}

	_RestoreSharedAttributes();
	_RestoreAttributes();

	free(toc);

	// determine volume title
	DetermineName(text, title, sizeof(title));

	fName = strdup(title);
	if (fName == NULL)
		return B_NO_MEMORY;

	fNumBlocks = totalFrames;
	return B_OK;
}


Semaphore&
Volume::Lock()
{
	return fLock;
}


Inode *
Volume::_CreateNode(Inode *parent, const char *name, off_t start, off_t frames,
	int32 type)
{
	Inode *inode = new Inode(this, parent, name, start, frames, type);
	if (inode == NULL)
		return NULL;

	if (inode->InitCheck() != B_OK) {
		delete inode;
		return NULL;
	}

	if (S_ISREG(type)) {
		// we need to order it by track for compatibility with BeOS' cdda
		Inode *last = NULL, *current = fFirstEntry;
		while (current != NULL) {
			last = current;
			current = current->Next();
		}

		if (last)
			last->SetNext(inode);
		else
			fFirstEntry = inode;
	}

	return inode;
}


Inode *
Volume::Find(vnode_id id)
{
	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		if (inode->ID() == id)
			return inode;
	}

	return NULL;
}


Inode *
Volume::Find(const char *name)
{
	if (!strcmp(name, ".")
		|| !strcmp(name, ".."))
		return fRootNode;

	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		if (!strcmp(inode->Name(), name))
			return inode;
	}

	return NULL;
}


status_t
Volume::SetName(const char *name)
{
	if (name == NULL || !name[0])
		return B_BAD_VALUE;

	name = strdup(name);
	if (name == NULL)
		return B_NO_MEMORY;

	free(fName);
	fName = (char *)name;
	return B_OK;
}


/*!
	Opens the file that contains the volume and inode titles as well as all
	of their attributes.
	The attributes are stored in files below B_USER_SETTINGS_DIRECTORY/cdda.
*/
int
Volume::_OpenAttributes(int mode, enum attr_mode attrMode)
{
	char* path = (char*)malloc(B_PATH_NAME_LENGTH);
	if (path == NULL)
		return -1;

	bool create = (mode & O_WRONLY) != 0;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, create, path,
			B_PATH_NAME_LENGTH) != B_OK) {
		free(path);
		return -1;
	}

	strlcat(path, "/cdda", B_PATH_NAME_LENGTH);
	if (create)
		mkdir(path, 0755);

	if (attrMode == kDiscIDAttributes) {
		char id[64];
		snprintf(id, sizeof(id), "/%08lx", fDiscID);
		strlcat(path, id, B_PATH_NAME_LENGTH);
	} else if (attrMode == kDeviceAttributes) {
		uint32 length = strlen(path);
		char *device = path + length;
		if (ioctl(fDevice, B_GET_PATH_FOR_DEVICE, device,
				B_PATH_NAME_LENGTH - length) < B_OK) {
			free(path);
			return B_ERROR;
		}

		device++;

		// replace slashes in the device path
		while (device[0]) {
			if (device[0] == '/')
				device[0] = '_';

			device++;
		}
	} else
		strlcat(path, "/shared", B_PATH_NAME_LENGTH);

dprintf("PATH: %s\n", path);
	int fd = open(path, mode | (create ? O_CREAT | O_TRUNC : 0), 0644);

	free(path);
	return fd;
}


/*!
	Reads the attributes, if any, that belong to the CD currently being
	mounted.
*/
void
Volume::_RestoreAttributes()
{
	int fd = _OpenAttributes(O_RDONLY);
	if (fd < 0)
		return;

	char line[B_FILE_NAME_LENGTH];
	if (!read_line(fd, line, B_FILE_NAME_LENGTH)) {
		close(fd);
		return;
	}

dprintf("VOLUME %s\n", line);
	SetName(line);

	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		if (!read_line(fd, line, B_FILE_NAME_LENGTH))
			break;

		inode->SetName(line);
dprintf("INODE %s\n", line);
	}

	if (read_attributes(fd, fRootNode)) {
		for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
			if (!read_attributes(fd, inode))
				break;
		}
	}

	close(fd);
}


void
Volume::_StoreAttributes()
{
	int fd = _OpenAttributes(O_WRONLY);
	if (fd < 0)
		return;

	write_line(fd, Name());

	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		write_line(fd, inode->Name());
	}

	write_attributes(fd, fRootNode);

	for (Inode *inode = fFirstEntry; inode != NULL; inode = inode->Next()) {
		write_attributes(fd, inode);
	}

	close(fd);
}


/*!
	Restores the attributes, if any, that are shared between CDs; some are
	stored per device, others are stored for all CDs no matter which device.
*/
void
Volume::_RestoreSharedAttributes()
{
	// device attributes overwrite shared attributes

	int fd = _OpenAttributes(O_RDONLY, kSharedAttributes);
	if (fd >= 0) {
		read_attributes(fd, fRootNode);
		close(fd);
	}

	fd = _OpenAttributes(O_RDONLY, kDeviceAttributes);
	if (fd >= 0) {
		read_attributes(fd, fRootNode);
		close(fd);
	}
}


void
Volume::_StoreSharedAttributes()
{
	// write shared and device specific settings

	int fd = _OpenAttributes(O_WRONLY, kSharedAttributes);
	if (fd >= 0) {
		write_attributes(fd, fRootNode, kSharedAttributes);
		close(fd);
	}

	fd = _OpenAttributes(O_WRONLY, kDeviceAttributes);
	if (fd >= 0) {
		write_attributes(fd, fRootNode, kDeviceAttributes);
		close(fd);
	}
}


//	#pragma mark - Attribute class


Attribute::Attribute(const char *name, type_code type)
	:
	fName(NULL),
	fType(0),
	fData(NULL),
	fSize(0)
{
	SetTo(name, type);
}


Attribute::~Attribute()
{
	free(fName);
	free(fData);
}


status_t
Attribute::SetTo(const char *name, type_code type)
{
	if (name == NULL || !name[0])
		return B_BAD_VALUE;

	name = strdup(name);
	if (name == NULL)
		return B_NO_MEMORY;

	free(fName);

	fName = (char *)name;
	fType = type;
	return B_OK;
}


status_t
Attribute::ReadAt(off_t offset, uint8 *buffer, size_t *_length)
{
	size_t length = *_length;

	if (offset < 0)
		return B_BAD_VALUE;
	if (offset >= fSize) {
		*_length = 0;
		return B_OK;
	}
	if (offset + length > fSize)
		length = fSize - offset;

	if (user_memcpy(buffer, fData + offset, length) < B_OK)
		return B_BAD_ADDRESS;

	*_length = length;
	return B_OK;
}


/*!
	Writes to the attribute and enlarges it as needed.
	An attribute has a maximum size of 65536 bytes for now.
*/
status_t
Attribute::WriteAt(off_t offset, const uint8 *buffer, size_t *_length)
{
	size_t length = *_length;

	if (offset < 0)
		return B_BAD_VALUE;

	// we limit the attribute size to something reasonable
	off_t end = offset + length;
	if (end > kMaxAttributeSize) {
		end = kMaxAttributeSize;
		length = end - offset;
	}
	if (offset > end) {
		*_length = 0;
		return E2BIG;
	}

	if (end > fSize) {
		// make room in the data stream
		uint8 *data = (uint8 *)realloc(fData, end);
		if (data == NULL)
			return B_NO_MEMORY;

		if (fSize < offset)
			memset(data + fSize, 0, offset - fSize);

		fData = data;
		fSize = end;
	}

	if (user_memcpy(fData + offset, buffer, length) < B_OK)
		return B_BAD_ADDRESS;

	*_length = length;
	return B_OK;
}


//!	Removes all data from the attribute.
void
Attribute::Truncate()
{
	free(fData);
	fData = NULL;
	fSize = 0;
}


/*!
	Resizes the data part of an attribute to the requested amount \a size.
	An attribute has a maximum size of 65536 bytes for now.
*/
status_t
Attribute::SetSize(off_t size)
{
	if (size > kMaxAttributeSize)
		return E2BIG;

	uint8 *data = (uint8 *)realloc(fData, size);
	if (data == NULL)
		return B_NO_MEMORY;

	if (fSize < size)
		memset(data + fSize, 0, size - fSize);

	fData = data;
	fSize = size;
	return B_OK;
}


//	#pragma mark - Inode class


Inode::Inode(Volume *volume, Inode *parent, const char *name, off_t start,
		off_t frames, int32 type)
	:
	fNext(NULL)
{
	fName = strdup(name);
	if (fName == NULL)
		return;

	fID = volume->GetNextNodeID();
	fType = type;
	fStartFrame = start;
	fFrameCount = frames;

	fUserID = geteuid();
	fGroupID = parent ? parent->GroupID() : getegid();

	fCreationTime = fModificationTime = time(NULL);

	if (frames) {
		// initialize WAV header

		// RIFF header
		fWAVHeader.header.magic = B_HOST_TO_BENDIAN_INT32('RIFF');
		fWAVHeader.header.length = B_HOST_TO_LENDIAN_INT32(Size()
			+ sizeof(wav_header) - sizeof(riff_chunk));
		fWAVHeader.header.id = B_HOST_TO_BENDIAN_INT32('WAVE');

		// 'fmt ' format chunk
		fWAVHeader.format.fourcc = B_HOST_TO_BENDIAN_INT32('fmt ');
		fWAVHeader.format.length = B_HOST_TO_LENDIAN_INT32(
			sizeof(wav_format_chunk) - sizeof(riff_chunk));
		fWAVHeader.format.format_tag = B_HOST_TO_LENDIAN_INT16(1);
		fWAVHeader.format.channels = B_HOST_TO_LENDIAN_INT16(2);
		fWAVHeader.format.samples_per_second = B_HOST_TO_LENDIAN_INT32(44100);
		fWAVHeader.format.average_bytes_per_second = B_HOST_TO_LENDIAN_INT32(
			44100 * sizeof(uint16) * 2);
		fWAVHeader.format.block_align = B_HOST_TO_LENDIAN_INT16(4);
		fWAVHeader.format.bits_per_sample = B_HOST_TO_LENDIAN_INT16(16);

		// 'data' chunk
		fWAVHeader.data.fourcc = B_HOST_TO_BENDIAN_INT32('data');
		fWAVHeader.data.length = B_HOST_TO_LENDIAN_INT32(Size());
	}
}


Inode::~Inode()
{
	free(const_cast<char *>(fName));
}


status_t 
Inode::InitCheck()
{
	if (fName == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
Inode::SetName(const char* name)
{
	if (name == NULL || !name[0]
		|| strchr(name, '/') != NULL
		|| strchr(name, '\n') != NULL)
		return B_BAD_VALUE;

	name = strdup(name);
	if (name == NULL)
		return B_NO_MEMORY;

	free(fName);
	fName = (char *)name;
	return B_OK;
}


Attribute *
Inode::FindAttribute(const char *name) const
{
	if (name == NULL || !name[0])
		return NULL;

	AttributeList::ConstIterator iterator = fAttributes.GetIterator();

	while (iterator.HasNext()) {
		Attribute *attribute = iterator.Next();
		if (!strcmp(attribute->Name(), name))
			return attribute;
	}

	return NULL;
}


status_t
Inode::AddAttribute(Attribute *attribute, bool overwrite)
{
	Attribute *oldAttribute = FindAttribute(attribute->Name());
	if (oldAttribute != NULL) {
		if (!overwrite)
			return B_NAME_IN_USE;

		fAttributes.Remove(oldAttribute);
		delete oldAttribute;
	}

	fAttributes.Add(attribute);
	return B_OK;
}


status_t
Inode::AddAttribute(const char *name, type_code type,
	bool overwrite, const uint8 *data, size_t length)
{
	Attribute *attribute = new Attribute(name, type);
	status_t status = attribute != NULL ? B_OK : B_NO_MEMORY;
	if (status == B_OK)
		status = attribute->InitCheck();
	if (status == B_OK && data != NULL && length != 0)
		status = attribute->WriteAt(0, data, &length);
	if (status == B_OK)
		status = AddAttribute(attribute, overwrite);
	if (status < B_OK) {
		delete attribute;
		return status;
	}

	return B_OK;
}


status_t
Inode::AddAttribute(const char *name, type_code type, const char *string)
{
	if (string == NULL)
		return NULL;

	return AddAttribute(name, type, true, (const uint8 *)string,
		strlen(string));
}


status_t
Inode::AddAttribute(const char *name, int32 value)
{
	return AddAttribute(name, B_INT32_TYPE, true,
		(const uint8 *)&value, sizeof(int32));
}


status_t
Inode::RemoveAttribute(const char *name)
{
	if (name == NULL || !name[0])
		return B_ENTRY_NOT_FOUND;

	AttributeList::Iterator iterator = fAttributes.GetIterator();
	
	while (iterator.HasNext()) {
		Attribute *attribute = iterator.Next();
		if (!strcmp(attribute->Name(), name)) {
			// look for attribute in cookies
			AttrCookieList::Iterator i = fAttrCookies.GetIterator();
			while (i.HasNext()) {
				attr_cookie *cookie = i.Next();
				if (cookie->current == attribute)
					cookie->current = attribute->GetDoublyLinkedListLink()->next;
			}

			iterator.Remove();
			delete attribute;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


void
Inode::AddAttrCookie(attr_cookie *cookie)
{
	fAttrCookies.Add(cookie);
	RewindAttrCookie(cookie);
}


void
Inode::RemoveAttrCookie(attr_cookie *cookie)
{
	fAttrCookies.Remove(cookie);
}


void
Inode::RewindAttrCookie(attr_cookie *cookie)
{
	cookie->current = fAttributes.First();
}


//	#pragma mark - Module API


static float
cdda_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	scsi_toc_toc *toc = (scsi_toc_toc *)malloc(1024);
	if (toc == NULL)
		return B_NO_MEMORY;

	status_t status = read_table_of_contents(fd, toc, 1024);
	if (status < B_OK) {
		free(toc);
		return status;
	}

	*_cookie = toc;
	return 0.8f;
}


static status_t
cdda_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	scsi_toc_toc *toc = (scsi_toc_toc *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;

	// compute length

	uint32 lastTrack = toc->last_track + 1 - toc->first_track;
	scsi_cd_msf& end = toc->tracks[lastTrack].start.time;

	partition->content_size = off_t(end.minute * kFramesPerMinute
		+ end.second * kFramesPerSecond + end.frame) * kFrameSize;
	partition->block_size = kFrameSize;

	// determine volume title

	cdtext text;
	read_cdtext(fd, text);

	char name[256];
	Volume::DetermineName(text, name, sizeof(name));
	partition->content_name = strdup(name);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
cdda_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	free(_cookie);
}


static status_t
cdda_mount(mount_id id, const char *device, uint32 flags, const char *args,
	fs_volume *_volume, vnode_id *_rootVnodeID)
{
	TRACE(("cdda_mount: entry\n"));

	Volume *volume = new Volume(id);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status = volume->InitCheck();
	if (status == B_OK)
		status = volume->Mount(device);

	if (status < B_OK) {
		delete volume;
		return status;
	}

	*_rootVnodeID = volume->RootNode().ID();
	*_volume = volume;

	return B_OK;
}


static status_t
cdda_unmount(fs_volume _volume)
{
	struct Volume *volume = (struct Volume *)_volume;

	TRACE(("cdda_unmount: entry fs = %p\n", _volume));
	delete volume;

	return 0;
}


static status_t
cdda_read_fs_stat(fs_volume _volume, struct fs_info *info)
{
	Volume *volume = (Volume *)_volume;
	Locker locker(volume->Lock());

	// File system flags.
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR | B_FS_HAS_MIME
		| B_FS_IS_REMOVABLE;
	info->io_size = 65536;

	info->block_size = 2048;
	info->total_blocks = volume->NumBlocks();
	info->free_blocks = 0;

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	strlcpy(info->fsh_name, "cdda", sizeof(info->fsh_name));

	return B_OK;
}


static status_t
cdda_write_fs_stat(fs_volume _volume, const struct fs_info *info, uint32 mask)
{
	Volume *volume = (Volume *)_volume;
	Locker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & FS_WRITE_FSINFO_NAME)
		status = volume->SetName(info->volume_name);

	return status;
}


static status_t
cdda_sync(fs_volume fs)
{
	TRACE(("cdda_sync: entry\n"));

	return 0;
}


static status_t
cdda_lookup(fs_volume _volume, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	Volume *volume = (Volume *)_volume;
	status_t status;

	TRACE(("cdda_lookup: entry dir %p, name '%s'\n", _dir, name));

	Inode *directory = (Inode *)_dir;
	if (!S_ISDIR(directory->Type()))
		return B_NOT_A_DIRECTORY;

	Locker _(volume->Lock());

	Inode *inode = volume->Find(name);
	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	Inode *dummy;
	status = get_vnode(volume->ID(), inode->ID(), (fs_vnode *)&dummy);
	if (status < B_OK)
		return status;

	*_id = inode->ID();
	*_type = inode->Type();
	return B_OK;
}


static status_t
cdda_get_vnode_name(fs_volume _volume, fs_vnode _node, char *buffer, size_t bufferSize)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("cdda_get_vnode_name(): inode = %p\n", inode));

	Locker _(volume->Lock());
	strlcpy(buffer, inode->Name(), bufferSize);
	return B_OK;
}


static status_t
cdda_get_vnode(fs_volume _volume, vnode_id id, fs_vnode *_inode, bool reenter)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode;

	TRACE(("cdda_getvnode: asking for vnode 0x%Lx, r %d\n", id, reenter));

	inode = volume->Find(id);
	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	*_inode = inode;
	return B_OK;
}


static status_t
cdda_put_vnode(fs_volume _volume, fs_vnode _node, bool reenter)
{
	return B_OK;
}


static status_t
cdda_open(fs_volume _volume, fs_vnode _node, int openMode, fs_cookie *_cookie)
{
	TRACE(("cdda_open(): node = %p, openMode = %d\n", _node, openMode));

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	TRACE(("  open cookie = %p\n", cookie));
	cookie->open_mode = openMode;
	cookie->buffer = NULL;

	*_cookie = (void *)cookie;

	return B_OK;
}


static status_t
cdda_close(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	return B_OK;
}


static status_t
cdda_free_cookie(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	file_cookie *cookie = (file_cookie *)_cookie;

	TRACE(("cdda_freecookie: entry vnode %p, cookie %p\n", _node, _cookie));

	free(cookie);
	return B_OK;
}


static status_t
cdda_fsync(fs_volume _volume, fs_vnode _v)
{
	return B_OK;
}


static status_t
cdda_read(fs_volume _volume, fs_vnode _node, fs_cookie _cookie, off_t offset,
	void *buffer, size_t *_length)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("cdda_read(vnode = %p, offset %Ld, length = %lu, mode = %d)\n",
		_node, offset, *_length, cookie->open_mode));

	if (S_ISDIR(inode->Type()))
		return B_IS_A_DIRECTORY;
	if (offset < 0)
		return B_BAD_VALUE;

	off_t maxSize = inode->Size() + sizeof(wav_header);
	if (offset >= maxSize) {
		*_length = 0;
		return B_OK;
	}

	if (cookie->buffer == NULL) {
		// TODO: move that to open() to make sure reading can't fail for this reason?
		cookie->buffer = malloc(volume->BufferSize());
		if (cookie->buffer == NULL)
			return B_NO_MEMORY;

		cookie->buffer_offset = -1;
	}

	size_t length = *_length;
	if (offset + length > maxSize)
		length = maxSize - offset;

	status_t status = B_OK;

	if (offset < sizeof(wav_header)) {
		// read fake WAV header
		size_t size = sizeof(wav_header) - offset;
		size = min_c(size, length);

		if (user_memcpy(buffer, (uint8 *)inode->WAVHeader() + offset, size) < B_OK)
			return B_BAD_ADDRESS;

		buffer = (void *)((uint8 *)buffer + size);
		length -= size;
		offset = 0;
	} else
		offset -= sizeof(wav_header);

	if (length > 0) {
		// read actual CD data
		offset += inode->StartFrame() * kFrameSize;

		status = read_cdda_data(volume->Device(), offset, buffer, length,
			cookie->buffer_offset, cookie->buffer, volume->BufferSize());
	}
	if (status == B_OK)
		*_length = length;

	return status;
}


static bool
cdda_can_page(fs_volume _volume, fs_vnode _v, fs_cookie cookie)
{
	return false;
}


static status_t
cdda_read_pages(fs_volume _volume, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return EPERM;
}


static status_t
cdda_write_pages(fs_volume _volume, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return EPERM;
}


static status_t
cdda_read_stat(fs_volume _volume, fs_vnode _node, struct stat *stat)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("cdda_read_stat: vnode %p (0x%Lx), stat %p\n", inode, inode->ID(), stat));

	fill_stat_buffer(volume, inode, NULL, *stat);

	return B_OK;
}


status_t 
cdda_rename(fs_volume _volume, void *_oldDir, const char *oldName, void *_newDir,
	const char *newName)
{
	if (_volume == NULL || _oldDir == NULL || _newDir == NULL
		|| oldName == NULL || *oldName == '\0'
		|| newName == NULL || *newName == '\0'
		|| !strcmp(oldName, ".") || !strcmp(oldName, "..")
		|| !strcmp(newName, ".") || !strcmp(newName, "..")
		|| strchr(newName, '/') != NULL)
		return B_BAD_VALUE;

	// we only have a single directory which simplifies things a bit :-)

	Volume *volume = (Volume *)_volume;
	Locker _(volume->Lock());

	Inode *inode = volume->Find(oldName);
	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	if (volume->Find(newName) != NULL)
		return B_NAME_IN_USE;

	return inode->SetName(newName);
}


//	#pragma mark - directory functions


static status_t
cdda_open_dir(fs_volume _volume, fs_vnode _node, fs_cookie *_cookie)
{
	Volume *volume = (Volume *)_volume;

	TRACE(("cdda_open_dir(): vnode = %p\n", _node));

	Inode *inode = (Inode *)_node;
	if (!S_ISDIR(inode->Type()))
		return B_BAD_VALUE;

	if (inode != &volume->RootNode())
		panic("pipefs: found directory that's not the root!");

	dir_cookie *cookie = (dir_cookie *)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->current = volume->FirstEntry();
	cookie->state = ITERATION_STATE_BEGIN;

	*_cookie = (void *)cookie;
	return B_OK;
}


static status_t
cdda_read_dir(fs_volume _volume, fs_vnode _node, fs_cookie _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;
	status_t status = 0;

	TRACE(("cdda_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _node, _cookie, dirent, bufferSize,_num));

	if (_node != &volume->RootNode())
		return B_BAD_VALUE;

	Locker _(volume->Lock());

	dir_cookie *cookie = (dir_cookie *)_cookie;
	Inode *childNode = NULL;
	const char *name = NULL;
	Inode *nextChildNode = NULL;
	int nextState = cookie->state;

	switch (cookie->state) {
		case ITERATION_STATE_DOT:
			childNode = inode;
			name = ".";
			nextChildNode = volume->FirstEntry();
			nextState = cookie->state + 1;
			break;
		case ITERATION_STATE_DOT_DOT:
			childNode = inode; // parent of the root node is the root node
			name = "..";
			nextChildNode = volume->FirstEntry();
			nextState = cookie->state + 1;
			break;
		default:
			childNode = cookie->current;
			if (childNode) {
				name = childNode->Name();
				nextChildNode = childNode->Next();
			}
			break;
	}

	if (!childNode) {
		// we're at the end of the directory
		*_num = 0;
		return B_OK;
	}

	dirent->d_dev = volume->ID();
	dirent->d_ino = inode->ID();
	dirent->d_reclen = strlen(name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize)
		return ENOBUFS;

	status = user_strlcpy(dirent->d_name, name, bufferSize);
	if (status < B_OK)
		return status;

	cookie->current = nextChildNode;
	cookie->state = nextState;
	return B_OK;
}


static status_t
cdda_rewind_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	Volume *volume = (Volume *)_volume;

	dir_cookie *cookie = (dir_cookie *)_cookie;
	cookie->current = volume->FirstEntry();
	cookie->state = ITERATION_STATE_BEGIN;

	return B_OK;
}


static status_t
cdda_close_dir(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	TRACE(("cdda_close: entry vnode %p, cookie %p\n", _node, _cookie));

	return 0;
}


static status_t
cdda_free_dir_cookie(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	dir_cookie *cookie = (dir_cookie *)_cookie;

	TRACE(("cdda_freecookie: entry vnode %p, cookie %p\n", _vnode, cookie));

	free(cookie);
	return 0;
}


//	#pragma mark - attribute functions


static status_t
cdda_open_attr_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie *_cookie)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_vnode;

	attr_cookie *cookie = new attr_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	Locker _(volume->Lock());

	inode->AddAttrCookie(cookie);
	*_cookie = cookie;
	return B_OK;
}


static status_t
cdda_close_attr_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	return B_OK;
}


static status_t
cdda_free_attr_dir_cookie(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_vnode;
	attr_cookie *cookie = (attr_cookie *)_cookie;

	Locker _(volume->Lock());

	inode->RemoveAttrCookie(cookie);
	delete cookie;
	return B_OK;
}


static status_t
cdda_rewind_attr_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_vnode;
	attr_cookie *cookie = (attr_cookie *)_cookie;

	Locker _(volume->Lock());

	inode->RewindAttrCookie(cookie);
	return B_OK;
}


static status_t
cdda_read_attr_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_vnode;
	attr_cookie *cookie = (attr_cookie *)_cookie;

	Locker _(volume->Lock());
	Attribute *attribute = cookie->current;

	if (attribute == NULL) {
		*_num = 0;
		return B_OK;
	}

	size_t length = strlcpy(dirent->d_name, attribute->Name(), bufferSize);
	dirent->d_dev = volume->ID();
	dirent->d_ino = inode->ID();
	dirent->d_reclen = sizeof(struct dirent) + length;

	cookie->current = attribute->GetDoublyLinkedListLink()->next;
	*_num = 1;
	return B_OK;
}


static status_t
cdda_create_attr(fs_volume _volume, fs_vnode _node, const char *name,
	uint32 type, int openMode, fs_cookie *_cookie)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	Locker _(volume->Lock());

	Attribute *attribute = inode->FindAttribute(name);
	if (attribute == NULL) {
		status_t status = inode->AddAttribute(name, type);
		if (status < B_OK)
			return status;
	} else if ((openMode & O_EXCL) == 0) {
		attribute->SetType(type);
		if ((openMode & O_TRUNC) != 0)
			attribute->Truncate();
	} else
		return B_FILE_EXISTS;

	*_cookie = strdup(name);
	if (*_cookie == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static status_t
cdda_open_attr(fs_volume _volume, fs_vnode _node, const char *name,
	int openMode, fs_cookie *_cookie)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	Locker _(volume->Lock());

	Attribute *attribute = inode->FindAttribute(name);
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	*_cookie = strdup(name);
	if (*_cookie == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static status_t
cdda_close_attr(fs_volume _fs, fs_vnode _file, fs_cookie cookie)
{
	return B_OK;
}


static status_t
cdda_free_attr_cookie(fs_volume _fs, fs_vnode _file, fs_cookie cookie)
{
	free(cookie);
	return B_OK;
}


static status_t
cdda_read_attr(fs_volume _volume, fs_vnode _node, fs_cookie _cookie,
	off_t offset, void *buffer, size_t *_length)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	Locker _(volume->Lock());

	Attribute *attribute = inode->FindAttribute((const char *)_cookie);
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	return attribute->ReadAt(offset, (uint8 *)buffer, _length);
}


static status_t
cdda_write_attr(fs_volume _volume, fs_vnode _file, fs_cookie _cookie,
	off_t offset, const void *buffer, size_t *_length)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_file;

	Locker _(volume->Lock());

	Attribute *attribute = inode->FindAttribute((const char *)_cookie);
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	return attribute->WriteAt(offset, (uint8 *)buffer, _length);
}


static status_t
cdda_read_attr_stat(fs_volume _volume, fs_vnode _file, fs_cookie _cookie,
	struct stat *stat)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_file;

	Locker _(volume->Lock());

	Attribute *attribute = inode->FindAttribute((const char *)_cookie);
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	fill_stat_buffer(volume, inode, attribute, *stat);
	return B_OK;
}


static status_t
cdda_write_attr_stat(fs_volume _volume, fs_vnode file, fs_cookie cookie,
	const struct stat *stat, int statMask)
{
	return EOPNOTSUPP;
}


static status_t
cdda_remove_attr(fs_volume _volume, fs_vnode _node, const char *name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	Locker _(volume->Lock());

	return inode->RemoveAttribute(name);
}


static status_t
cdda_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


static file_system_module_info sCDDAFileSystem = {
	{
		"file_systems/cdda" B_CURRENT_FS_API_VERSION,
		0,
		cdda_std_ops,
	},

	"CDDA File System",

	cdda_identify_partition,
	cdda_scan_partition,
	cdda_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	cdda_mount,
	cdda_unmount,
	cdda_read_fs_stat,
	cdda_write_fs_stat,
	cdda_sync,

	cdda_lookup,
	cdda_get_vnode_name,

	cdda_get_vnode,
	cdda_put_vnode,
	NULL,	// fs_remove_vnode()

	cdda_can_page,
	cdda_read_pages,
	cdda_write_pages,

	NULL,	// get_file_map()

	// common
	NULL,	// fs_ioctl()
	NULL,	// fs_set_flags()
	NULL,	// fs_select()
	NULL,	// fs_deselect()
	cdda_fsync,

	NULL,	// fs_read_link()
	NULL,	// fs_symlink()
	NULL,	// fs_link()
	NULL,	// fs_unlink()
	cdda_rename,

	NULL,	// fs_access()
	cdda_read_stat,
	NULL,	// fs_write_stat()

	// file
	NULL,	// fs_create()
	cdda_open,
	cdda_close,
	cdda_free_cookie,
	cdda_read,
	NULL,	// fs_write()

	// directory
	NULL,	// fs_create_dir()
	NULL,	// fs_remove_dir()
	cdda_open_dir,
	cdda_close_dir,
	cdda_free_dir_cookie,
	cdda_read_dir,
	cdda_rewind_dir,

	// attribute directory operations
	cdda_open_attr_dir,
	cdda_close_attr_dir,
	cdda_free_attr_dir_cookie,
	cdda_read_attr_dir,
	cdda_rewind_attr_dir,

	// attribute operations
	cdda_create_attr,
	cdda_open_attr,
	cdda_close_attr,
	cdda_free_attr_cookie,
	cdda_read_attr,
	cdda_write_attr,

	cdda_read_attr_stat,
	cdda_write_attr_stat,
	NULL,	// fs_rename_attr()
	cdda_remove_attr,

	// the other operations are not yet supported (indices, queries)
	NULL,
};

module_info *modules[] = {
	(module_info *)&sCDDAFileSystem,
	NULL,
};
