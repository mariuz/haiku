/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>

#include <arch/vm.h>
#include <arch/int.h>
#include <arch/cpu.h>

#include <arch/x86/bios.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static uint32 *sMTRRBitmap;
static int32 sMTRRCount;
static spinlock sMTRRLock;


static status_t
init_mtrr_bitmap(void)
{
	if (sMTRRBitmap != NULL)
		return B_OK;

	sMTRRCount = x86_count_mtrrs();
	if (sMTRRCount == 0)
		return B_UNSUPPORTED;

	sMTRRBitmap = malloc(sMTRRCount / 8);
	if (sMTRRBitmap == NULL)
		return B_NO_MEMORY;

	memset(sMTRRBitmap, 0, sMTRRCount / 8);
	return B_OK;
}


static int32
allocate_mtrr(void)
{
	int32 count = sMTRRCount / 32;
	int32 i, j;

	cpu_status state = disable_interrupts();
	acquire_spinlock(&sMTRRLock);

	for (i = 0; i < count; i++) {
		if (sMTRRBitmap[i] == 0xffffffff)
			continue;

		// find free bit

		for (j = 0; j < 32; j++) {
			if (sMTRRBitmap[i] & (1UL << j))
				continue;

			sMTRRBitmap[i] |= 1UL << j;

			release_spinlock(&sMTRRLock);
			restore_interrupts(state);
			return i * 32 + j;
		}
	}

	release_spinlock(&sMTRRLock);
	restore_interrupts(state);

	return -1;
}


static void
free_mtrr(int32 index)
{
	int32 i = index / 32;
	int32 j = index - i * 32;

	cpu_status state = disable_interrupts();
	acquire_spinlock(&sMTRRLock);

	sMTRRBitmap[i] &= ~(1UL << j);

	release_spinlock(&sMTRRLock);
	restore_interrupts(state);
}


status_t
arch_vm_init(kernel_args *args)
{
	TRACE(("arch_vm_init: entry\n"));
	return 0;
}


status_t
arch_vm_init_post_area(kernel_args *args)
{
	void *dmaAddress;
	area_id id;

	TRACE(("arch_vm_init_post_area: entry\n"));

	// account for DMA area and mark the pages unusable
	vm_mark_page_range_inuse(0x0, 0xa0000 / B_PAGE_SIZE);

	// map 0 - 0xa0000 directly
	id = map_physical_memory("dma_region", (void *)0x0, 0xa0000,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &dmaAddress);
	if (id < 0) {
		panic("arch_vm_init_post_area: unable to map dma region\n");
		return B_NO_MEMORY;
	}

	return bios_init();
}


status_t
arch_vm_init_end(kernel_args *args)
{
	TRACE(("arch_vm_init_endvm: entry\n"));

	// throw away anything in the kernel_args.pgtable[] that's not yet mapped
	vm_free_unused_boot_loader_range(KERNEL_BASE, 0x400000 * args->arch_args.num_pgtables);

	return B_OK;
}


void
arch_vm_aspace_swap(vm_address_space *aspace)
{
	i386_swap_pgdir((addr_t)i386_translation_map_get_pgdir(&aspace->translation_map));
}


bool
arch_vm_supports_protection(uint32 protection)
{
	// x86 always has the same read/write properties for userland and the kernel.
	// That's why we do not support user-read/kernel-write access. While the
	// other way around is not supported either, we don't care in this case
	// and give the kernel full access.
	if ((protection & (B_READ_AREA | B_WRITE_AREA)) == B_READ_AREA
		&& protection & B_KERNEL_WRITE_AREA)
		return false;

	return true;
}


void
arch_vm_init_area(vm_area *area)
{
	area->memory_type.type = 0;
}


void
arch_vm_unset_memory_type(vm_area *area)
{
	if (area->memory_type.type == 0)
		return;

	x86_unset_mtrr(area->memory_type.index);
	free_mtrr(area->memory_type.index);
}


status_t
arch_vm_set_memory_type(vm_area *area, uint32 type)
{
	status_t status;
	int32 index;

	if (type == 0)
		return B_OK;

	switch (type) {
		case B_MTR_UC:	// uncacheable
			type = 0;
			break;
		case B_MTR_WC:	// write combining
			type = 1;
			break;
		case B_MTR_WT:	// write through
			type = 4;
			break;
		case B_MTR_WP:	// write protected
			type = 5;
			break;
		case B_MTR_WB:	// write back
			type = 6;
			break;

		default:
			return B_BAD_VALUE;
	}

	if (sMTRRBitmap == NULL) {
		status = init_mtrr_bitmap();
		if (status < B_OK)
			return status;
	}

	index = allocate_mtrr();
	if (index < 0)
		return B_ERROR;

	status = x86_set_mtrr(index, area->base, area->size, type);
	if (status != B_OK) {
		area->memory_type.type = (uint16)type;
		area->memory_type.index = (uint16)index;
	} else
		free_mtrr(index);

	dprintf("memory type: %u, index: %ld\n", area->memory_type.type, index);
	return status;
}
