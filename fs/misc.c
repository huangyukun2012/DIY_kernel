#include "fs.h"
#include "string.h"
#include "proc.h"
#include "err.h"

#define SECTOR_SIZE 512
int search_file(char *path);
int strip_path(char *filename, const char *pathname, struct inode **ppinode);
struct inode * get_inode(int dev, int num);
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
	int i,j;
	char filename[MAX_PATH_LEN];
	memset(filename , 0, MAX_FILENAME_LEN);
	struct inode *dir_inode;
	if(strip_path(filename, path, &dir_inode)!=0)
		return 0;
	if(filename[0]== 0)//path '/'
		return dir_inode->i_num;

	//search the dir for the file
	int dir_sects0_index = dir_inode->i_start_sect;
	int nr_dir_sects = (dir_inode->i_size +SECTOR_SIZE -1)/SECTOR_SIZE;
	int nr_dir_dentries = dir_inode->i_size/DIR_ENTRY_SIZE;

	int m= 0;
	struct dir_entry *pde;
	for (i = 0; i < nr_dir_sects; ++i){
		RD_SECT(dir_inode->i_dev, dir_sects0_index +i);
		pde = (struct dir_entry *)fsbuf;
		for(j=0; j<nr_dir_dentries;j++ , pde++){
			if(memcpy(filename , pde->name, MAX_FILENAME_LEN) == 0)
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
	RD_SECT(dev , 0);
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
		else{
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

