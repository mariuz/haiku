/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_MEMHEAP_H
#define _KERNEL_MEMHEAP_H

#include <kernel.h>
#include <stage2.h>

#define HEAP_SIZE	0x00400000

int   heap_init(addr new_heap_base);
int   heap_init_postsem(kernel_args *ka);
void *kmalloc(unsigned int size);
void  kfree(void *address);
char *kstrdup(const char*text);

#endif /* _KERNEL_MEMHEAP_H */
