
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fuse.h>
#include <strings.h>
#include "disk_emu.h"
#define LASTNAME_FIRSTNAME_DISK "harris_alexander.disk"
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 1024  //maximum number of data blocks on the disk.
#define NUM_INODES 100
#define NUM_BLOCKS_INODE ceil(sizeof(inode_table)/BLOCK_SIZE)
#define NUM_BLOCKS_ROOT ceil(sizeof(root_dir)/BLOCK_SIZE)
#define MAGIC_NUMBER 0xACBD0005
#define BITMAP_ROW_SIZE (NUM_BLOCKS/8) // this essentially mimcs the number of rows we have in the bitmap. we will have 128 rows. 

/* macros */
#define FREE_BIT(_data, _which_bit) \
    _data = _data | (1 << _which_bit)

#define USE_BIT(_data, _which_bit) \
    _data = _data & ~(1 << _which_bit)

file_descriptor_t fd_table[NUM_INODES];
inode_t inode_table[NUM_INODES];
directory_entry_t root_dir[NUM_INODES];
superblock_t super_block;
int current_file = 0;

//initialize all bits to high
uint8_t free_bit_map[BITMAP_ROW_SIZE] = { [0 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };

void force_set_index(uint32_t index) {
    // Used to force indicies to used 
    // this is the opposite of rm_index. 
    uint32_t i = index / 8;
    uint8_t bit = index % 8;

    USE_BIT(free_bit_map[i], bit);
}

uint32_t get_index() {
    uint32_t i = 0;

    // find the first section with a free bit
    // let's ignore overflow for now...
    while (free_bit_map[i] == 0) { i++; }

    // now, find the first free bit
    /*
        The ffs() function returns the position of the first (least
       significant) bit set in the word i.  The least significant bit is
       position 1 and the most significant position is, for example, 32 or
       64.  
    */
    // Since ffs has the lsb as 1, not 0. So we need to subtract
    uint8_t bit = ffs(free_bit_map[i]) - 1;

    // set the bit to used
    USE_BIT(free_bit_map[i], bit);

    //return which block we used
    return i*8 + bit;
}

void rm_index(uint32_t index) {

    // get index in array of which bit to free
    uint32_t i = index / 8;

    // get which bit to free
    uint8_t bit = index % 8;

    // free bit
    FREE_BIT(free_bit_map[i], bit);
}

//Helper methods for initializing and writing to disk
int init_fdt() {
    file_descriptor_t fd;
    fd.rwptr = 0;
    fd.inode = NULL;
    fd.inodeIndex = -1;

    for (int i = 0; i < NUM_INODES; i++) {
        fd_table[i] = fd;
    }

    return 0;
}

int init_dir() {
    directory_entry_t root;
    root.num = -1;
    strcpy(root.name, "");

    for (int i = 0; i < NUM_INODES; i++) {
        root_dir[i] = root;
    }

    int dir_written = write_blocks(NUM_BLOCKS_INODE + 1, NUM_BLOCKS_ROOT, &root_dir);
    if (dir_written < 0) {
        printf("Failed initializing directory\n");
        return -1;
    }
    for (int i = 0; i < dir_written; i++) {
        force_set_index(NUM_BLOCKS_INODE + 1 + i);
    }

    return 0;
}

int write_dir() {
    int dir_written = write_blocks(NUM_BLOCKS_INODE + 1, NUM_BLOCKS_ROOT, &root_dir);
    if (dir_written < 0) {
        printf("Failed writing directory\n");
        return -1;
    }
    for (int i = 0; i < dir_written; i++) {
        force_set_index(NUM_BLOCKS_INODE + 1 + i);
    }
    return 0;
}

int init_bitmap() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        rm_index(i);
    }
    int bitmap_written = write_blocks(NUM_BLOCKS - 1, 1, &free_bit_map);
    if (bitmap_written < 0) {
        printf("Failure initializing bitmap\n");
        return -1;
    }
    force_set_index(NUM_BLOCKS - 1);
    return 0;
}

int init_inode_table() {
    inode_t inode;
    inode.mode = 777;
    inode.link_cnt = 0;
    inode.uid = 0;
    inode.gid = 0;
    inode.size = -1;
    inode.indirectPointer = -1;
    for (int i = 0; i < 12; i++) {
        inode.data_ptrs[i] = -1;
    }

    for (int i = 0; i < NUM_INODES; i++) {
        inode_table[i] = inode;
    }

    int inode_written = write_blocks(1, NUM_BLOCKS_INODE, &inode_table);
    if (inode_written < 0) {
        printf("Failure initializing inode table\n");
        return -1;
    }
    for (int i = 1; i < inode_written; i++) {
        force_set_index(i);
    }

    return 0;
}

int write_inode_table() {
    int inode_written = write_blocks(1, NUM_BLOCKS_INODE, &inode_table);
    if (inode_written < 0) {
        printf("Failure writing inode table\n");
        return -1;
    }
    for (int i = 1; i < inode_written; i++) {
        force_set_index(i);
    }
    return 0;
}

int init_superblock() {
    super_block.magic = MAGIC_NUMBER;
    super_block.block_size = BLOCK_SIZE;
    super_block.fs_size = NUM_BLOCKS * BLOCK_SIZE;
    super_block.inode_table_len = NUM_INODES;
    super_block.root_dir_inode = 0;

    int super_written = write_blocks(0, 1, &super_block);
    if (super_written < 0) {
        printf("Failure initializing superblock\n");
        return -1;
    }
    force_set_index(0);
    return 0;
}

int write_superblock() {
    int super_written = write_blocks(0, 1, &super_block);
    if (super_written < 0) {
        printf("Failure writing superblock\n");
        return -1;
    }
    force_set_index(0);
    return 0;
}

void mksfs(int fresh) {
   
    init_fdt();

    if (fresh) {
        unlink(LASTNAME_FIRSTNAME_DISK); //Delete disk if already existing.

        init_fresh_disk(LASTNAME_FIRSTNAME_DISK, BLOCK_SIZE, NUM_BLOCKS);

        init_superblock();
        init_inode_table();
        init_dir();
        init_bitmap();

    }
    else {
        init_disk(LASTNAME_FIRSTNAME_DISK, BLOCK_SIZE, NUM_BLOCKS);

        int super_read = read_blocks(0, 1, &super_block);
        int bitmap_read = read_blocks(NUM_BLOCKS - 1, 1, &free_bit_map);
        int inode_read = read_blocks(1, NUM_BLOCKS_INODE, &inode_table);
        int dir_read = read_blocks(NUM_BLOCKS_INODE + 1, NUM_BLOCKS_ROOT, &root_dir);

        if (super_read < 1) {
            printf("Failure reading superblock\n");
        }
        if (bitmap_read < 1) {
            printf("Failure reading bitmap\n");
        }
        if (inode_read < NUM_BLOCKS_INODE) {
            printf("Failure reading inode table\n");
        }
        if (dir_read < NUM_BLOCKS_ROOT) {
            printf("Failure reading root directory\n");
        }

    }
}

int sfs_getnextfilename(char *fname) {
    for (int i = current_file; i < NUM_INODES; i++) {
        if (root_dir[i].num >= 0) {
            strcpy(fname, root_dir[i].name);
            current_file = i + 1;
            break;
        }
        return 0;
    }
    if (current_file > NUM_INODES) {
        current_file = 0;
        return 0;
    }

    return 1;
}

int sfs_getfilesize(const char* path) {
    for (int i = 0; i < NUM_INODES; i++) {
        if (root_dir[i].num >= 0 && !strcmp(root_dir[i].name, path)) {
            return inode_table[root_dir[i].num].size;
        }
    }
    printf("Couldn't get file size(file dne)\n");
    return -1;
}

int sfs_fopen(char *name) {
    int file = 0;
    int dir_index = -1;

    for (int i = 0; i < NUM_INODES; i++) {
        if (!strcmp(root_dir[i].name, name)) {
            file = 1;
            dir_index = root_dir[i].num;
            break;
        }
    }

    if (file) {
        //Check if file already opened in file descriptor
        for (int i = 0; i < NUM_INODES; i++) {
            if (fd_table[i].inodeIndex == dir_index) {
                return i;
            }
        }
        //Find free space in fd table
        int fd_index = -1;
        for (int i = 0; i < NUM_INODES; i++) {
            if (fd_table[i].inodeIndex == -1) {
                fd_index = i;
                break;
            }
        }
        if (fd_index == -1) {
            printf("File descriptor table full\n");
            return -1;
        }

        file_descriptor_t opened_file;
        opened_file.inodeIndex = dir_index;
        opened_file.inode = &inode_table[dir_index];
        opened_file.rwptr = inode_table[dir_index].size;
        fd_table[fd_index] = opened_file;

        return fd_index;

    }
    else {
        //Find free space in directory
        for (int i = 0; i < NUM_INODES; i++) {
            if (root_dir[i].num < 0) {
                dir_index = i;
                break;
            }
        }
        if (dir_index == -1) {
            printf("Directory full\n");
            return -1;
        }

        //Find free space in fd table
        int fd_index = -1;
        for (int i = 0; i < NUM_INODES; i++) {
            if (fd_table[i].inodeIndex == -1) {
                fd_index = i;
                break;
            }
        }
        if (fd_index == -1) {
            printf("File descriptor table full\n");
            return -1;
        }

        int inode = -1;
        for (int i = 0; i < NUM_INODES; i++) {
            if (inode_table[i].size == -1) {
                inode = i;
                break;
            }
        }
        if (inode == -1) {
            printf("Inode table full\n");
            return -1;
        }

        inode_table[inode].size = 0;
        root_dir[dir_index].num = inode;
        strcpy(root_dir[dir_index].name, name);

        file_descriptor_t new_file;
        new_file.inodeIndex = inode;
        new_file.inode = &inode_table[inode];
        new_file.rwptr = inode_table[inode].size;
        fd_table[fd_index] = new_file;

        //write new file to disk
        int inode_written = write_inode_table();
        int dir_written = write_dir();

        if (inode_written < 0 || dir_written < 0) {
            printf("Failed to write new file to disk\n");
            return -1;
        }

        return fd_index;
    }
}

int sfs_fclose(int fileID) {

    file_descriptor_t empty;
    empty.rwptr = 0;
    empty.inode = NULL;
    empty.inodeIndex = -1;

    if (fd_table[fileID].inodeIndex == -1) {
        printf("File is already closed/does not exist\n");
        return -1;
    }

    fd_table[fileID] = empty;
    return 0;
}

int sfs_fread(int fileID, char *buf, int length) {
    return 0;
}

int sfs_fwrite(int fileID, const char *buf, int length) {
    return 0;
}

int sfs_fseek(int fileID, int loc) {
    return 0;
}

int sfs_remove(char *file) {
    return 0;
}

