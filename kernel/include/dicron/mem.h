#ifndef _DICRON_MEM_H
#define _DICRON_MEM_H

#include <dicron/types.h>

void  *kmalloc(size_t size);
void  *kzalloc(size_t size);
void  *krealloc(void *ptr, size_t new_size);
void   kfree(void *ptr);
size_t kmalloc_usable_size(void *ptr);
void   kheap_dump_stats(void);

#endif
