#ifndef __FS_H__
#define __FS_H__
#include "type.h"
#include "msg.h"
extern t_8 *fsbuf;
extern const int FSBUF_SIZE ;
extern void task_fs();
extern MESSAGE fs_msg;
#define RD_SECT(dev,sect_nr)  rw_sector(dev, (sect_nr) * SECTOR_SIZE, TASK_FS, fsbuf, SECTOR_SIZE, DEV_READ)
#define WR_SECT(dev,sect_nr)  rw_sector(dev, (sect_nr) * SECTOR_SIZE, TASK_FS, fsbuf, SECTOR_SIZE, DEV_WRITE)

void rw_sector( int device, t_64 pos, int proc_nr,  void *buf, int bytes, int io_type);
#define MAGIC_V1	0x111
//some nr
#define NR_FILES    64
#define NR_FILE_DESC    64  /* FIXME */
#define NR_INODE    64  /* FIXME */
#define NR_SUPER_BLOCK  8
//end

//some length
#define MAX_PATH_LEN  128
struct file_desc{
	int fd_mode;
	int fd_pos;
	struct inode* fd_inode;
};

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

//file type
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000                                                                                                                      
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE    0010000

#define is_special(m)   ((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||    \
		             (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))
//end

extern  struct file_desc    f_desc_table[NR_FILE_DESC];
extern  struct inode        inode_table[NR_INODE];                                                                                               
extern  struct super_block  super_block[NR_SUPER_BLOCK];
extern  struct proc *       pcaller;
extern  struct inode *      root_inode;
//extern  struct dev_drv_map  dd_map[];
 
//function
struct inode *get_inode(int dev, int num);
int do_close();
int do_rw();
int do_open();

#endif
