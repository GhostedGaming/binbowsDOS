#include <stdint.h>
#include <stddef.h>

#define MIN_ALLOC_SIZE 16
#define ALIGN_SIZE 8

typedef struct free_list_block {
    uint32_t magic;
    size_t size;
    struct free_list_block *next;
} free_list_block;

#define FREE_BLOCK_MAGIC 0xF00DBABE

static free_list_block *free_list_head = NULL;
static uintptr_t allocator_start = 0;
static uintptr_t allocator_end = 0;

static inline unsigned long save_and_cli(void) {
    unsigned long flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
    __asm__ volatile("cli");
    return flags;
}

static inline void restore_flags(unsigned long flags) {
    if (flags & (1 << 9))
        __asm__ volatile("sti");
}

static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static int valid_block_ptr(free_list_block *b) {
    if (!b) return 0;
    uintptr_t addr = (uintptr_t)b;
    if (addr < allocator_start || addr + sizeof(free_list_block) > allocator_end) return 0;
    if (b->magic != FREE_BLOCK_MAGIC) return 0;
    if (b->size > (allocator_end - addr)) return 0;
    return 1;
}

static void insert_free_block_sorted(free_list_block *block) {
    if (!block) return;
    block->magic = FREE_BLOCK_MAGIC;
    if (!free_list_head) {
        free_list_head = block;
        block->next = NULL;
        return;
    }
    free_list_block *prev = NULL;
    free_list_block *cur = free_list_head;
    while (cur && (uintptr_t)cur < (uintptr_t)block) {
        prev = cur;
        cur = cur->next;
    }
    if (!prev) {
        block->next = free_list_head;
        free_list_head = block;
    } else {
        block->next = prev->next;
        prev->next = block;
    }
    if (block->next) {
        uintptr_t end_block = (uintptr_t)block + sizeof(free_list_block) + block->size;
        if (end_block == (uintptr_t)block->next) {
            block->size += sizeof(free_list_block) + block->next->size;
            block->next = block->next->next;
        }
    }
    prev = NULL;
    cur = free_list_head;
    while (cur && cur->next) {
        uintptr_t end_cur = (uintptr_t)cur + sizeof(free_list_block) + cur->size;
        if (end_cur == (uintptr_t)cur->next) {
            cur->size += sizeof(free_list_block) + cur->next->size;
            cur->next = cur->next->next;
            continue;
        }
        cur = cur->next;
    }
}

void init_allocator_region(uint32_t region_base, uint32_t region_size) {
    uintptr_t aligned_start = (uintptr_t)align_up(region_base, ALIGN_SIZE);
    if (region_size <= sizeof(free_list_block) + (aligned_start - region_base)) return;
    region_size -= (aligned_start - region_base);
    allocator_start = aligned_start;
    allocator_end = aligned_start + region_size;
    free_list_head = (free_list_block *)aligned_start;
    free_list_head->magic = FREE_BLOCK_MAGIC;
    free_list_head->size = region_size - sizeof(free_list_block);
    free_list_head->next = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = align_up(size, ALIGN_SIZE);
    if (size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;
    if (!free_list_head) return NULL;
    unsigned long flags = save_and_cli();
    free_list_block *prev = NULL;
    free_list_block *cur = free_list_head;
    while (cur) {
        if (!valid_block_ptr(cur)) break;
        if (cur->size >= size) {
            if (cur->size >= size + sizeof(free_list_block) + MIN_ALLOC_SIZE) {
                free_list_block *new_block = (free_list_block *)((uint8_t *)cur + sizeof(free_list_block) + size);
                new_block->magic = FREE_BLOCK_MAGIC;
                new_block->size = cur->size - size - sizeof(free_list_block);
                new_block->next = cur->next;
                cur->size = size;
                if (prev) prev->next = new_block;
                else free_list_head = new_block;
            } else {
                if (prev) prev->next = cur->next;
                else free_list_head = cur->next;
            }
            restore_flags(flags);
            void *res = (uint8_t *)cur + sizeof(free_list_block);
            for (size_t i = 0; i < size; i++) ((uint8_t *)res)[i] = 0;
            return res;
        }
        prev = cur;
        cur = cur->next;
    }
    restore_flags(flags);
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    free_list_block *block = (free_list_block *)((uint8_t *)ptr - sizeof(free_list_block));
    if ((uintptr_t)block < allocator_start || (uintptr_t)block + sizeof(free_list_block) > allocator_end) return;
    unsigned long flags = save_and_cli();
    insert_free_block_sorted(block);
    restore_flags(flags);
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;
    for (size_t i = 0; i < n; i++) pdest[i] = psrc[i];
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    if (psrc < pdest) {
        for (size_t i = n; i > 0; i--) pdest[i-1] = psrc[i-1];
    } else {
        for (size_t i = 0; i < n; i++) pdest[i] = psrc[i];
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] < p2[i] ? -1 : 1;
    }
    return 0;
}