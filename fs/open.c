
#include "fs.h"
#include "err.h"
#include "string.h"
#include "proc.h"
#include "msg.h"
#include "global.h"
#include "hd.h"
#include "nostdio.h"
#include "drive.h"
int do_open();
static struct inode *create_file(char *path, int flags);
static int alloc_smap_bit(int dev, int nr_sects_to_alloc);
static struct inode* new_inode(int dev, int inode_nr, int start_sect);
static int alloc_imap_bit(int dev);
static void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename);
int do_close();
#define SECTOR_SIZE 512

int do_open()
{//return the fd of the file in fs_msg
#ifdef DEBUG
	printf("do_open: ");
#endif
	int fd=-1;
	char pathname[MAX_PATH_LEN];
	int flags = fs_msg.FLAGS;
	int name_len = fs_msg.NAME_LEN;
	int src = fs_msg.source;

	assert(name_len < MAX_PATH_LEN);
	phys_copy((void *)va2la(TASK_FS, pathname), (void *)va2la(src,  fs_msg.PATHNAME), name_len);
	pathname[name_len] = 0;
	
	int i;
	for (i = 0; i < NR_FILES; ++i){//find the first empty fd of a proc
		if(pcaller->filp[i] == 0){
			fd = i;
			break;
		}
	}

	if( fd< 0 || fd>= NR_FILES)
		panic("filep[] if full (pid:%d)", proc2pid(pcaller));

	for (i = 0; i < NR_FILE_DESC; ++i){ //find the first empty desc in f_desc_table 
		if(f_desc_table[i].fd_inode == 0)
			break;
	}

	if(i >= NR_FILE_DESC){
		panic("f_desc_table is full (pid:%d)",proc2pid(pcaller));
	}
	
	int inode_nr = search_file(pathname);//find the inode num of a file:pathname
	
	struct inode *pin =0;
	if(flags&O_CREAT){
		if(inode_nr){//can no create existing file
			printl("file exists.\n");
			return -1;
		}
		else{//create file and return its inode
			pin = create_file(pathname, flags);
		}
	}
	else{//flags:read or write
		assert(flags & O_RDWR);
		char filename[MAX_PATH_LEN];
		struct inode * dir_inode;
		if(strip_path(filename, pathname, &dir_inode) != 0){//return dir_inode and filename
			return -1;
		}
		pin = get_inode(dir_inode->i_dev, inode_nr);//pin is the inode of our dest file

	}

	if(pin){//inode of the dest file
		pcaller->filp[fd] = &f_desc_table[i];//connect proc with file_descriptor, watch out for i

		f_desc_table[i].fd_inode = pin;//connect file_descriptor with inode
		f_desc_table[i].fd_mode = flags;
		f_desc_table[i].fd_pos = 0;

		int imode = pin->i_mode & I_TYPE_MASK;

		if(imode == I_CHAR_SPECIAL){//specal char device
			MESSAGE driver_msg;
			driver_msg.type = DEV_OPEN;
			int dev = pin->i_dev;//err: it is?
				
			driver_msg.DEVICE = MINOR(dev);
			assert(MAJOR(dev) == 4);
			assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
			
			send_recv(BOTH,dd_map[MAJOR(dev)].driver_nr, &driver_msg);
		}
		else if(imode == I_DIRECTORY){// directory file
			assert(pin->i_num == ROOT_INODE);
		}
		else{//regular file
			assert(pin->i_mode == I_REGULAR);
		}
	}
	else{
		return -1;
	}
	return fd;
}

/*----------------------create_file()--------------------------*/
static struct inode *create_file(char *path, int flags)
{
#ifdef DEBUG
	printf("createfile:%d ");
#endif
	char filename[MAX_PATH_LEN];
	struct inode *dir_inode;
	if(strip_path(filename, path, &dir_inode) != 0)
		return 0;
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, NR_DEFAULT_FILE_SECTS);
	struct inode *new_ino = new_inode(dir_inode->i_dev, inode_nr, free_sect_nr);

	new_dir_entry(dir_inode, new_ino->i_num, filename);
	return new_ino;
}

static int alloc_imap_bit(int dev)
{
#ifdef DEBUG
	printf("alloc_imap_bit: ");
#endif
	int inode_nr =0;
	int i,j,k;//sect, byte, bit
	int imap_blk0_index = 2;
	struct super_block *get_super_block(int dev);
	struct super_block *sb = get_super_block(dev);
	for (i = 0; i < sb->nr_imap_sects; ++i){
		RD_SECT(dev, imap_blk0_index +i);
		for(j =0; j< SECTOR_SIZE; j++){
			if(fsbuf[j] == 0xff)
			 	continue;
			for(k=0; ((fsbuf[j]>>k)&1)!=0;k++){
				//skip 1
			}
			inode_nr = (i*SECTOR_SIZE +j)*8 + k;
			fsbuf[j] |= (1<<k);

			WR_SECT(dev,imap_blk0_index+i);
			break;
		}
		return inode_nr;
	}
	panic("inode-map is probally full.\n");
	return 0;
}

static int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	int i,j,k;//sector ,byte ,bite index
	struct super_block *get_super_block(int dev);
	struct super_block *sb = get_super_block(dev);

	int smap_blk0_index = 2+ sb->nr_imap_sects;
	int free_sect_index =0;

	for (i = 0; i < sb->nr_smap_sects; ++i){
		RD_SECT(dev, smap_blk0_index +i);
		for(j=0; j<SECTOR_SIZE && nr_sects_to_alloc >0;j++){
			k=0;
			if(!free_sect_index ){
				if(fsbuf[j]==0xff){
					continue;
				}
				for(;((fsbuf[j] >>k) &1)!=0;k++){
					//skip 1 bit
				}
				free_sect_index =(i*SECTOR_SIZE +j)*8 +k-1+sb->n_1st_sect;
			}
			for(;k<8;k++){
				assert(((fsbuf[j]>>k)& 1)==0);
				fsbuf[j] |= (1 <<k);
				if(--nr_sects_to_alloc==0)
					break;
			}
		}
		if(free_sect_index)
			WR_SECT(dev,smap_blk0_index+i);
		if(nr_sects_to_alloc == 0)
			break;
	}
	assert(nr_sects_to_alloc == 0);
	return free_sect_index;
}

/*---------------------------new_inode-----------------------*/
static struct inode* new_inode(int dev, int inode_nr, int start_sect)
{
	struct inode * new_inode = get_inode(dev, inode_nr);
	new_inode->i_mode = I_REGULAR;
	new_inode->i_size = 0;
	new_inode->i_start_sect = start_sect;
	new_inode->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;
	sync_inode(new_inode);//write to the inode array
	return new_inode;
}

static void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename)
{// add a dentry made of inode_nr and filename to dir_inode
	int dir_blk0_index = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE -1)/SECTOR_SIZE;
	int nr_dir_entries = dir_inode->i_size/DIR_ENTRY_SIZE;

	int m= 0;
	struct dir_entry *pde;
	struct dir_entry * new_de=0;
	int i,j;//sector 
	for (i = 0; i < nr_dir_blks; ++i){
		RD_SECT(dir_inode->i_dev, dir_blk0_index +i);
		pde = (struct dir_entry *)fsbuf;
		for(j=0; j<SECTOR_SIZE/DIR_ENTRY_SIZE; j++, pde++){
			if(++m > nr_dir_entries)
				break;
			if(pde->inode_nr == 0){
				new_de = pde;
				break;
			}
		}
		if(m > nr_dir_entries || new_de)
			break;
	}
	if(!new_de){
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);

	WR_SECT(dir_inode->i_dev, dir_blk0_index + i);

	sync_inode(dir_inode);
}

int do_close()
{
	int fd=fs_msg.FD;
	put_inode(pcaller->filp[fd]->fd_inode);
	pcaller->filp[fd]->fd_inode =0;
	pcaller->filp[fd]=0;
	return 0;

}
