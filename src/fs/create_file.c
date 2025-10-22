#include <fs/elixir.h>
#include <vga.h>
#include <ide.h>

struct inode* create_file(uint8_t drive){
    struct inode* in;

    if (drive >= 4) {
        printf("Error: Invalid drive index %d.\n", drive);
        return NULL;
    }

    printf("Allocating super_block struct...\n");
    in = kmalloc(sizeof(struct inode));
    if (!in) {
        printf("Failed to allocate inode!\n");
        return NULL;
    }

    memset(inode, 0, sizeof(struct inode));


}