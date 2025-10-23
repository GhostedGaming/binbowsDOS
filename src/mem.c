#include <stdint.h>
#include <stddef.h>
#include <vga.h>
#include <mem.h>

#define MIN_ALLOC_SIZE 16
#define ALIGN_SIZE 8
#define PAGE_SIZE 4096

static free_list_block *free_list_head = NULL;

static inline unsigned long save_and_cli(void) {
    unsigned long flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
    __asm__ volatile("cli");
    return flags;
}

// Restore flags if interrupts were enabled
static inline void restore_flags(unsigned long flags) {
    if (flags & (1 << 9))
        __asm__ volatile("sti");
}

static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static void memset_page(void *ptr, int value, size_t size) {
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < size; i++)
        p[i] = (uint8_t)value;
}

static void insert_free_block_sorted(free_list_block *block) {
    if (!block) return;

    free_list_block **cur = &free_list_head;
    while (*cur && *cur < block)
        cur = &(*cur)->next;

    block->next = *cur;
    *cur = block;

    // Merge with next block (coalescing forward)
    if (block->next && (uint8_t*)block + sizeof(free_list_block) + block->size == (uint8_t*)block->next) {
        block->size += sizeof(free_list_block) + block->next->size;
        block->next = block->next->next;
    }

    // Merge with previous block (coalescing backward)
    if (cur != &free_list_head) {
        free_list_block *prev = free_list_head;
        // Search for the block *before* 'block'
        while (prev && prev->next != block) prev = prev->next;
        if (prev && (uint8_t*)prev + sizeof(free_list_block) + prev->size == (uint8_t*)block) {
            prev->size += sizeof(free_list_block) + block->size;
            prev->next = block->next;
        }
    }
}

void init_allocator_region(uint32_t region_base, uint32_t region_size) {
    printf("Initializing allocator region: 0x%x - 0x%x (%u KB)\n",
           region_base, region_base + region_size, region_size / 1024);

    uint32_t aligned_start = align_up(region_base, ALIGN_SIZE);
    
    // Check if the region is too small after alignment adjustment
    if (region_size <= sizeof(free_list_block) + (aligned_start - region_base)) {
        printf("Region too small to use for allocator\n");
        return;
    }
    
    // Adjust size for the alignment offset
    region_size -= (aligned_start - region_base);

    free_list_head = (free_list_block *)aligned_start;
    free_list_head->size = region_size - sizeof(free_list_block);
    free_list_head->next = NULL;

    printf("Allocator ready: base=0x%x, size=%u bytes\n", aligned_start, free_list_head->size);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    size = align_up(size, ALIGN_SIZE);
    if (size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;

    if (!free_list_head) {
        printf("kmalloc: allocator not initialized!\n");
        return NULL;
    }

    unsigned long flags = save_and_cli();
    free_list_block **current = &free_list_head;

    while (*current) {
        printf("Current\n");
        free_list_block *block = *current;
        if (block->size >= size) {
            // Split block if remaining space is large enough for another header + min block
            if (block->size >= size + sizeof(free_list_block) + MIN_ALLOC_SIZE) {
                free_list_block *new_block =
                    (free_list_block *)((uint8_t *)block + sizeof(free_list_block) + size);
                new_block->size = block->size - size - sizeof(free_list_block);
                new_block->next = block->next;
                *current = new_block; // The new (smaller) free block replaces the old one
                block->size = size;    // Set the allocated block's size
            } else {
                // Not enough space to split, allocate the whole block
                *current = block->next;
            }

            restore_flags(flags);
            void *result = (uint8_t *)block + sizeof(free_list_block);
            memset_page(result, 0, size);
            return result;
        }
        current = &((*current)->next);
    }

    restore_flags(flags);
    printf("kmalloc: out of memory for %u bytes\n", size);
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    free_list_block *block = (free_list_block *)((uint8_t *)ptr - sizeof(free_list_block));
    unsigned long flags = save_and_cli();
    insert_free_block_sorted(block);
    restore_flags(flags);
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;
    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    return 0;
}