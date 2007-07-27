// KPartition.h

#ifndef _K_DISK_DEVICE_SHADOW_PARTITION_H
#define _K_DISK_DEVICE_SHADOW_PARTITION_H

#include <KPartition.h>
#include <KPartitionListener.h>

namespace BPrivate {
namespace DiskDevice {

class KPhysicalPartition;

///	\brief Class representing a shadow of an existing partition.
///
///	See \ref path_kernel_structures for more information.
class KShadowPartition : public KPartition, private KPartitionListener {
public:
	KShadowPartition(KPhysicalPartition *physicalPartition);
	virtual ~KShadowPartition();

	// Hierarchy

	virtual status_t CreateChild(partition_id id, int32 index,
								 KPartition **child = NULL);

	// Shadow Partition

	virtual KShadowPartition *ShadowPartition() const;
	virtual bool IsShadowPartition() const;
	void UnsetPhysicalPartition();
	virtual KPhysicalPartition *PhysicalPartition() const;

	void SyncWithPhysicalPartition();

	virtual void WriteUserData(UserDataWriter &writer,
							   user_partition_data *data);

	virtual void Dump(bool deep, int32 level);

private:
	virtual void OffsetChanged(KPartition *partition, off_t offset);
	virtual void SizeChanged(KPartition *partition, off_t size);
	virtual void ContentSizeChanged(KPartition *partition, off_t size);
	virtual void BlockSizeChanged(KPartition *partition, uint32 blockSize);
	virtual void IndexChanged(KPartition *partition, int32 index);
	virtual void StatusChanged(KPartition *partition, uint32 status);
	virtual void FlagsChanged(KPartition *partition, uint32 flags);
	virtual void NameChanged(KPartition *partition, const char *name);
	virtual void ContentNameChanged(KPartition *partition, const char *name);
	virtual void TypeChanged(KPartition *partition, const char *type);
	virtual void IDChanged(KPartition *partition, partition_id id);
	virtual void VolumeIDChanged(KPartition *partition, dev_t volumeID);
	virtual void MountCookieChanged(KPartition *partition, void *cookie);
	virtual void ParametersChanged(KPartition *partition,
								   const char *parameters);
	virtual void ContentParametersChanged(KPartition *partition,
										  const char *parameters);
	virtual void ChildAdded(KPartition *partition, KPartition *child,
							int32 index);
	virtual void ChildRemoved(KPartition *partition, KPartition *child,
							  int32 index);
	virtual void DiskSystemChanged(KPartition *partition,
								   KDiskSystem *diskSystem);
	virtual void CookieChanged(KPartition *partition, void *cookie);
	virtual void ContentCookieChanged(KPartition *partition, void *cookie);

private:
	KPhysicalPartition	*fPhysicalPartition;

};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KShadowPartition;

#endif	// _K_DISK_DEVICE_SHADOW_PARTITION_H
