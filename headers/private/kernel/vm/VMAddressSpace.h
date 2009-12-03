/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_ADDRESS_SPACE_H
#define _KERNEL_VM_VM_ADDRESS_SPACE_H


#include <OS.h>

#include <vm/vm_translation_map.h>
#include <vm/VMArea.h>


struct VMAddressSpace {
public:
			class Iterator;

public:
								VMAddressSpace(team_id id, addr_t base,
									size_t size, bool kernel);
								~VMAddressSpace();

	static	status_t			Init();
	static	status_t			InitPostSem();

			team_id				ID() const				{ return fID; }
			addr_t				Base() const			{ return fBase; }
			size_t				Size() const			{ return fSize; }
			size_t				FreeSpace() const		{ return fFreeSpace; }
			bool				IsBeingDeleted() const	{ return fDeleting; }

			vm_translation_map&	TranslationMap()	{ return fTranslationMap; }

			status_t			ReadLock()
									{ return rw_lock_read_lock(&fLock); }
			void				ReadUnlock()
									{ rw_lock_read_unlock(&fLock); }
			status_t			WriteLock()
									{ return rw_lock_write_lock(&fLock); }
			void				WriteUnlock()
									{ rw_lock_write_unlock(&fLock); }

			int32				RefCount() const
									{ return fRefCount; }

			void				Get()	{ atomic_add(&fRefCount, 1); }
			void 				Put();
			void				RemoveAndPut();

			void				IncrementFaultCount()
									{ atomic_add(&fFaultCount, 1); }
			void				IncrementChangeCount()
									{ fChangeCount++; }

			VMArea*				FirstArea() const
									{ return fAreas.Head(); }
			VMArea*				NextArea(VMArea* area) const
									{ return fAreas.GetNext(area); }

			VMArea*				LookupArea(addr_t address) const;
			status_t			InsertArea(void** _address, uint32 addressSpec,
									addr_t size, VMArea* area);
			void				RemoveArea(VMArea* area);

	inline	Iterator			GetIterator();

	static	status_t			Create(team_id teamID, addr_t base, size_t size,
									bool kernel,
									VMAddressSpace** _addressSpace);

	static	team_id				KernelID()
									{ return sKernelAddressSpace->ID(); }
	static	VMAddressSpace*		Kernel()
									{ return sKernelAddressSpace; }
	static	VMAddressSpace*		GetKernel();

	static	team_id				CurrentID();
	static	VMAddressSpace*		GetCurrent();

	static	VMAddressSpace*		Get(team_id teamID);

			VMAddressSpace*&	HashTableLink()	{ return fHashTableLink; }

			void				Dump() const;

private:
			status_t			_InsertAreaIntoReservedRegion(addr_t start,
									size_t size, VMArea* area);
			status_t			_InsertAreaSlot(addr_t start, addr_t size,
									addr_t end, uint32 addressSpec,
									VMArea* area);

	static	int					_DumpCommand(int argc, char** argv);
	static	int					_DumpListCommand(int argc, char** argv);

private:
			friend class Iterator;

			struct HashDefinition;

private:
			VMAddressSpace*		fHashTableLink;
			addr_t				fBase;
			size_t				fSize;
			size_t				fFreeSpace;
			rw_lock				fLock;
			team_id				fID;
			int32				fRefCount;
			int32				fFaultCount;
			int32				fChangeCount;
			vm_translation_map	fTranslationMap;
			VMAddressSpaceAreaList fAreas;
	mutable	VMArea*				fAreaHint;
			bool				fDeleting;
	static	VMAddressSpace*		sKernelAddressSpace;
};


class VMAddressSpace::Iterator {
public:
	Iterator()
	{
	}

	Iterator(VMAddressSpace* addressSpace)
		:
		fIterator(addressSpace->fAreas.GetIterator())
	{
	}

	bool HasNext() const
	{
		return fIterator.HasNext();
	}

	VMArea* Next()
	{
		return fIterator.Next();
	}

	void Rewind()
	{
		fIterator.Rewind();
	}

private:
	VMAddressSpaceAreaList::Iterator fIterator;
};


inline VMAddressSpace::Iterator
VMAddressSpace::GetIterator()
{
	return Iterator(this);
}


#ifdef __cplusplus
extern "C" {
#endif

status_t vm_delete_areas(struct VMAddressSpace *aspace);
#define vm_swap_address_space(from, to) arch_vm_aspace_swap(from, to)

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_VM_VM_ADDRESS_SPACE_H */
