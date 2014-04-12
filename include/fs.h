#ifndef __FS_H__
#define __FS_H__
#include "type.h"
extern t_8 *fsbuf;
extern const int FSBUF_SZIE ;
void task_fs();

#define MAGIC_V1	0x111
struct super_block{
	t_32	magic;
	t_32	nr_inodes;
	t_32 nr_sects;
	t_32 nr_imap_sects;
	t_32 nr_smap_sects;
	t_32 n_1st_sect;
	t_32 nr_inode_sects;
	t_32 root_inode;

	t_32 inode_size;
	t_32 inode_isize_off;
	t_32 inode_start_off;

	t_32 dir_ent_size;
	t_32 dir_ent_inode_off;
	t_32 dir_ent_fname_off;

	int sb_dev;

};

#define SUPER_BLOCK_SIZE 56

struct inode{
	t_32 i_mode;
	t_32 i_size;
	t_32 i_start_sect;
	t_32 i_nr_sects;

	t_8 _unused[16];
	int i_dev;
	int i_cnt;
	int i_num;

};

#define INODE_SIZE 32
#define MAX_FILENAME_LEN 12

struct dir_entry {
	int inode_nr;
	char name[MAX_FILENAME_LEN];
};
#define DIR_ENTRY_SIZE sizeof(struct dir_entry)

#define NR_DEFAULT_FILE_SECTS 2048 
#define INVALID_INODE 0
#define ROOT_INODE 1

#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000                                                                                                                      
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE    0010000

#define is_special(m)   ((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||    \
		             (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))
#endif//the head
