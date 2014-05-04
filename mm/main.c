#include "msg.h"
#include "proc.h"
#include "kernel.h"
#include "mm.h"
#include "stdio.h"
#include "err.h"

int alloc_mem(int pid, int memsize);
void init_mm();

MESSAGE mm_msg;
int memory_size;
/**
 *  * 7MB~8MB: buffer for MM
 *   */
char *  mmbuf       = (char *)0x700000;                                                                                                     
const int   MMBUF_LEN  = 0x100000;

void task_mm()
{
	printl("Task_mm begin:>>>");
	init_mm();
	while(1){
		send_recv(RECEIVE, ANY, &mm_msg);
		int src= mm_msg.source;
		int reply= 1;
		
		switch(mm_msg.type){
			case FORK:
				mm_msg.RETVAL = do_fork();
				break;
			case EXEC:
				mm_msg.RETVAL = do_exec();
				break;
			case WAIT:
				do_wait();//watch out for reply 
				reply=0;
				break;
			case EXIT:
				do_exit(mm_msg.STATUS);
				reply=0;
				break;
			default:
				dump_msg("MM:unknown msg", &mm_msg);
				break;
		}
		if(reply){//msg has return value, then send a msg to src to unblock it 
			mm_msg.type = SYSCALL_RET;
			send_recv(SEND, src, &mm_msg);
		}
	}
}

void init_mm()
{
	struct boot_params bp;
	get_boot_params(&bp);
	memory_size = bp.mem_size;
	printl("MM memsize: %dMB\n", memory_size/(1024*1024));
}

int alloc_mem(int pid, int memsize)
{
	assert(pid >= NR_TASKS + NR_NATIVE_PROCS);
	if(memsize > PROC_IMAGE_SIZE_DEFAULT){
		panic("unsupported memory request:%d(should less than %d)", memsize, PROC_IMAGE_SIZE_DEFAULT);

	}
	int base = PRORC_BASE + (pid - NR_TASKS -NR_NATIVE_PROCS)*(PROC_IMAGE_SIZE_DEFAULT);
	
	if(base + memsize >=memory_size)
		panic("memory allocation failed:pid:%d", pid);

	return base;
}
/* we do not need to free any thing, for the mem is corresponding to pid */
int free_mem(int pid)
{
	return 0;
}
