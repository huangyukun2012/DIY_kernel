#include "msg.h"
#include "proc.h"
#include "err.h"
#include "nostdio.h"
#include "hd.h"
#include "drive.h"
#include "config.h"
#include "fs.h"
#include "console.h"
#include "global.h"
#include "debug.h"

t_8 *fsbuf = (t_8 *)0x600000;
const int FSBUF_SIZE = 0x100000;
MESSAGE fs_msg;
struct proc * pcaller=0;
#define SECTOR_SIZE 512
void rw_sector( int device, t_64 pos, int proc_nr, void *buf, int bytes, int io_type);
void init_fs();
static void mkfs();
struct super_block *get_super_block(int dev);
static void read_super_block(int dev);
struct inode * get_inode(int dev, int num);

struct file_desc    f_desc_table[NR_FILE_DESC];
struct inode        inode_table[NR_INODE];                                                                                               
struct super_block  super_block[NR_SUPER_BLOCK];
struct inode *      myroot_inode;


void task_fs()
{
	printl("Task Fs begins:\n");
	init_fs();
	while(1){
		send_recv(RECEIVE, ANY, &fs_msg);
		int src=fs_msg.source;
		pcaller = &proc_table[src];
#ifdef DEBUG
				printf("task_fs: fs_msg.type=%d\n",fs_msg.type);
#endif
		switch(fs_msg.type){
			case OPEN:
				fs_msg.FD=do_open();
				break;
			case READ:
			case WRITE:
				fs_msg.CNT = do_rw();
				break;
			case CLOSE:
				fs_msg.RETVAL = do_close();
				break;
		}

		fs_msg.type = SYSCALL_RET;
		send_recv(SEND, src, &fs_msg);
	}

}

void init_fs()
{
	int i;
	for(i=0; i<NR_FILE_DESC;i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
	for(i=0; i<NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));
	struct super_block *sb = super_block;
	for(; sb<&super_block[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NO_DEV;

	MESSAGE driver_msg;
	driver_msg.type=DEV_OPEN;
	driver_msg.DEVICE = MINOR( ROOT_DEV);//minor dev no is enough , becasue the major is for driver
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH,dd_map[MAJOR(ROOT_DEV)].driver_nr,&driver_msg);
	mkfs();

	read_super_block(ROOT_DEV);
	sb=get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
	struct inode * get_inode(int , int );
	myroot_inode = get_inode(ROOT_DEV , ROOT_INODE);

}

static void mkfs()
{
	MESSAGE driver_msg;
	int i,j;
// get the root logic partion base and size
	int bits_per_sect = SECTOR_SIZE * 8;
	struct part_info geo;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	driver_msg.PROC_NR = TASK_FS;
	driver_msg.BUF = &geo;
	
	driver_msg.type = DEV_IOCTL;
	driver_msg.REQUEST = DIOCTL_GET_GEO;
	
	int major_dev = MAJOR(ROOT_DEV);
	int driver_nr = dd_map[major_dev].driver_nr;
	assert(driver_nr != INVALID_DRIVER);
	send_recv(BOTH, driver_nr , &driver_msg);
	printl("dev size 0x %xsectors\n", geo.size);

	//build super block
	struct super_block sb;
	sb.magic = MAGIC_V1;
	//sb.sb_dev = driver_msg.DEVICE;
	sb.nr_inodes = bits_per_sect;
	sb.nr_inode_sects = (sb.nr_inodes * INODE_SIZE)/SECTOR_SIZE;
	sb.nr_sects = geo.size;
	sb.nr_imap_sects = 1;
	sb.nr_smap_sects = sb.nr_sects / bits_per_sect +1;
	sb.n_1st_sect = 2+sb.nr_imap_sects+sb.nr_smap_sects +sb.nr_inode_sects;
	sb.root_inode = ROOT_INODE;//un
	sb.inode_size = INODE_SIZE;
	struct inode x;
	sb.inode_isize_off = (int )&x.i_size - (int )&x;
	sb.inode_start_off = (int )&x.i_start_sect - (int )&x;
	sb.dir_ent_size = DIR_ENTRY_SIZE;
	struct dir_entry de;
	sb.dir_ent_inode_off = (int )&de.inode_nr - (int )&de;
	sb.dir_ent_fname_off = (int)&de.name - (int )&de;

	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);
	WR_SECT(ROOT_DEV, 1);
	printl("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
			"        inodes:0x%x00, 1st_sector:0x%x00\n",
			geo.base*2,
			(geo.base +1)*2,
			(geo.base +2)*2,
			(geo.base +2+ sb.nr_imap_sects)*2,
			(geo.base +2 +sb.nr_imap_sects +sb.nr_smap_sects ) *2,
			(geo.base +sb.n_1st_sect) *2);

	//inode map
	memset(fsbuf,0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLE+2); ++i){//3 ttys 1 unused, 1 root dir
		fsbuf[0] |= 1<<i;
		
	}

	assert(fsbuf[0]==0x1f);
	WR_SECT(ROOT_DEV, 2);

	/************************/
	/*      secter map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	/*             ~~~~~~~~~~~~~~~~~~~|~   |
	 *                                |    `--- bit 0 is reserved
	 *                                `-------- for `/'
	 */
	for (i = 0; i < nr_sects / 8; i++)
		fsbuf[i] = 0xFF;

	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects);    

	/* zeromemory the rest sector-map */
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nr_smap_sects; i++)
	{
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i);
	}

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4; /* 4 files:
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLE; i++) {
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects);

	/************************/
	/*          `/'         */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry * pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = 1;
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLE; i++) {
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		sprintf(pde->name, "dev_tty%d", i);
	}
	
	WR_SECT(ROOT_DEV, sb.n_1st_sect);
}

void rw_sector( int device, t_64 pos, int proc_nr,  void *buf, int bytes, int io_type)
{

	#ifdef DEBUG
	if(proc_nr !=3){
	printf("device:%d ", device);
	printf("pos:%x ", pos);
	printf("proc_nr:%d ", proc_nr);
	printf("buf:%x ", buf);
	printf("bytes:%x ", bytes);
	printf("io_type:%d\n", io_type);
	}
	
	#endif
	MESSAGE msg;
	msg.type=io_type;
	msg.DEVICE=MINOR(device);
	msg.POSITION=pos;

	msg.PROC_NR=proc_nr;
	msg.BUF=buf;

	msg.CNT=bytes;

	int major_nr_dev = MAJOR(device);
	int driver = dd_map[major_nr_dev].driver_nr;
	assert(driver != INVALID_DRIVER);
#ifdef DEBUG_RW
	#ifdef DEBUG
	if(DEBUG_rw_sector ==1){
		printf("rw_sector: sending msg to %d\n", driver );
	}

	#endif
#endif
	send_recv(BOTH, driver, &msg);

}

static void read_super_block(int dev)
{// the dev is full dev no
	int i ;
	MESSAGE driver_msg;
	driver_msg.type = DEV_READ;
	driver_msg.DEVICE = MINOR(dev);
	driver_msg.POSITION = SECTOR_SIZE *1;
	driver_msg.BUF = fsbuf;
	driver_msg.CNT = SECTOR_SIZE;
	driver_msg.PROC_NR = TASK_FS;

	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	
	for(i= 0;i<NR_SUPER_BLOCK; i++)
		if(super_block[i].sb_dev == NO_DEV)
			break;
	if(i == NR_SUPER_BLOCK)
		panic("super_block slots used up");
	assert(i==0);

	struct super_block *psb = (struct super_block *)fsbuf;

	super_block[i]= *psb;
	super_block[i].sb_dev = dev;
}

struct super_block *get_super_block(int dev)
{
	struct super_block * sb = super_block;
	for(; sb< &super_block[NR_SUPER_BLOCK]; sb++)
	{
		if(sb->sb_dev == dev)
			return sb;
	}
	panic("super block of device %d not found\n", dev);
	return 0;
}
