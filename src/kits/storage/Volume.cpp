// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		Volume.cpp
//
//	Description:	BVolume class
// ----------------------------------------------------------------------
/*!
	\file Volume.h
	BVolume implementation.
*/

#include <errno.h>

#include <Bitmap.h>
#include <Directory.h>
#include <fs_info.h>
#include <kernel_interface.h>
#include <Node.h>
#include <Volume.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

/*!
	\class BVolume
	\brief Represents a disk volume
	
	Provides an interface for querying information about a volume.

	The class is a simple wrapper for a \c dev_t and the function
	fs_stat_dev. The only exception is the method is SetName(), which
	sets the name of the volume.

	\author Vincent Dominguez
	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/

/*!	\var dev_t BVolume::fDevice
	\brief The volume's device ID.
*/

/*!	\var dev_t BVolume::fCStatus
	\brief The object's initialization status.
*/

// constructor
/*!	\brief Creates an uninitialized BVolume.

	InitCheck() will return \c B_NO_INIT.
*/
BVolume::BVolume()
	: fDevice(-1),
	  fCStatus(B_NO_INIT)
{
}

// constructor
/*!	\brief Creates a BVolume and initializes it to the volume specified
		   by the supplied device ID.

	InitCheck() should be called to check whether the initialization was
	successful.

	\param device The device ID of the volume.
*/
BVolume::BVolume(dev_t device)
	: fDevice(-1),
	  fCStatus(B_NO_INIT)
{
	SetTo(device);
}

// copy constructor
/*!	\brief Creates a BVolume and makes it a clone of the supplied one.

	Afterwards the object refers to the same device the supplied object
	does. If the latter is not properly initialized, this object isn't
	either.

	\param volume The volume object to be cloned.
*/
BVolume::BVolume(const BVolume &volume)
	: fDevice(volume.fDevice),
	  fCStatus(volume.fCStatus)
{
}

// destructor
/*!	\brief Frees all resources associated with the object.

	Does nothing.
*/
BVolume::~BVolume()
{
}

// InitCheck
/*!	\brief Returns the result of the last initialization.
	\return
	- \c B_OK: The object is properly initialized.
	- an error code otherwise
*/
status_t
BVolume::InitCheck(void) const
{	
	return fCStatus;
}

// SetTo
/*!	\brief Re-initializes the object to refer to the volume specified by
		   the supplied device ID.
	\param device The device ID of the volume.
	\param
	- \c B_OK: Everything went fine.
	- an error code otherwise
*/
status_t
BVolume::SetTo(dev_t device)
{
	// uninitialize
	Unset();
	// check the parameter
	status_t error = (device >= 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fs_info info;
		if (fs_stat_dev(device, &info) != 0)
			error = errno;
	}
	// set the new value
	if (error == B_OK)	
		fDevice = device;
	// set the init status variable
	fCStatus = error;
	return fCStatus;
}

// Unset
/*!	\brief Uninitialized the BVolume.
*/
void
BVolume::Unset()
{
	fDevice = -1;
	fCStatus = B_NO_INIT;
}

// Device
/*!	\brief Returns the device ID of the volume the object refers to.
	\return Returns the device ID of the volume the object refers to
			or -1, if the object is not properly initialized.
*/
dev_t
BVolume::Device() const 
{
	return fDevice;
}

// GetRootDirectory
/*!	\brief Returns the root directory of the volume referred to by the object.
	\param directory A pointer to a pre-allocated BDirectory to be initialized
		   to the volume's root directory.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a directory or the object is not properly
	  initialized.
	- another error code
*/
status_t
BVolume::GetRootDirectory(BDirectory *directory) const
{
	// check parameter and initialization
	status_t error = (directory && InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	// init the directory
	if (error == B_OK) {
		node_ref ref;
		ref.device = info.dev;
		ref.node = info.root;
		error = directory->SetTo(&ref);
	}
	return error;
}

// Capacity
/*!	\brief Returns the volume's total storage capacity.
	\return
	- The volume's total storage capacity (in bytes), when the object is
	  properly initialized.
	- \c B_BAD_VALUE otherwise.
*/
off_t
BVolume::Capacity() const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK ? info.total_blocks * info.block_size : error);
}

// FreeBytes
/*!	\brief Returns the amount of storage that's currently unused on the
		   volume (in bytes).
	\return
	- The amount of storage that's currently unused on the volume (in bytes),
	  when the object is properly initialized.
	- \c B_BAD_VALUE otherwise.
*/
off_t
BVolume::FreeBytes() const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK ? info.free_blocks * info.block_size : error);
}

// GetName
/*!	\brief Returns the name of the volume.

	The name of the volume is copied into the provided buffer.

	\param name A pointer to a pre-allocated character buffer of size
		   \c B_FILE_NAME_LENGTH or larger into which the name of the
		   volume shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name or the object is not properly
	  initialized.
	- another error code
*/
status_t
BVolume::GetName(char *name) const
{
	// check parameter and initialization
	status_t error = (name && InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	// copy the name
	if (error == B_OK)
		strncpy(name, info.volume_name, B_FILE_NAME_LENGTH);
	return error;
}

// SetName
/*!	\brief Sets the name of the volume referred to by this object.
	\param name The volume's new name. Must not be longer than
		   \c B_FILE_NAME_LENGTH (including the terminating null).
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name or the object is not properly
	  initialized.
	- another error code
*/
status_t
BVolume::SetName(const char *name)
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = BPrivate::Storage::set_volume_name(fDevice, name);
	return error;
}

// GetIcon
/*!	\brief Returns the icon of the volume.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.
*/
status_t
BVolume::GetIcon(BBitmap *icon, icon_size which) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	// get the icon
	if (error == B_OK)
		error = get_device_icon(info.device_name, icon, which);
	return error;
}

// IsRemovable
/*!	\brief Returns whether the volume is removable.
	\return \c true, when the object is properly initialized and the
	referred to volume is removable, \c false otherwise.
*/
bool
BVolume::IsRemovable() const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_REMOVABLE));
}

// IsReadOnly
/*!	\brief Returns whether the volume is read only.
	\return \c true, when the object is properly initialized and the
	referred to volume is read only, \c false otherwise.
*/
bool
BVolume::IsReadOnly(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_READONLY));
}

// IsPersistent
/*!	\brief Returns whether the volume is persistent.
	\return \c true, when the object is properly initialized and the
	referred to volume is persistent, \c false otherwise.
*/
bool
BVolume::IsPersistent(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_PERSISTENT));
}

// IsShared
/*!	\brief Returns whether the volume is shared.
	\return \c true, when the object is properly initialized and the
	referred to volume is shared, \c false otherwise.
*/
bool
BVolume::IsShared(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_IS_SHARED));
}

// KnowsMime
/*!	\brief Returns whether the volume supports MIME types.
	\return \c true, when the object is properly initialized and the
	referred to volume supports MIME types, \c false otherwise.
*/
bool
BVolume::KnowsMime(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_HAS_MIME));
}

// KnowsAttr
/*!	\brief Returns whether the volume supports attributes.
	\return \c true, when the object is properly initialized and the
	referred to volume supports attributes, \c false otherwise.
*/
bool
BVolume::KnowsAttr(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_HAS_ATTR));
}

// KnowsQuery
/*!	\brief Returns whether the volume supports queries.
	\return \c true, when the object is properly initialized and the
	referred to volume supports queries, \c false otherwise.
*/
bool
BVolume::KnowsQuery(void) const
{
	// check initialization
	status_t error = (InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get FS stat
	fs_info info;
	if (error == B_OK && fs_stat_dev(fDevice, &info) != 0)
		error = errno;
	return (error == B_OK && (info.flags & B_FS_HAS_QUERY));
}

// ==
/*!	\brief Returns whether two BVolume objects are equal.

	Two volume objects are said to be equal, if they either are both
	uninitialized, or both are initialized and refer to the same volume.

	\param volume The object to be compared with.
	\result \c true, if this object and the supplied one are equal, \c false
			otherwise.
*/
bool
BVolume::operator==(const BVolume &volume) const
{
	return (InitCheck() != B_OK && volume.InitCheck() != B_OK
			|| fDevice == volume.fDevice);
}

// !=
/*!	\brief Returns whether two BVolume objects are unequal.

	Two volume objects are said to be equal, if they either are both
	uninitialized, or both are initialized and refer to the same volume.

	\param volume The object to be compared with.
	\result \c true, if this object and the supplied one are unequal, \c false
			otherwise.
*/
bool
BVolume::operator!=(const BVolume &volume) const
{
	return !(*this == volume);
}

// =
/*!	\brief Assigns another BVolume object to this one.

	This object is made an exact clone of the supplied one.

	\param volume The volume from which shall be assigned.
	\return A reference to this object.
*/
BVolume&
BVolume::operator=(const BVolume &volume)
{
	if (&volume != this) {
		this->fDevice = volume.fDevice;
		this->fCStatus = volume.fCStatus;
	}
	return *this;
}


// FBC 
void BVolume::_ReservedVolume1() {} 
void BVolume::_ReservedVolume2() {} 
void BVolume::_ReservedVolume3() {} 
void BVolume::_ReservedVolume4() {} 
void BVolume::_ReservedVolume5() {} 
void BVolume::_ReservedVolume6() {} 
void BVolume::_ReservedVolume7() {} 
void BVolume::_ReservedVolume8() {}

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif
