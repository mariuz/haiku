//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <ddm_userland_interface.h>
#include <DiskSystem.h>
#include <Partition.h>

// constructor
BDiskSystem::BDiskSystem()
	: fID(B_NO_INIT),
	  fName(),
	  fPrettyName(),
	  fFileSystem(false)
{
}

// destructor
BDiskSystem::~BDiskSystem()
{
}

// InitCheck
status_t
BDiskSystem::InitCheck() const
{
	return (fID > 0 ? B_OK : fID);
}

// Name
const char *
BDiskSystem::Name() const
{
	return fName.String();
}

// PrettyName
const char *
BDiskSystem::PrettyName() const
{
	return fPrettyName.String();
}

// SupportsDefragmenting
bool
BDiskSystem::SupportsDefragmenting(BPartition *partition,
								   bool *whileMounted) const
{
	return (InitCheck() == B_OK && IsFileSystem()
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_defragmenting_partition(fID,
					partition->_ShadowID(), whileMounted));
}

// SupportsRepairing
bool
BDiskSystem::SupportsRepairing(BPartition *partition, bool checkOnly,
							   bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_repairing_partition(fID, partition->_ShadowID(),
												  checkOnly, whileMounted));
}

// SupportsResizing
bool
BDiskSystem::SupportsResizing(BPartition *partition, bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_resizing_partition(fID, partition->_ShadowID(),
												 whileMounted));
}

// SupportsResizingChild
bool
BDiskSystem::SupportsResizingChild(BPartition *child) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_resizing_child_partition(fID,
													   child->_ShadowID()));
}

// SupportsMoving
bool
BDiskSystem::SupportsMoving(BPartition *partition, bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_moving_partition(fID, partition->_ShadowID(),
											   whileMounted));
}

// SupportsMovingChild
bool
BDiskSystem::SupportsMovingChild(BPartition *child) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_moving_child_partition(fID, child->_ShadowID()));
}

// SupportsSettingName
bool
BDiskSystem::SupportsSettingName(BPartition *partition) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& partition && partition->_IsShadow() && partition->Parent()
			&& partition->Parent()->_DiskSystem() == fID
			&& _kern_supports_setting_partition_name(fID,
													 partition->_ShadowID()));
}

// SupportsSettingContentName
bool
BDiskSystem::SupportsSettingContentName(BPartition *partition,
										bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_setting_partition_content_name(fID,
					partition->_ShadowID(), whileMounted));
}

// SupportsSettingType
bool
BDiskSystem::SupportsSettingType(BPartition *partition) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& partition && partition->_IsShadow() && partition->Parent()
			&& partition->Parent()->_DiskSystem() == fID
			&& _kern_supports_setting_partition_type(fID,
													 partition->_ShadowID()));
}

// SupportsCreatingChild
bool
BDiskSystem::SupportsCreatingChild(BPartition *partition) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_creating_child_partition(fID,
					partition->_ShadowID()));
}

// SupportsDeletingChild
bool
BDiskSystem::SupportsDeletingChild(BPartition *child) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_deleting_child_partition(fID,
													   child->_ShadowID()));
}

// SupportsInitializing
bool
BDiskSystem::SupportsInitializing(BPartition *partition) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& _kern_supports_initializing_partition(fID,
				partition->_ShadowID()));
}

// SupportsInitializingChild
bool
BDiskSystem::SupportsInitializingChild(BPartition *child,
									   const char *diskSystem) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem() && diskSystem
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_initializing_child_partition(fID,
					child->_ShadowID(), diskSystem));
}

// ValidateResize
status_t
BDiskSystem::ValidateResize(BPartition *partition, off_t *size) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!size || !partition || !partition->_IsShadow()
		|| partition->_DiskSystem() != fID) {
		return B_BAD_VALUE;
	}
	return _kern_validate_resize_partition(fID, partition->_ShadowID(), size);
}

// ValidateResizeChild
status_t
BDiskSystem::ValidateResizeChild(BPartition *child, off_t *size) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!size || !child || !child->_IsShadow() || !child->Parent()
		|| child->Parent()->_DiskSystem() != fID || !IsPartitioningSystem()) {
		return B_BAD_VALUE;
	}
	return _kern_validate_resize_child_partition(fID, child->_ShadowID(),
												 size);
}

// ValidateMove
status_t
BDiskSystem::ValidateMove(BPartition *partition, off_t *start) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!start || !partition || !partition->_IsShadow()
		|| partition->_DiskSystem() != fID) {
		return B_BAD_VALUE;
	}
	return _kern_validate_move_partition(fID, partition->_ShadowID(), start);
}

// ValidateMoveChild
status_t
BDiskSystem::ValidateMoveChild(BPartition *child, off_t *start) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!start || !child || !child->_IsShadow() || !child->Parent()
		|| child->Parent()->_DiskSystem() != fID || !IsPartitioningSystem()) {
		return B_BAD_VALUE;
	}
	return _kern_validate_move_child_partition(fID, child->_ShadowID(), start);
}

// ValidateSetName
status_t
BDiskSystem::ValidateSetName(BPartition *partition, char *name) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!name || !partition || !partition->_IsShadow() || !partition->Parent()
		|| partition->Parent()->_DiskSystem() != fID
		|| !IsPartitioningSystem()) {
		return B_BAD_VALUE;
	}
	return _kern_validate_set_partition_name(fID, partition->_ShadowID(),
											 name);
}

// ValidateSetContentName
status_t
BDiskSystem::ValidateSetContentName(BPartition *partition, char *name) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!name || !partition || !partition->_IsShadow()
		|| partition->_DiskSystem() != fID) {
		return B_BAD_VALUE;
	}
	return _kern_validate_set_partition_content_name(fID,
				partition->_ShadowID(), name);
}

// ValidateSetType
status_t
BDiskSystem::ValidateSetType(BPartition *partition, const char *type) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!type || !partition || !partition->_IsShadow() || !partition->Parent()
		|| partition->Parent()->_DiskSystem() != fID
		|| !IsPartitioningSystem()) {
		return B_BAD_VALUE;
	}
	return _kern_validate_set_partition_type(fID, partition->_ShadowID(),
											 type);
}

// ValidateCreateChild
status_t
BDiskSystem::ValidateCreateChild(BPartition *partition, off_t *start,
								 off_t *size, const char *type,
								 const char *parameters) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
// parameter may be NULL
	if (!start || !size || !type || !partition || !partition->_IsShadow()
		|| partition->_DiskSystem() != fID || !IsPartitioningSystem()) {
		return B_BAD_VALUE;
	}
	return _kern_validate_create_child_partition(fID, partition->_ShadowID(),
				start, size, type, parameters);
}

// ValidateInitialize
status_t
BDiskSystem::ValidateInitialize(BPartition *partition, char *name,
								const char *parameters) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
// parameters may be NULL
	if (!name || !partition || !partition->_IsShadow())
		return B_BAD_VALUE;
	return _kern_validate_initialize_partition(fID, partition->_ShadowID(),
											   name, parameters);
}

// GetNextSupportedType
status_t
BDiskSystem::GetNextSupportedType(BPartition *partition, int32 *cookie,
								  char *type) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!cookie || !type || !partition || !partition->_IsShadow()
		|| !partition->Parent() || partition->Parent()->_DiskSystem() != fID
		|| !IsPartitioningSystem()) {
		return B_BAD_VALUE;
	}
	return _kern_get_next_supported_partition_type(fID, partition->_ShadowID(),
												   cookie, type);
}

// GetTypeForContentType
status_t
BDiskSystem::GetTypeForContentType(const char *contentType, char *type) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!contentType || !type || !IsPartitioningSystem())
		return B_BAD_VALUE;
	return _kern_get_partition_type_for_content_type(fID, contentType, type);
}

// IsPartitioningSystem
bool
BDiskSystem::IsPartitioningSystem() const
{
	return (InitCheck() == B_OK && !fFileSystem);
}

// IsFileSystem
bool
BDiskSystem::IsFileSystem() const
{
	return (InitCheck() == B_OK && fFileSystem);
}

// IsSubSystemFor
bool
BDiskSystem::IsSubSystemFor(BPartition *parent) const
{
	return (InitCheck() == B_OK
			&& parent && parent->_IsShadow()
			&& _kern_is_sub_disk_system_for(fID, parent->_ShadowID()));
}

// _SetTo
status_t
BDiskSystem::_SetTo(disk_system_id id)
{
	_Unset();
	if (id < 0)
		return fID;
	user_disk_system_info info;
	status_t error = _kern_get_disk_system_info(id, &info);
	if (error != B_OK)
		return (fID = error);
	return _SetTo(&info);
}

// _SetTo
status_t
BDiskSystem::_SetTo(user_disk_system_info *info)
{
	_Unset();
	if (!info)
		return (fID = B_BAD_VALUE);
	fID = info->id;
	fName = info->name;
	fPrettyName = info->pretty_name;
	fFileSystem = info->file_system;
	return B_OK;
}

// _Unset
void
BDiskSystem::_Unset()
{
	fID = B_NO_INIT;
	fName = (const char*)NULL;
	fPrettyName = (const char*)NULL;
	fFileSystem = false;
}

