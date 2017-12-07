#ifndef _INCLUDE_SFS_API_H_
#define _INCLUDE_SFS_API_H_

#include <stdint.h>

#define MAX_FILE_NAME 20
#define MAX_EXTENSION_NAME 3
#define NUM_DIRECT_POINTERS 12  
#define NUM_INDIRECT_POINTERS 256

typedef struct superblock_t{
    uint64_t magic;
    uint64_t block_size;
    uint64_t fs_size;
    uint64_t inode_table_len;
    uint64_t root_dir_inode;
} superblock_t;

typedef struct inode_t {
    unsigned int mode;
    unsigned int link_cnt;
    unsigned int uid;
    unsigned int gid;
    unsigned int size;
    unsigned int data_ptrs[NUM_DIRECT_POINTERS];
    unsigned int indirectPointer; // points to a data block that points to other data blocks (Single indirect)
} inode_t;

/*
 * inodeIndex    which inode this entry describes
 * inode  pointer towards the inode in the inode table
 * rwptr    where in the file to start   
 */
typedef struct file_descriptor_t {
    uint64_t inodeIndex;
    inode_t* inode; // 
    uint64_t rwptr;
} file_descriptor_t; 


typedef struct directory_entry_t{
    int num; // represents the inode number of the entry. 
    char name[MAX_FILE_NAME]; // represents the name of the entry. 
} directory_entry_t;


void mksfs(int fresh);
int sfs_getnextfilename(char *fname);
int sfs_getfilesize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, const char *buf, int length);
int sfs_fseek(int fileID, int loc);
int sfs_remove(char *file);

#endif //_INCLUDE_SFS_API_H_
