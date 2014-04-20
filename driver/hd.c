#include "msg.h"
#include "proc.h"
#include "hd.h"
#include "err.h"
#include "nostdio.h"
#include "type.h"
#include "proto.h"
#include "math.h"
#include "string.h"
#include "debug.h"
#define SECTOR_SIZE 512
#define SECTOR_SIZE_SHIFT 9
 t_8 hd_status;
 t_8 hdbuf[2*SECTOR_SIZE];
 static struct hd_info hds_info[1];
#define DRV_OF_DEV(dev) (dev<= MAX_PRIM_INDEX ? \
							dev / NR_PRIM_PER_DRIVE: \
							(dev- MINOR_hd1a)/ NR_SUB_PER_DRIVE)

void	init_hd			();
void	hd_cmd_out		(struct hd_cmd* cmd);
int	waitfor			(int mask, int val, int timeout);
void	interrupt_wait		();
void	hd_identify		(int drive);
void	print_identify_info	(t_16* hd_info);
void 	hd_handler();
static void print_hdsinfo(struct hd_info *hdi);
static void partition(int device, int style);
static void get_part_table(int drive, int sector_nr, struct part_entry *entry);
void hd_open(int device);
static hd_close(int device_nr);
static void hd_ioctl(MESSAGE *msgp);
static void hd_rw( MESSAGE *msgp);


void task_hd()
{
	MESSAGE msg;
	init_hd();
	while(1){
		send_recv(RECEIVE,ANY,&msg);

		int src=msg.source;

		switch(msg.type){
			case DEV_OPEN:
					hd_open(msg.DEVICE);
					break;
			case DEV_CLOSE:
					hd_close(msg.DEVICE);
					break;
			case DEV_READ:
			case DEV_WRITE:
					hd_rw(&msg);
					break;
			case DEV_IOCTL:
					hd_ioctl(&msg);
					break;
			default:
					dump_msg("HD driver: unknown msg",&msg);
					spin("FS: main loop (invalid msg,type)");
					break;
		}
		send_recv(SEND,src,&msg);
	}
}
void init_hd()
{
	t_8 * Nr_Drivers_p = (t_8*)(0x475);
	printl("NrDdirves:%d\n",*Nr_Drivers_p);
	assert(*Nr_Drivers_p);

	put_irq_handler(AT_WINI_IRQ,hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);

	int i;
	int nr_drive=sizeof(hds_info) / sizeof(hds_info[0]);
	for(i=0;i < nr_drive; i++)
	{
		memset(&hds_info[i],0, sizeof hds_info[0]);
	}
	hds_info[0].open_cnt = 0;

}

void hd_identify(int drive_no)
{
	struct hd_cmd cmd;
	cmd.device = MAKE_DEVICE_REG(0,drive_no,0);
	cmd.command= ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	port_read(REG_DATA,hdbuf,SECTOR_SIZE);
	print_identify_info((t_16*)hdbuf);

	t_16 * hdinfo = (t_16*)hdbuf;

	hds_info[drive_no ].primary[0].base = 0;

	hds_info[drive_no ].primary[0].size = (( int )hdinfo[61] <<16 ) + hdinfo[60];
}

void print_identify_info(t_16 *hd_info)
{
	int i,k;
	char s[64];

	struct iden_info_ascii{
		int idx;
		int len;
		char *desc;
	}iinfo[]={
		{10,20,"HD SN"},
		{27,40,"HD Model"}	};
	int length=sizeof(iinfo)/sizeof(iinfo[0]);

	for(k=0;k<length;k++)
	{
		char *p=(char *)&hd_info[iinfo[k].idx];
		for(i=0;i<iinfo[k].len/2;i++)
		{
			s[i*2+1] = *p++;//low Bype
			s[i*2] = *p++;
		}
		s[2*i]=0;
		printl("%s: %s\n",iinfo[k].desc, s);

	}

	int capabilities=hd_info[49];
	printl("LBA SUPPORTED: %s\n", (capabilities& 0x200)? "yes":"no");

	int cmd_set_supported=hd_info[83];
	printl("LBA48 SUPPORTED: %s\n", (cmd_set_supported& 0x400)? "yes":"no");

	int sectors=((int )hd_info[61] << 16 ) + hd_info[60];
	printl("hd sieze: %d MB\n",sectors *512 /1000000);

}

 void hd_cmd_out(struct hd_cmd *cmd)
{// we can only send cmd to a hd until it is not busy
	if(!waitfor(STATUS_BSY,0,HD_TIMEOUT))
		panic("hd error,.");

	out_byte(REG_DEV_CTRL,0);
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->sector_count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	out_byte(REG_CMD,     cmd->command);
}

 void interrupt_wait()
{
	MESSAGE msg;
	send_recv(RECEIVE, INTERRUPT, &msg);

}

 int waitfor(int mask, int val, int timeout)
{
	int t=get_ticks();

	while(((get_ticks()-t)*1000/HZ) < timeout){
		if((in_byte(REG_STATUS) & mask) == val)
			return 1;
	}

	return 0;
}

void hd_handler(int irq)
{
	hd_status = in_byte(REG_STATUS);
	inform_int(TASK_HD);
}

void hd_open(int device)
{
	int drive = DRV_OF_DEV( device);
	assert( drive == 0);
	hd_identify(drive);
	
	if(hds_info[drive].open_cnt ++ == 0){
		partition(drive * (NR_PART_PER_DRIVE +1), P_PRIMARY);
		print_hdsinfo(&hds_info[drive]);
	}

}
static void get_part_table(int drive, int sector_nr, struct part_entry *entry)
{//get partition table of a drive not for a entended partion
	struct hd_cmd cmd;
	cmd.features = 0;
	cmd.sector_count = 1;
	cmd.lba_low = sector_nr & 0xff;
	cmd.lba_mid = (sector_nr >>8) &0xff;
	cmd.lba_high = (sector_nr >> 16)& 0xff;
	cmd.device = MAKE_DEVICE_REG(1,drive, (sector_nr >>24) &0xf);
	cmd.command = ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();
	port_read( REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy( entry, hdbuf + PARTITION_TABLE_OFFSET, sizeof(struct part_entry) *NR_PART_PER_DRIVE);
}
static void partition(int device, int style)
{//changed hds_info
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info *hdi = &hds_info[drive];
	struct part_entry part_table[NR_SUB_PER_DRIVE];

	if(style == P_PRIMARY){
		get_part_table(drive, drive, part_table);

		int nr_primary_parts=0;
		for (i = 0; i < NR_PART_PER_DRIVE; ++i){
			if (part_table[i].sys_id == NO_PART){
				continue;
			}
			nr_primary_parts++;
			int dev_nr=i+1;
			hdi->primary[dev_nr].base = part_table[i].start_sect;
			hdi->primary[dev_nr].size = part_table[i].nr_sects;

			if(part_table[i].sys_id == EXT_PART){
				partition(device + dev_nr, P_EXTENDED);
			}
		}
		assert(nr_primary_parts != 0);
	}
	else if(style== P_EXTENDED){
		printl("in ext mode\n");
		int j= device%NR_PRIM_PER_DRIVE;
		int ext_start_sect = hdi->primary[j].base;
		int s=ext_start_sect;
		int nr_1st_sub = (j-1) * NR_SUB_PER_PART ;

		for (i = 0; i < NR_SUB_PER_PART; ++i){
			int dev_nr = nr_1st_sub +i;
			get_part_table(drive , s, part_table);
			hdi->logical[dev_nr].base = s + part_table[0].start_sect;
			hdi->logical[dev_nr].size = part_table[0].nr_sects;

			s=ext_start_sect + part_table[1].start_sect;

			if(part_table[1].sys_id == NO_PART)
				break;
			
		}
	}
	else{
		assert(0);
	}
}

static void print_hdsinfo(struct hd_info *hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE+1; ++i){
		printl("%sPART_%d: base %d(0x%x), size %d (0x%x) (in sector) \n", 
				i==0? " ":"    ",
				i,
				hdi->primary[i].base,
				hdi->primary[i].base,
				hdi->primary[i].size,
				hdi->primary[i].size);
		
	}
	
	for (i = 0; i < NR_SUB_PER_DRIVE; ++i){
		if(hdi->logical[i].size == 0)
			continue;
		printl("%sPART_%d: base %d(0x%x), size %d (0x%x) (in sector) \n", 
				i==0? " ":"    ",
				i,
				hdi->logical[i].base,
				hdi->logical[i].base,
				hdi->logical[i].size,
				hdi->logical[i].size);
	}

}

static hd_close(int device_nr)
{
	int drive_nr=DRV_OF_DEV(device_nr);
	assert(drive_nr==0);
	hds_info[drive_nr].open_cnt--;

}


static void hd_rw( MESSAGE *msgp)
{
#ifdef DEBUG_RW
	if(DEBUG_rw_sector==1){
		printl("in hd_rw:\n");
	}
#endif
	int drive_nr=DRV_OF_DEV(msgp->DEVICE);
	t_64 pos=msgp->POSITION;

	assert( (pos >> SECTOR_SIZE_SHIFT) < (1<<31));
	assert( (pos & 0x1ff) == 0);

	t_32 start_sector = (t_32) (pos >> SECTOR_SIZE_SHIFT);
	int logic_part_index=(msgp->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	start_sector += (msgp->DEVICE < MAX_PRIM_INDEX ? hds_info[drive_nr].primary[msgp->DEVICE].base : hds_info[drive_nr].logical[logic_part_index].base);

	void *lineraddress=(void *)va2la(msgp->PROC_NR,msgp->BUF);
	int byte_left= msgp->CNT;

	struct hd_cmd cmd;
	cmd.features =0;
	cmd.sector_count = (msgp->CNT + SECTOR_SIZE - 1)/SECTOR_SIZE;
	cmd.lba_low = (start_sector & 0xff);
	cmd.lba_mid = (start_sector >> 8) & 0xff;
	cmd.lba_high = (start_sector >> 16 )& 0xff;
	cmd.device = MAKE_DEVICE_REG(1,drive_nr,(start_sector >>24)& 0xf);
	cmd.command = (msgp->type == DEV_READ)? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd);
#ifdef DEBUG_RW
	if(DEBUG_rw_sector==1){
		printl("hd_rw: cmd to hd was sended\n");
		printl("msg.CNT= %d", msgp->CNT);
	}
#endif

	while(byte_left){
#ifdef DEBUG_RW
	if(DEBUG_rw_sector==1){
		printl(">byte_left:%d  ", byte_left);
	}
#endif
		int bytes=min(SECTOR_SIZE, byte_left);
		if(msgp->type == DEV_READ){
			interrupt_wait();//等待硬盘中断到来
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(lineraddress, (void *)va2la(TASK_HD, hdbuf), bytes);

		}
		else{
			if(!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				panic("hd writing error");
			port_write(REG_DATA,lineraddress, bytes);
			interrupt_wait();
		}
		byte_left -=SECTOR_SIZE;
		lineraddress +=SECTOR_SIZE;
	}
#ifdef DEBUG_RW
	if(DEBUG_rw_sector==1){
		printl("out hd_rw:\n");
	}
#endif
}

/*-----------------------hd_ioctl--------------------*/

static void hd_ioctl(MESSAGE *msgp)
{
	int device_nr = msgp->DEVICE;
	int drive_nr = DRV_OF_DEV(device_nr);
	
	struct hd_info * hdi = &hds_info[drive_nr];

	if(msgp->REQUEST == DIOCTL_GET_GEO){
		void *dst = va2la(msgp->PROC_NR,msgp->BUF);
		void *src = va2la(TASK_HD, device_nr < MAX_PRIM_INDEX? &hdi->primary[device_nr] : 
				&hdi->logical[(device_nr - MINOR_hd1a )%  NR_SUB_PER_DRIVE]);
		phys_copy(dst,src, sizeof (struct part_info));
	}
	else{
		assert(0);
	}
}
