/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_GENERIC_SYSCALLS_H
#define _KERNEL_GENERIC_SYSCALLS_H


#include <SupportDefs.h>


/* If we decide to make this API public, the contents of this file
 * should be moved to KernelExport.h
 */

typedef status_t (*syscall_hook)(uint32 subsystem, uint32 function, void *buffer, size_t bufferSize);

/* predefined functions */
#define B_SYSCALL_INFO	~0UL
	// gets a minimum version uint32, and fills it with the current version on return

/* syscall flags */
#define B_SYSCALL_NOT_REPLACEABLE	1
#define B_DO_NOT_REPLACE_SYSCALL	2


#ifdef __cplusplus
extern "C" {
#endif

status_t register_generic_syscall(uint32 subsystem, syscall_hook hook,
			uint32 version, uint32 flags);
status_t unregister_generic_syscall(uint32 subsystem, uint32 version);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_GENERIC_SYSCALLS_H */
