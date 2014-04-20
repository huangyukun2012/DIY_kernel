#include "fs.h"
#include "err.h"
#include "string.h"
#include "proc.h"
#include "msg.h"
#include "global.h"
#include "hd.h"
#include "nostdio.h"
#include "drive.h"
#include "math.h"
#include "debug.h"
static struct inode *create_file(char *path, int flags);
static int alloc_smap_bit(int dev, int nr_sects_to_alloc);
static struct inode* new_inode(int dev, int inode_nr, int start_sect);
static int alloc_imap_bit(int dev);
static void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename);
int do_close();
int do_rw();
int do_open();

#define SECTOR_SIZE 512

#define SECTOR_SIZE_SHIFT 9

	int DEBUG_rw_sector = 0;

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
#ifdef DEBUG
	if(inode_nr == 0)
		printf("do_open: get inode_nr %d\n",inode_nr );
#endif

	
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
			printf("filepath invalid\n");
			return -1;
		}
		if(inode_nr == 0)
			printf("do_open: -> > get_inode( , %d)\n",inode_nr );
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
			int dev = pin->i_start_sect;//err: it is?
				
			
			driver_msg.DEVICE = MINOR(dev);
			if(MAJOR(dev)!=DEV_CHAR_TTY ){
				printf("do_open:major-DEV is not DEV_CHAR_TTY ");
			}
			assert(MAJOR(dev) == DEV_CHAR_TTY);
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
		printf("can not find the inode:inode==0\n");
		return -1;
	}
	return fd;
}

/*----------------------create_file()--------------------------*/
static struct inode *create_file(char *path, int flags)
{
#ifdef DEBUG
	printf("createfile:\n ");
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
	printf("alloc_imap_bit:\n ");
#endif
	int inode_nr =0;
	int i,j,k;//sect, byte, bit
	int imap_blk0_index = 2;
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


//return :how many byte have been read/written
//note:sector map is not needed to update
//input the fd of the file:must in mem
int do_rw()
{
	int fd = fs_msg.FD;
	void *buf = fs_msg.BUF;
	int len = fs_msg.CNT;
	int src = fs_msg.source;
	#ifdef DEBUG_RW
		printf("do_rw:fd=%d, len=%d\n", fd,len);
	#endif
	assert((pcaller->filp[fd] >= &f_desc_table[0]) &&
				(pcaller->filp[fd] < &f_desc_table[NR_FILE_DESC]));
	if(!(pcaller->filp[fd]->fd_mode & O_RDWR)){

	#ifdef DEBUG_RW
		printf("do_rw:access to file was denied\n");
	#endif
		return -1;
	}

	int pos = pcaller->filp[fd]->fd_pos;
	struct inode * pin = pcaller->filp[fd]->fd_inode;
	assert(pin >= &inode_table[0] && pin < &inode_table[NR_INODE]);
	
	int imode = pin->i_mode & I_TYPE_MASK;
	if(imode == I_CHAR_SPECIAL){
		int t = fs_msg.type == READ? DEV_READ:DEV_WRITE;
		fs_msg.type = t;

		int dev = pin->i_start_sect;
		assert(MAJOR(dev)==4);

		fs_msg.DEVICE= MINOR(dev);
		fs_msg.BUF = buf;
		fs_msg.CNT = len;
		fs_msg.PROC_NR = src;

		int driver_no = dd_map[MAJOR(dev)].driver_nr;
		assert(driver_no != INVALID_DRIVER);
		send_recv(BOTH, driver_no, &fs_msg);//交给相应的驱动程序处理
		assert(fs_msg.CNT==len);
		

		#ifdef DEBUG_RW
			printf("out do_rw\n");
		#endif
		return fs_msg.CNT;
	}
	else{//regular file or directory
		#ifdef DEBUG_RW
			printf("is regular file\n");
		#endif
		assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		assert(fs_msg.type == READ || fs_msg.type == WRITE);

		int pos_end;
		if(fs_msg.type == READ)
		{
			pos_end = min(pos +len, pin->i_size);
		}
		else
			pos_end = min(pos + len, pin->i_nr_sects * SECTOR_SIZE);

		int off= pos%SECTOR_SIZE;
		int rw_sect_start =pin->i_start_sect +(pos>>SECTOR_SIZE_SHIFT);
		int rw_sect_end	 = pin->i_start_sect +(pos_end >> SECTOR_SIZE_SHIFT);

		int chunk = min(rw_sect_end - rw_sect_start +1, FSBUF_SIZE >>SECTOR_SIZE_SHIFT);

		int bytes_rw = 0;
		int bytes_left = len;
		int i;
#ifdef DEBUG_RW
	printf("do_rw:from %d to %d (sector)\n",rw_sect_start, rw_sect_end );
	DEBUG_rw_sector = 1;
#endif
		for (i = rw_sect_start ; i <= rw_sect_end; i+=chunk){
			int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
			rw_sector( pin->i_dev,i* SECTOR_SIZE ,TASK_FS,fsbuf,chunk * SECTOR_SIZE ,DEV_READ);
#ifdef DEBUG_RW
	printf("read rector end in do_rw()\n");
#endif

			if(fs_msg.type == READ){
				phys_copy((void *)va2la(src, buf + bytes_rw), (void *)va2la(TASK_FS,fsbuf+off), bytes);

			}
			else{
				phys_copy((void *)va2la(TASK_FS, fsbuf+ off), (void *)va2la(src, buf + bytes_rw), bytes);
				rw_sector( pin->i_dev,i* SECTOR_SIZE ,TASK_FS,fsbuf,chunk * SECTOR_SIZE ,DEV_WRITE);
			}

			off = 0;
			bytes_rw += bytes;
			pcaller->filp[fd]->fd_pos +=bytes;
			bytes_left -= bytes;
		}

		if(pcaller->filp[fd]->fd_pos > pin->i_size){
			pin->i_size = pcaller->filp[fd]->fd_pos;

			sync_inode(pin);
		}
#ifdef DEBUG_RW
	printf("out do_rw\n");
#endif
		return bytes_rw;
	}
}

/* ***********************
 * input:
 * @pathname:the file to be unlinked
 * operatation:
 * imap, sectormap, the parent dir, inod array update
 * output:
 * return:
 * 0 if seccessed, -1 if failed
 * */
int do_unlink()
{
	//get the input file name from a source
	char pathname[MAX_PATH_LEN];
	int name_len = fs_msg.NAME_LEN;
	int proc_source = fs_msg.source;
	void *dest = (void *)va2la(TASK_FS, pathname);
	void *src = (void *)va2la(proc_source, fs_msg.PATHNAME );
	phys_copy(dest, src, name_len);
	pathname[name_len]=0;

	if(strcmp(pathname, "/")== 0){
		printf("you can no remove the root dir\n");
		return -1;
	}
	
	int inode_nr=search_file(pathname);
	assert(inode_nr !=0);
	if(inode_nr == 0){
		printf("in do_unlink:search_file return invalid inode\n");
		return -1;
	}

	char filename[MAX_PATH_LEN];
	struct inode *dir_inode;
	if(strip_path(filename, pathname, &dir_inode) != 0){
		printf("in do_unlink:strip_path failed\n");
		return -1;
	}
	struct inode *file_inode= get_inode(dir_inode->i_dev, inode_nr);

	if(file_inode->i_mode !=I_REGULAR){
		printf("can not remove file %s, for it is not an regular file\n", pathname);
		return -1;
	}

	if(file_inode->i_cnt >1){//we opened this file, so the cnt is 1 or more
		printf("you can not remove file %s, for it is opened by others\n", pathname);
		return -1;
	}
	struct super_block *sb=get_super_block(file_inode->i_dev);


	/*****************imap*******************/
	int byte_index= inode_nr/8;
	int bit_index = inode_nr%8;
	assert(byte_index<SECTOR_SIZE);
	RD_SECT(file_inode->i_dev, 2);//skip boot and sb block
	assert(fsbuf[byte_index] & (1<<bit_index));
	fsbuf[byte_index] &= (~(1<<bit_index));
	WR_SECT(file_inode->i_dev, 2);//skip boot and sb block
	//end
	
	/*****************smap*******************/
	/*1)find the start bit of the file : offset of the smap : bit_start -- byte_start -- sector_start
	 * 2)find how many bits need to clear:bits_to_clear -- bytes to clear -- sectors to clear
	 * 3)read one sector from the disk from the sector of bit_start,
	 *   update one byte once, until clear up or the sector used up
	 * 4)if it is clear up, then go to end; otherwise , write the sector and read one sector funther more
	 * */

	int bit_start=file_inode->i_start_sect - sb->n_1st_sect+1;//in bit
	int byte_start = bit_start/8;
	int clear_start_sect = 2+sb->nr_imap_sects + byte_start/SECTOR_SIZE;

	int first_byte_in_sector=byte_start%SECTOR_SIZE;
	int first_bit_in_byte = bit_start%8;

	int bits_to_clear=file_inode->i_nr_sects;
	int bytes_to_clear=0;

	if(first_bit_in_byte!=0){
		bytes_to_clear = (bits_to_clear - (8- first_bit_in_byte))/8;//注意，头和尾部的smap位不在字节边界，所以要除去
	}
	else{
		bytes_to_clear = bits_to_clear/8;
	}
	
	
	RD_SECT(file_inode->i_dev, clear_start_sect);
	int i;
	for(i=first_bit_in_byte;i<8&&bits_to_clear;i++){//clear the first byte
		assert( fsbuf[first_byte_in_sector] & (1<<i));
		fsbuf[first_byte_in_sector] &= ~(1<<i);
		bits_to_clear--;
	}
	//clear to the second to the last
	i=first_byte_in_sector + 1;//byte 
	int j;
	for(j=0; j<bytes_to_clear ;j++){
		if(i==SECTOR_SIZE){
			i=0;
			WR_SECT(file_inode->i_dev, clear_start_sect);
			clear_start_sect++;
			RD_SECT(file_inode->i_dev, clear_start_sect);
		}
		assert(fsbuf[i]==0xff);
		fsbuf[i]=0;
		i++;
		bits_to_clear-=8;
	}
	//clear the last byte
	if(i==SECTOR_SIZE){
		i=0;
		WR_SECT(file_inode->i_dev, clear_start_sect);
		clear_start_sect++;
		RD_SECT(file_inode->i_dev, clear_start_sect);
	}

	for(j=0;j<bits_to_clear;j++){//clear the last byte
		assert( fsbuf[i] & (1<<j));
		fsbuf[i] &= ~(1<<j);
	}

	WR_SECT(file_inode->i_dev, clear_start_sect);

	/*************************clear inode itself
	 * we have already get the inode ptr, now we will clear the content of the *ptr
	 * */
	file_inode->i_mode=0;
	file_inode->i_size =0;
	file_inode->i_start_sect = 0;
	file_inode->i_nr_sects = 0;
	sync_inode(file_inode);
	put_inode(file_inode);

	/**********set the inode_nr to 0 in the directory, and change the size of dir if necessary
	 * */
	int dir_start_sect = dir_inode->i_start_sect;
	int dir_size_sects = (dir_inode->i_size + SECTOR_SIZE -1)/SECTOR_SIZE;
	int dir_entries_nr = dir_inode->i_size/DIR_ENTRY_SIZE;

	struct dir_entry *pde;
	int isfound=0;
	int m=0;
	int dir_size=0;
	int lastp=0;//size in byte

	for(i=0;i<dir_size_sects;i++){
		RD_SECT(dir_inode->i_dev, dir_start_sect + i);
		
		pde=(struct dir_entry *)fsbuf;
		int j;
		for(j=0;j<SECTOR_SIZE/DIR_ENTRY_SIZE;j++){
			if(++m>dir_entries_nr)//the end of dir
				break;
#ifdef UNLINK_DEBUG
				printf("searching for inode_nr:%d\n", pde->inode_nr );
#endif
			if(pde->inode_nr == inode_nr){
				memset(pde, 0, DIR_ENTRY_SIZE);
				WR_SECT(dir_inode->i_dev, dir_start_sect +i);
				isfound=1;//
				break;
			}
			else{
				dir_size+=DIR_ENTRY_SIZE;
				if(pde->inode_nr !=0){
					lastp=dir_size;
				}
			}
			pde++;
		}
		if(m>dir_entries_nr || isfound==1)
			break;
	}
	assert(isfound);
	if(m==dir_entries_nr){//delete the last entry
		dir_inode->i_size = lastp;
	}
	
	return 0;
}
