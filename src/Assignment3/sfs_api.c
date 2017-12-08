#include "sfs_api.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
//#include <fuse.h>
#include <strings.h>
#include "disk_emu.h"

// Alexander Harris - 260688155
// Still problems with test 1, read and write not working properly
// Test 2 segfaults

#define LASTNAME_FIRSTNAME_DISK "HARRIS_ALEXANDER.disk"
#define BLOCK_SIZE 1024
#define NUM_INODES 100
#define NUM_DIRECT_POINTERS 12
#define NUM_INDIRECT_POINTERS 256
#define NUM_BLOCKS_INODE (sizeof(inode_t) * NUM_INODES) / BLOCK_SIZE + ((sizeof(inode_t) * NUM_INODES) % BLOCK_SIZE > 0)
#define NUM_BLOCKS_ROOT (sizeof(directory_entry) * (NUM_INODES)) / BLOCK_SIZE + ((sizeof(directory_entry) * (NUM_INODES)) % BLOCK_SIZE > 0)

superblock_t super_block;
file_descriptor fd_table[NUM_INODES];
inode_t inode_table[NUM_INODES];
directory_entry root_dir[NUM_INODES];

int current_file = 0;

int init_fdt()
{
    file_descriptor fd_root;
    fd_root.inodeIndex = 0;
    fd_root.inode = &inode_table[0];
    fd_root.rwptr = 0;

    file_descriptor fd;
    fd.inodeIndex = -1;
    fd.inode = NULL;
    fd.rwptr = 0;

    fd_table[0] = fd_root;
    for (int i = 1; i < NUM_INODES; i++)
    {
        fd_table[i] = fd;
    }

    return 0;
}

int init_dir()
{
    directory_entry empty;
    empty.num = -1;
    strcpy(empty.name, "");

    for (int i = 0; i < NUM_INODES; i++)
    {
        root_dir[i] = empty;
    }

    int dir_written = write_blocks(NUM_BLOCKS_INODE + 1, NUM_BLOCKS_ROOT, root_dir);
    if (dir_written < NUM_BLOCKS_ROOT)
    {
        printf("Failed initializing directory\n");
        return -1;
    }
    for (int i = 0; i < dir_written; i++)
    {
        force_set_index(NUM_BLOCKS_INODE + 1 + i);
    }

    return 0;
}

int write_dir()
{
    int dir_written = write_blocks(NUM_BLOCKS_INODE + 1, NUM_BLOCKS_ROOT, root_dir);
    if (dir_written < NUM_BLOCKS_ROOT)
    {
        printf("Failed writing directory\n");
        return -1;
    }
    for (int i = 0; i < dir_written; i++)
    {
        force_set_index(NUM_BLOCKS_INODE + 1 + i);
    }
    return 0;
}

int init_bitmap()
{
    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        rm_index(i);
    }
    int bitmap_written = write_blocks(NUM_BLOCKS - 1, 1, free_bit_map);
    if (bitmap_written < 1)
    {
        printf("Failure initializing bitmap\n");
        return -1;
    }
    force_set_index(NUM_BLOCKS - 1);
    return 0;
}

int write_bitmap()
{
    int bitmap_written = write_blocks(NUM_BLOCKS - 1, 1, free_bit_map);
    if (bitmap_written < 1)
    {
        printf("Failure writing bitmap\n");
        return -1;
    }
    force_set_index(NUM_BLOCKS - 1);
    return 0;
}

int init_inode_table()
{
    // Generic inode
    inode_t inode;
    inode.mode = 777;
    inode.link_cnt = 0;
    inode.uid = 0;
    inode.gid = 0;
    inode.size = -1;
    inode.indirectPointer = -1;
    for (int i = 0; i < NUM_DIRECT_POINTERS; i++)
    {
        inode.data_ptrs[i] = -1;
    }

    //Root dir inode
    inode_t root;
    root.mode = 777;
    root.link_cnt = 0;
    root.uid = 0;
    root.gid = 0;
    root.size = 0;
    root.indirectPointer = -1;
    for (int i = 0; i < NUM_BLOCKS_ROOT; i++)
    {
        root.data_ptrs[i] = i + NUM_BLOCKS_INODE + 1;
    }
    for (int i = NUM_BLOCKS_ROOT; i < NUM_DIRECT_POINTERS; i++)
    {
        root.data_ptrs[i] = -1;
    }

    inode_table[0] = root;
    for (int i = 1; i < NUM_INODES; i++)
    {
        inode_table[i] = inode;
    }

    int inode_written = write_blocks(1, NUM_BLOCKS_INODE, inode_table);
    if (inode_written < NUM_BLOCKS_INODE)
    {
        printf("Failure initializing inode table\n");
        return -1;
    }
    //Inode table after superblock
    for (int i = 0; i < inode_written; i++)
    {
        force_set_index(i + 1);
    }

    return 0;
}

int write_inode_table()
{
    int inode_written = write_blocks(1, NUM_BLOCKS_INODE, inode_table);
    if (inode_written < NUM_BLOCKS_INODE)
    {
        printf("Failure writing inode table\n");
        return -1;
    }
    for (int i = 0; i < inode_written; i++)
    {
        force_set_index(i + 1);
    }
    return 0;
}

int init_superblock()
{
    super_block.magic = 0xACBD0005;
    super_block.block_size = BLOCK_SIZE;
    super_block.fs_size = NUM_BLOCKS;
    super_block.inode_table_len = NUM_BLOCKS_INODE;
    super_block.root_dir_inode = 0;

    int super_written = write_blocks(0, 1, &super_block);
    if (super_written < 1)
    {
        printf("Failure initializing superblock\n");
        return -1;
    }
    force_set_index(0);
    return 0;
}

int write_superblock()
{
    int super_written = write_blocks(0, 1, &super_block);
    if (super_written < 1)
    {
        printf("Failure writing superblock\n");
        return -1;
    }
    force_set_index(0);
    return 0;
}

void mksfs(int fresh)
{

    if (fresh)
    {
        int init_fresh = init_fresh_disk(LASTNAME_FIRSTNAME_DISK, BLOCK_SIZE, NUM_BLOCKS);

        if (init_fresh != 0)
        {
            printf("Failed to init fresh disk\n");
        }

        init_superblock();
        init_inode_table();
        init_dir();
        init_fdt();
        init_bitmap();
    }
    else
    {
        int init = init_disk(LASTNAME_FIRSTNAME_DISK, BLOCK_SIZE, NUM_BLOCKS);

        if (init != 0)
        {
            printf("Failed to init disk\n");
        }

        int bitmap_read = read_blocks(NUM_BLOCKS - 1, 1, free_bit_map);
        force_set_index(NUM_BLOCKS - 1);

        int super_read = read_blocks(0, 1, &super_block);
        force_set_index(0);

        int inode_read = read_blocks(1, NUM_BLOCKS_INODE, inode_table);
        for (int i = 0; i < NUM_BLOCKS_INODE; i++)
        {
            force_set_index(i + 1);
        }

        int dir_read = read_blocks(NUM_BLOCKS_INODE + 1, NUM_BLOCKS_ROOT, root_dir);
        for (int i = 0; i < NUM_BLOCKS_ROOT; i++)
        {
            force_set_index(i + NUM_BLOCKS_INODE + 1);
        }

        init_fdt();

        if (super_read < 1)
        {
            printf("Failure reading superblock\n");
        }
        if (bitmap_read < 1)
        {
            printf("Failure reading bitmap\n");
        }
        if (inode_read < NUM_BLOCKS_INODE)
        {
            printf("Failure reading inode table\n");
        }
        if (dir_read < NUM_BLOCKS_ROOT)
        {
            printf("Failure reading root_dir\n");
        }
    }
}

int sfs_getnextfilename(char *fname)
{
    if (current_file >= NUM_INODES)
    {
        current_file = 0;
        return 0;
    }
    for (int i = current_file; i < NUM_INODES; i++)
    {
        if (root_dir[i].num == -1)
        {
            strcpy(fname, root_dir[i].name);
            current_file = i + 1;
            break;
        }
    }
    return 1;
}

int sfs_getfilesize(const char *path)
{
    for (int i = 0; i < NUM_INODES; i++)
    {
        if (root_dir[i].num != -1 && strcmp(root_dir[i].name, path) == 0)
        {
            return inode_table[root_dir[i].num].size;
        }
    }
    printf("File does not exist\n");
    return -1;
}

int sfs_fopen(char *name)
{
    int length = strlen(name);
    if (length > MAX_FILE_NAME)
    {
        printf("File name too long\n");
        return -1;
    }

    int file = 0;
    int dir_index = -1;
    int fd_index = -1;
    int inode_index = -1;

    //Check if file exists
    for (int i = 0; i < NUM_INODES; i++)
    {
        if (!strcmp(root_dir[i].name, name))
        {
            file = 1;
            inode_index = root_dir[i].num;
            break;
        }
    }

    if (file == 1)
    {
        //Check if file already opened in file descriptor
        for (int i = 1; i < NUM_INODES; i++)
        {
            if (fd_table[i].inodeIndex == inode_index)
            {
                return i;
            }
        }
        //Find free space in fd table
        for (int i = 1; i < NUM_INODES; i++)
        {
            if (fd_table[i].inodeIndex == -1)
            {
                fd_index = i;
                break;
            }
        }
        if (fd_index == -1)
        {
            printf("File descriptor table full\n");
            return -1;
        }

        file_descriptor opened_file;
        opened_file.inodeIndex = inode_index;
        opened_file.inode = &inode_table[inode_index];
        opened_file.rwptr = inode_table[inode_index].size;
        fd_table[fd_index] = opened_file;

        return fd_index;
    }
    else
    {
        dir_index = -1;
        //Find free space in directory
        for (int i = 0; i < NUM_INODES; i++)
        {
            if (root_dir[i].num == -1)
            {
                dir_index = i;
                break;
            }
        }
        if (dir_index == -1)
        {
            printf("Directory full\n");
            return -1;
        }

        //Find free space in inode table
        int inode = -1;
        for (int i = 1; i < NUM_INODES; i++)
        {
            if (inode_table[i].size == -1)
            {
                inode = i;
                break;
            }
        }
        if (inode == -1)
        {
            printf("Inode table full\n");
            return -1;
        }

        //Find free space in fd table
        for (int i = 1; i < NUM_INODES; i++)
        {
            if (fd_table[i].inodeIndex == -1)
            {
                fd_index = i;
                break;
            }
        }
        if (fd_index == -1)
        {
            printf("File descriptor table full\n");
            return -1;
        }

        strcpy(root_dir[dir_index].name, name);
        inode_table[inode].size = 0;
        root_dir[dir_index].num = inode;

        //write new file to disk
        int inode_written = write_inode_table();
        int dir_written = write_dir();

        file_descriptor new_file;
        new_file.inodeIndex = inode;
        new_file.inode = &inode_table[inode];
        new_file.rwptr = inode_table[inode].size;
        fd_table[fd_index] = new_file;

        if (inode_written < 0 || dir_written < 0)
        {
            printf("Failed to write new file to disk\n");
            return -1;
        }

        return fd_index;
    }
}

int sfs_fclose(int fileID)
{

    file_descriptor empty;
    empty.inodeIndex = -1;
    empty.inode = NULL;
    empty.rwptr = 0;

    if (fileID > NUM_INODES || fileID <= 0)
    {
        printf("Invalid fileID\n");
        return -1;
    }

    if (fd_table[fileID].inodeIndex == -1)
    {
        printf("File is already closed/does not exist\n");
        return -1;
    }

    fd_table[fileID] = empty;
    return 0;
}

int sfs_fread(int fileID, char *buf, int length)
{
    file_descriptor *target_file = &fd_table[fileID];
    int rw_ptr = (*target_file).rwptr;
    inode_t *inode = (*target_file).inode;

    if ((*target_file).inodeIndex == -1)
    {
        printf("Can't read closed file\n");
        return -1;
    }

    void *temp = malloc(BLOCK_SIZE);

    int first_block = rw_ptr / BLOCK_SIZE;
    int first_block_index = rw_ptr % BLOCK_SIZE;
    int end_ptr = rw_ptr + length;

    if (end_ptr > (*inode).size)
    {
        end_ptr = (*inode).size;
    }

    int last_block = end_ptr / BLOCK_SIZE;
    int last_block_index = end_ptr % BLOCK_SIZE;

    int bytes_written = 0;
    int ind_pointers[NUM_INDIRECT_POINTERS];
    int ind_index;
    int ind_pointers_read = 0;

    for (int i = first_block; i <= last_block; i++)
    {
        memset(temp, 0, BLOCK_SIZE);
        // Indirect pointers
        if (i >= NUM_DIRECT_POINTERS)
        {
            if (ind_pointers_read == 0)
            {
                read_blocks((*inode).indirectPointer, 1, temp);
                memcpy(ind_pointers, temp, BLOCK_SIZE);
                memset(temp, 0, BLOCK_SIZE);
                ind_pointers_read = 1;
            }

            ind_index = i - NUM_DIRECT_POINTERS;
            read_blocks(ind_pointers[ind_index], 1, temp);

            if (i == first_block)
            {
                if (first_block == last_block)
                {
                    memcpy(buf, temp + first_block_index, last_block_index - first_block_index);
                    bytes_written += last_block_index - first_block_index;
                }
                else
                {
                    memcpy(buf, temp + first_block_index, BLOCK_SIZE - first_block_index);
                    bytes_written += BLOCK_SIZE - first_block_index;
                }
            }
            else if (i == last_block)
            {
                memcpy(buf + bytes_written, temp, last_block_index);
                bytes_written += last_block_index;
            }
            else
            {
                memcpy(buf + bytes_written, temp, BLOCK_SIZE);
                bytes_written += BLOCK_SIZE;
            }
        }
        // Direct pointers
        else
        {
            read_blocks((*inode).data_ptrs[i], 1, temp);

            if (i == first_block)
            {
                if (first_block == last_block)
                {
                    memcpy(buf, temp + first_block_index, last_block_index - first_block_index);
                    bytes_written += last_block_index - first_block_index;
                }
                else
                {
                    memcpy(buf, temp + first_block_index, BLOCK_SIZE - first_block_index);
                    bytes_written += BLOCK_SIZE - first_block_index;
                }
            }
            else if (i == last_block)
            {
                memcpy(buf + bytes_written, temp, last_block_index);
                bytes_written += last_block_index;
            }
            else
            {
                memcpy(buf + bytes_written, temp, BLOCK_SIZE);
                bytes_written += BLOCK_SIZE;
            }
        }
    }
    // Set pointer to new position
    (*target_file).rwptr += bytes_written;
    free(temp);
    return bytes_written;
}

int sfs_fwrite(int fileID, const char *buf, int length)
{
    file_descriptor *target_file = &fd_table[fileID];
    int rw_ptr = (*target_file).rwptr;
    inode_t *inode = (*target_file).inode;

    if ((*target_file).inodeIndex == -1)
    {
        printf("Can't read closed file\n");
        return -1;
    }

    void *temp = malloc(BLOCK_SIZE);

    int first_block = rw_ptr / BLOCK_SIZE;
    int first_block_index = rw_ptr % BLOCK_SIZE;
    int end_ptr = rw_ptr + length;
    int last_block = end_ptr / BLOCK_SIZE;
    int last_block_index = end_ptr % BLOCK_SIZE;
    int bytes_written = 0;
    int ind_pointers[NUM_INDIRECT_POINTERS];
    int ind_index;
    int ind_pointers_read = 0;
    int ind_pointers_written = 0;

    for (int i = first_block; i <= last_block; i++)
    {
        memset(temp, 0, BLOCK_SIZE);

        if (i >= NUM_DIRECT_POINTERS)
        {
            if (ind_pointers_read == 0)
            {
                if ((*inode).indirectPointer == -1)
                {
                    (*inode).indirectPointer = get_index();
                    for (int j = 0; j < NUM_INDIRECT_POINTERS; j++)
                    {
                        ind_pointers[j] = -1;
                    }
                    ind_pointers_written = 1;
                }
                else
                {
                    read_blocks((*inode).indirectPointer, 1, temp);
                    memcpy(ind_pointers, temp, BLOCK_SIZE);
                    memset(temp, 0, BLOCK_SIZE);
                }
                ind_pointers_read = 1;
            }

            ind_index = i - NUM_DIRECT_POINTERS;
            if (ind_index >= NUM_INDIRECT_POINTERS)
            {
                printf("Max size\n");
                break;
            }
            if (ind_pointers[ind_index] == -1)
            {
                ind_pointers[ind_index] = get_index();
                ind_pointers_written = 1;
            }
            read_blocks(ind_pointers[ind_index], 1, temp);

            //Copy to buffer
            if (i == first_block)
            {
                if (first_block == last_block)
                {
                    memcpy(temp + first_block_index, buf, last_block_index - first_block_index);
                    bytes_written += last_block_index - first_block_index;
                }
                else
                {
                    memcpy(temp + first_block_index, buf, BLOCK_SIZE - first_block_index);
                    bytes_written += BLOCK_SIZE - first_block_index;
                }
            }
            else if (i == last_block)
            {
                memcpy(temp, buf + bytes_written, last_block_index);
                bytes_written += last_block_index;
            }
            else
            {
                memcpy(temp, buf + bytes_written, BLOCK_SIZE);
                bytes_written += BLOCK_SIZE;
            }
            write_blocks(ind_pointers[ind_index], 1, temp);
        }
        else
        {
            if ((*inode).data_ptrs[i] == -1)
            {
                (*inode).data_ptrs[i] = get_index();
            }
            read_blocks((*inode).data_ptrs[i], 1, temp);

            //Copy to buffer
            if (i == first_block)
            {
                if (first_block == last_block)
                {
                    memcpy(temp + first_block_index, buf, last_block_index - first_block_index);
                    bytes_written += last_block_index - first_block_index;
                }
                else
                {
                    memcpy(temp + first_block_index, buf, BLOCK_SIZE - first_block_index);
                    bytes_written += BLOCK_SIZE - first_block_index;
                }
            }
            else if (i == last_block)
            {
                memcpy(temp, buf + bytes_written, last_block_index);
                bytes_written += last_block_index;
            }
            else
            {
                memcpy(temp, buf + bytes_written, BLOCK_SIZE);
                bytes_written += BLOCK_SIZE;
            }
            write_blocks((*inode).data_ptrs[i], 1, temp);
        }
    }

    if (ind_pointers_written == 1)
    {
        memset(temp, 0, BLOCK_SIZE);
        memcpy(temp, ind_pointers, BLOCK_SIZE);
        write_blocks((*inode).indirectPointer, 1, temp);
    }

    (*target_file).rwptr += bytes_written;
    if ((*inode).size < (*target_file).rwptr)
    {
        (*inode).size = (*target_file).rwptr;
    }

    int inode_written = write_inode_table();
    int bitmap_written = write_bitmap();
    if (inode_written < 0 || bitmap_written < 0)
    {
        printf("Failed writing changes to disk\n");
        return -1;
    }

    free(temp);
    return bytes_written;
}

int sfs_fseek(int fileID, int loc)
{

    if (fileID > NUM_INODES || fileID <= 0)
    {
        printf("Invalid filedID\n");
        return -1;
    }
    if (fd_table[fileID].inodeIndex == -1)
    {
        printf("Can't seek closed file\n");
        return -1;
    }

    if (loc < 0)
    {
        fd_table[fileID].rwptr = 0;
    }
    else if (loc > inode_table[fd_table[fileID].inodeIndex].size)
    {
        fd_table[fileID].rwptr = inode_table[fd_table[fileID].inodeIndex].size;
    }
    else
    {
        fd_table[fileID].rwptr = loc;
    }
    return 0;
}

int sfs_remove(char *file)
{
    // Search for file in dir
    int inode_index = -1;
    for (int i = 0; i < NUM_INODES; i++)
    {
        if (!strcmp(root_dir[i].name, file))
        {
            inode_index = root_dir[i].num;
            break;
        }
    }
    if (inode_index == -1)
    {
        printf("File not removed, not found\n");
        return -1;
    }

    // Check for file in fd table and close it if found
    file_descriptor closed;
    closed.inodeIndex = -1;
    closed.inode = NULL;
    closed.rwptr = 0;
    for (int i = 1; i < NUM_INODES; i++)
    {
        if (fd_table[i].inodeIndex == inode_index)
        {
            fd_table[i] = closed;
        }
    }

    inode_t *inode = &inode_table[inode_index];
    void *temp = malloc(BLOCK_SIZE);
    int last_block = (*inode).size / BLOCK_SIZE;
    int ind_pointers[NUM_INDIRECT_POINTERS];
    int ind_pointers_read = 0;
    int ind_pointers_written = 0;
    int ind_index;

    for (int i = 0; i <= last_block; i++)
    {
        memset(temp, 0, BLOCK_SIZE);

        //Indirect pointers
        if (i >= NUM_DIRECT_POINTERS)
        {
            if (ind_pointers_read == 0)
            {
                read_blocks((*inode).indirectPointer, 1, temp);
                memcpy(ind_pointers, temp, BLOCK_SIZE);
                memset(temp, 0, BLOCK_SIZE);
                ind_pointers_read = 1;
            }
            ind_index = i - NUM_DIRECT_POINTERS;
            write_blocks(ind_pointers[ind_index], 1, temp);
            rm_index(ind_pointers[ind_index]);
            ind_pointers[ind_index] = -1;
            ind_pointers_written = 1;
        }
        //Direct pointers
        else
        {
            write_blocks((*inode).data_ptrs[i], 1, temp);
            rm_index((*inode).data_ptrs[i]);
            (*inode).data_ptrs[i] = -1;
        }
    }

    if (ind_pointers_read == 1)
    {
        rm_index((*inode).indirectPointer);
        (*inode).indirectPointer = -1;
    }
    if (ind_pointers_written == 1)
    {
        memset(temp, 0, BLOCK_SIZE);
        memcpy(temp, ind_pointers, BLOCK_SIZE);
        write_blocks((*inode).indirectPointer, 1, temp);
    }
    (*inode).size = -1;

    write_inode_table();
    write_bitmap();
    write_dir();

    free(temp);

    return 0;
}
