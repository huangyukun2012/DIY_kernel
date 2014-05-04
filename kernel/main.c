
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "proc.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "global.h"
#include "stdio.h"
#include "msg.h"
#include "fs.h"
#include "unistd.h"
#include "err.h"
#include "kernel.h"
#include "keyboard.h"
#include "math.h"

#define SECTOR_SIZE 512

struct posix_tar_header{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];

};

void untar(const char* filename);
static void shutdown_proc(int n);
static void set_proc_prio(int proc, int myprio);
/*======================================================================*
                            tinix_main
 *======================================================================*/
PUBLIC int tinix_main()
{
	disp_str("-----\"tinix_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL ;

	int i, j;
	t_8 privilege;
	t_8 rpl;
	int eflags;
	int prio;

	for(i=0;i<NR_PROCS;i++){
		if(i>= NR_TASKS + NR_NATIVE_PROCS){
			p_proc->p_flags= FREE_SLOT;//pflags and free slot
			continue;
		}
		if(i<NR_TASKS){
			p_task=task_table+i;
			privilege=PRIVILEGE_TASK;
			rpl=RPL_TASK;
			eflags=0x1202;
			prio=100;
		}
		else{
			p_task=user_proc_table+i-NR_TASKS;
			privilege=PRIVILEGE_USER;
			rpl=RPL_USER;
			eflags=0x0202;
			prio=3;
		}
		strcpy(p_proc->name, p_task->name);	// name of the process
		p_proc->pid	= i;			// pid
		p_proc->p_parent= 0;

		if(strcmp(p_task->name, "Init") !=0){
			memcpy(&p_proc->ldts[INDEX_LDT_C], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
			memcpy(&p_proc->ldts[INDEX_LDT_RW], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
			p_proc->ldts[INDEX_LDT_C].attr1 = DA_C | privilege << 5;	// change the DPL
			p_proc->ldts[INDEX_LDT_RW].attr1 = DA_DRW | privilege << 5;	// change the DPL

		}
		else{
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			disp_int(k_base);
			disp_str(" ");
			disp_int(k_limit);

			assert(ret == 0);
			init_descriptor(&p_proc->ldts[INDEX_LDT_C] , 0, \
					(k_base + +k_limit) >> LIMIT_4K_SHIFT, DA_32 | DA_LIMIT_4K | DA_C | privilege << 5);
			init_descriptor(&p_proc->ldts[INDEX_LDT_RW] , 0,\
					(k_base + +k_limit) >> LIMIT_4K_SHIFT, DA_32 | DA_LIMIT_4K | DA_DRW | privilege << 5);
		}
		p_proc->regs.cs		= (INDEX_LDT_C ) << 3| SA_TIL | rpl;

		p_proc->regs.ds		= INDEX_LDT_RW <<3 | SA_TIL | rpl;
		p_proc->regs.es		= INDEX_LDT_RW << 3| SA_TIL | rpl;
		p_proc->regs.fs		= INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p_proc->regs.ss		= INDEX_LDT_RW << 3 | SA_TIL | rpl;

		p_proc->regs.gs		= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p_proc->regs.eip	= (t_32)p_task->initial_eip;
		p_proc->regs.esp	= (t_32)p_task_stack;
		p_proc->regs.eflags	= eflags ;	// IF=1, IOPL=1, bit 2 is always 1.

		p_task_stack -= p_task->stacksize;
		p_proc->is_msg_critical_allowed = 1;
	
		p_proc->p_flags=0;
		p_proc->p_msg=0;
		p_proc->p_recvfrom=NO_TASK;
		p_proc->p_sendto=NO_TASK;
		p_proc->has_int_msg=0;
		p_proc->q_sending=0;
		p_proc->next_sending=0;
		
		p_proc->ticks=p_proc->priority=prio;
	

		for(j=0;j<NR_FILES;j++){
			p_proc->filp[j] = 0;
		}

		p_proc++;
		p_task++;

	}

	k_reenter	= 0;
	ticks		= 0;

	p_proc_ready	= proc_table;
	/* shutdown_proc(6); */
	/* shutdown_proc(7); */
	/* shutdown_proc(8); */
	init_clock();
	init_keyboard();

	restart();
	while(1){
				
	}
}

static void shutdown_proc(int n)
{
	proc_table[n].ticks = proc_table[n].priority = 0;
}
static void set_proc_prio(int proc, int myprio)
{
	proc_table[proc].ticks = proc_table[proc].priority = myprio;
}
/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{//very important: the stdin and stdout must be open , or the printf will not work
	printl("TestA:>>>\n");
	char tty_name[] = "/dev_tty0";
	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);
	
	char *filenames[]={
		"/foo", "/bar", "/test"
	};
	
	int i;
	int nr=sizeof(filenames)/sizeof(filenames[0]);


	printf("Created file :");
	for(i=0;i<nr;i++){
		int fd=open(filenames[i], O_CREAT | O_RDWR);
		assert(fd!=-1);
		printf("%s ",filenames[i]);
		assert(close(fd)==0);
	}
	printf("\n");

	char *rmfilenames[]={
		"/bar", "/test", "/foo"  
	};

	nr=sizeof(rmfilenames)/sizeof(rmfilenames[0]);

	printf("file removed:");
	for(i=0;i<nr;i++){
		if(unlink(rmfilenames[i]) == 0){
			printf("%s ", rmfilenames[i]);
		}
		else{
			printf("Faild removing file \"%s\"", rmfilenames[i]);
		}
	}
	printf("\n");
	spin("Test A");
	while(0){
#ifdef DEBUG
		printf("testA\n");
#endif
		int fd=open("/test",O_CREAT);
		printf("fd: %d\n",fd);
		close(fd);
		spin("TestA");
	}
}


/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	printl("TestB:>>>\n");
	char tty_name[] = "/dev_tty1";
	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);
	char rdbuf[128];

	printf("In TestB:>>>\n");
	while(1){
		printf("$:");
		int r = read(fd_stdin, rdbuf,70);
		rdbuf[r] = 0;
		if(strcmp(rdbuf, "hello") == 0){
			printf("hello world\n");

		}
		else{
			if(rdbuf[0]){
				printf("{%s}\n", rdbuf);
			}
		}
	}
	assert(0);//never arrive here
}


/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	spin("Test C. . . \n");
	while(1){
		printf("C");
		milli_delay(10);
	}
}

int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type=GET_TICKS;
	send_recv(BOTH,TASK_SYS,&msg);
	return msg.RETVAL;
}

void Init()
{
	printl("Init running>>>");
	char tty_name[] = "/dev_tty0";
	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);
	untar("/cmd.tar");

	int pid = fork();
	if(pid!=0){//parent
		printl("In parent process: my child process is %d\n", pid);
		int s=-2;
		int child = wait(&s);//patent coming into state of waiting, if its child is not exit
		printl("child %d exit with status status:%d\n", child, s);
	}
	else if(pid==0){
		execl("/echo", "hello", "world", 0);
		
	}

	while(1){//child will no come here for it invoked exit
		int s;
		int child = wait(&s);
		printf("child %d exit with status status:%d\n", child, s);
	}

}

/**************************************************************
 * 						untar
 **************************************************************
	@function:extract the file in a tar file and store them
	@input:The path of the filename
	@return:void
**************************************************************/
void untar(const char* filename)
{
	printf("[extract------ '%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd!=-1);
	char buf[SECTOR_SIZE *16];
	int chunk = sizeof(buf);

	while(1){// a file per time
		read(fd,buf, SECTOR_SIZE);
		if(buf[0]==0){
			break;	
		}
		
		struct posix_tar_header *tarhead_p = (struct posix_tar_header *)buf;

		char *p=tarhead_p->size;
		int f_len = 0;

		while(*p){
			f_len = (f_len *8) + (*p++ - '0');/*octal*/
		}

		int bytes_left= f_len;
		int fd_out = open(tarhead_p->name, O_CREAT | O_RDWR);

		if(fd_out == -1){
			printf("failed to extract file: %s\n", tarhead_p->name);
			printf("aborted]\n");
			return;
		}
		printf("    %s (%d bytes)\n", tarhead_p->name, f_len);
		while(bytes_left){
			int iobytes = min(chunk, bytes_left);
			read(fd, buf, ((iobytes -1)/SECTOR_SIZE +1)* SECTOR_SIZE);
			write(fd_out, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fd_out);
	}
	close(fd);
	printf("done!]\n");
}
