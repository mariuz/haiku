#include "Icb.h"

#include "AllocationDescriptorList.h"
#include "DirectoryIterator.h"
#include "Utils.h"
#include "Volume.h"

using namespace Udf;

Icb::Icb(Volume *volume, udf_long_address address)
	: fVolume(volume)
	, fData(volume)
	, fInitStatus(B_NO_INIT)
	, fId(to_vnode_id(address))
	, fFileEntry(&fData)
	, fExtendedEntry(&fData)
{
	DEBUG_INIT_ETC(CF_PUBLIC, "Icb", ("volume: %p, address(block: %ld, "
	               "partition: %d, length: %ld)", volume, address.block(),
	               address.partition(), address.length()));  
	status_t err = volume ? B_OK : B_BAD_VALUE;
	if (!err) {
		off_t block;
		err = fVolume->MapBlock(address, &block);
		if (!err) {
			udf_icb_header *header = reinterpret_cast<udf_icb_header*>(fData.SetTo(block));
			PDUMP(header);
			err = header->tag().init_check(address.block());
		}
	}		
	fInitStatus = err;
}
	
status_t
Icb::InitCheck()
{
	return fInitStatus;
}

status_t
Icb::Read(off_t pos, void *buffer, size_t *length, uint32 *block)
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_HIGH_VOLUME, "Icb",
	               ("pos: %lld, buffer: %p, length: (%p)->%ld", pos, buffer, length, (length ? *length : 0)));

	if (!buffer || !length || uint64(pos) >= Length())
		RETURN(B_BAD_VALUE);

	switch (IcbTag().descriptor_flags()) {
		case ICB_DESCRIPTOR_TYPE_SHORT: {
			PRINT(("descriptor type: short\n"));
			AllocationDescriptorList<ShortDescriptorAccessor> list(this, ShortDescriptorAccessor(0));
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}
	
		case ICB_DESCRIPTOR_TYPE_LONG: {
			PRINT(("descriptor type: long\n"));
			AllocationDescriptorList<LongDescriptorAccessor> list(this);
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}
		
		case ICB_DESCRIPTOR_TYPE_EXTENDED: {
			PRINT(("descriptor type: extended\n"));
//			AllocationDescriptorList<ExtendedDescriptorAccessor> list(this, ExtendedDescriptorAccessor(0));
//			RETURN(_Read(list, pos, buffer, length, block));
			RETURN(B_ERROR);
			break;
		}
		
		case ICB_DESCRIPTOR_TYPE_EMBEDDED: {
			PRINT(("descriptor type: embedded\n"));
			RETURN(B_ERROR);
			break;
		}
		
		default:
			PRINT(("Invalid icb descriptor flags! (flags = %d)\n", IcbTag().descriptor_flags()));
			RETURN(B_BAD_VALUE);
			break;		
	}
}

status_t
Icb::GetDirectoryIterator(DirectoryIterator **iterator)
{
	status_t err = iterator ? B_OK : B_BAD_VALUE;

	if (!err) {
		*iterator = new DirectoryIterator(this);
		if (*iterator) {
			err = fIteratorList.PushBack(*iterator);
		} else {
			err = B_NO_MEMORY;
		}
	}
	
	return err;
}

status_t
Icb::Find(const char *filename, vnode_id *id)
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_DIRECTORY_OPS | CF_HIGH_VOLUME, "Icb",
	               ("filename: `%s', id: %p", filename, id));
	               
	if (!filename || !id)
		RETURN(B_BAD_VALUE);
		
	DirectoryIterator *i;
	status_t err = GetDirectoryIterator(&i);
	if (!err) {
		vnode_id entryId;
		uint32 length = B_FILE_NAME_LENGTH;
		char name[B_FILE_NAME_LENGTH];
		
		bool foundIt = false;
		while (i->GetNextEntry(name, &length, &entryId) == B_OK)
		{
	    	if (strcmp(filename, name) == 0) {
	    		foundIt = true;
	    		break;
	    	}	
		}
		
		if (foundIt) {
			*id = entryId;
		} else {
			err = B_ENTRY_NOT_FOUND;
		}
	}
	
	RETURN(err);
}
