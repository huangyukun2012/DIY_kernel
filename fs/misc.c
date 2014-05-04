#include "fs.h"
#include "string.h"
#include "proc.h"
#include "err.h"
#include "debug.h"
#include "unistd.h"
#include "drive.h"

#define SECTOR_SIZE 512
int search_file(char *path);
int strip_path(char *filename, const char *pathname, struct inode **ppinode);
struct inode * get_inode(int dev, int num);
int do_stat();
extern struct inode *myroot_inode;

int strip_path(char *filename, const char *pathname, struct inode **ppinode)
{
	const char *s=pathname;
	char * t = filename;

	if(s==0)
		return -1;
	if(*s=='/')
		s++;
	while(*s){
		if(*s== '/')
			return -1;
		*t++=*s++;
		if(t-filename>= MAX_FILENAME_LEN)
			break;
	}
	*t=0;
	*ppinode = myroot_inode;
	return 0;
}


//path:the full name of the file
//return:inode num of the file if success, otherwise zero
int search_file(char *path)
{
#ifdef DEBUG_rw
	printl("search_file : path =%s|\n", path );
#endif
	int i,j;
	char filename[MAX_PATH_LEN];
	memset(filename , 0, MAX_FILENAME_LEN);
	struct inode *dir_inode;
	if(strip_path(filename, path, &dir_inode)!=0)
		return 0;
#ifdef DEBUG_rw
	printl("search_file : filename =%s|\n", filename );
#endif
	if(filename[0]== 0)//path '/'
		return dir_inode->i_num;

	//search the dir for the file
	int dir_sects0_index = dir_inode->i_start_sect;
	int nr_dir_sects = (dir_inode->i_size +SECTOR_SIZE -1)/SECTOR_SIZE;
	int nr_dir_dentries = (dir_inode->i_size)/DIR_ENTRY_SIZE;

	int m= 0;
	struct dir_entry *pde;
	for (i = 0; i < nr_dir_sects; ++i){//里面有空的
		RD_SECT(dir_inode->i_dev, dir_sects0_index +i);
		pde = (struct dir_entry *)fsbuf;
		for(j=0; j<SECTOR_SIZE/DIR_ENTRY_SIZE;j++ , pde++){
			if(memcmp(filename , pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inode_nr;
			if(++m >nr_dir_dentries)
				break;
		}
		if(m> nr_dir_dentries)
			break;
	}
	return 0;
}
//dev :
//num :the no of inodg
//return :the ptr to the inode
struct inode * get_inode(int dev, int num)
{
//	RD_SECT(dev , 0);
	if(num == 0)
		return 0;
	struct inode *p;
	struct inode *q =0;

	for(p = &inode_table[0]; p< &inode_table[NR_INODE]; p++){
		if(p->i_cnt){//we find it in inode table
			if((p->i_dev == dev )&& (p->i_num == num)){
				p->i_cnt++;
				return p;
			}
		}
		else{//p->i_cnt==0
			if(!q){//the first cnt=0 inode
				q=p;
			}
		}
	}
	//
	if(!q)
		panic("the inode talbe is full\n");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;
	struct super_block * get_super_block(int dev);
	struct super_block *sb = get_super_block(dev);
	int blk_nr = 2+sb->nr_imap_sects + sb->nr_smap_sects+ ((num-1)/(SECTOR_SIZE /INODE_SIZE));//inode_nr==0 is not used

	RD_SECT(dev, blk_nr);
	struct inode *pinode= (struct inode *)((t_8 *)fsbuf + ((num-1)%(SECTOR_SIZE / INODE_SIZE))*INODE_SIZE );
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode ->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
	
}

void put_inode(struct inode * pinode)
{
	assert(pinode->i_cnt >0);
	pinode->i_cnt--;
}

//when inode is changed , write it to disk
void sync_inode(struct inode *p)
{
	struct inode *pinode;
	struct super_block * get_super_block(int dev);
	struct super_block *sb = get_super_block(p->i_dev);
	int blk_nr = 2+sb->nr_imap_sects + sb->nr_smap_sects+ ((p->i_num-1)/(SECTOR_SIZE /INODE_SIZE));//inode_nr==0 is not used
	RD_SECT(p->i_dev, blk_nr);
	pinode = (struct inode *)((t_8*)fsbuf + ((p->i_num-1)%(SECTOR_SIZE/INODE_SIZE)) *INODE_SIZE);

	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr);
}

/**************************************************************
 *               do_stat
 **************************************************************
	@function:get the information of a pathname
		The information we need for stat hided in the inode. So we just need to get the info of inode of pathname.
	@input:
	@return:0 if sucess
**************************************************************/
int hykdo_stat()
{
	char filename[MAX_FILENAME_LEN];
	char pathname[MAX_PATH_LEN];
	int pathname_len = fs_msg.NAME_LEN;
	int src = fs_msg.source;
//	pathname = fs_msg.PATHNAME; /*no!!! you can not do this directly! For they are not in the same address space*/
	phys_copy( (void *)va2la(TASK_FS, pathname),\
				(void *)va2la(src, fs_msg.PATHNAME),\
				fs_msg.NAME_LEN);
	pathname[pathname_len] = 0;

	int inode_nr_path = search_file(pathname);
	if(inode_nr_path == INVALID_INODE){
		printl("[FS]:stat faild becase of invalid inode\n");
		return -1;
	}
	struct inode *dir_inode;
	if(strip_path(filename, pathname, &dir_inode)!=0){
		assert(0);
	}
	struct inode *file_inode=get_inode(dir_inode->i_dev, inode_nr_path);

	struct stat s;
	s.dev = file_inode->i_dev;
	s.size = file_inode->i_size;
	s.mode = file_inode->i_mode;
	s.ino = file_inode->i_num;
	s.rdev = is_special(file_inode->i_mode) ? file_inode->i_start_sect: NO_DEV;

	put_inode(file_inode);

	phys_copy( (void *)va2la(src, fs_msg.BUF),\
				(void *)va2la(TASK_FS, &stat),\
				sizeof(struct stat));
	printl("fs_msg.buf.size:%x", ((struct stat *)va2la(src, fs_msg.BUF))->size);	       
	return 0;
}

int do_stat()
{
	char pathname[MAX_PATH_LEN]; /* parameter from the caller */
	char filename[MAX_PATH_LEN]; /* directory has been stipped */

	/* get parameters from the message */
	int name_len = fs_msg.NAME_LEN;	/* length of filename */
	int src = fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH_LEN);
	phys_copy((void*)va2la(TASK_FS, pathname),    /* to   */
		  (void*)va2la(src, fs_msg.PATHNAME), /* from */
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		printl("{FS} FS::do_stat():: search_file() returns "
		       "invalid inode: %s\n", pathname);
		return -1;
	}

	struct inode * pin = 0;

	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0) {
		/* theoretically never fail here
		 * (it would have failed earlier when
		 *  search_file() was called)
		 */
		assert(0);
	}
	pin = get_inode(dir_inode->i_dev, inode_nr);

	struct stat s;		/* the thing requested */
	s.dev = pin->i_dev;
	s.ino = pin->i_num;
	s.mode= pin->i_mode;
	s.rdev= is_special(pin->i_mode) ? pin->i_start_sect : NO_DEV;
	s.size= pin->i_size;

	put_inode(pin);

	phys_copy((void*)va2la(src, fs_msg.BUF), /* to   */
		  (void*)va2la(TASK_FS, &s),	 /* from */
		  sizeof(struct stat));

	return 0;
}
