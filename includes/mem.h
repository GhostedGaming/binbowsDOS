#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <stdint.h>

typedef struct free_list_block {
    size_t size;
    struct free_list_block *next;
} free_list_block;

void init_allocator_region(uint32_t region_base, uint32_t region_size);

void *kmalloc(size_t size);
void kfree(void *ptr);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
static inline void destroy(void **p) {
    if (p && *p) {
        kfree(*p);
        *p = NULL;
    }
}

#endif
