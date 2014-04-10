#include "msg.h"
#include "proc.h"
#include "hd.h"
#include "err.h"
#include "nostdio.h"
#include "type.h"
#include "proto.h"

#define SECTOR_SIZE 512
 t_8 hd_status;
 t_8 hdbuf[2*SECTOR_SIZE];

void	init_hd			();
void	hd_cmd_out		(struct hd_cmd* cmd);
int	waitfor			(int mask, int val, int timeout);
void	interrupt_wait		();
void	hd_identify		(int drive);
void	print_identify_info	(t_16* hd_info);
void 	hd_handler();
void task_hd()
{
	MESSAGE msg;
	init_hd();
	while(1){
		send_recv(RECEIVE,ANY,&msg);

		int src=msg.source;

		switch(msg.type){
			case DEV_OPEN:
					hd_identify(0);
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
{
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

