#include "proc.h"
#include "err.h"
#include "type.h"
#include "stdio.h"
#include "msg.h"
#include "mm.h"
#include "global.h"
#include "string.h"


/*perform the fork syscall.
 * return 0 if seccess, otherwise -1
 * */
int do_fork()
{
#if DEBUG
	printl("Do_fork beging:>>>");
#endif
	//find a free slot in proc_table
	struct proc *child_procp= proc_table;//child proc
	int i;
	for (i = 0; i < NR_PROCS; ++i, ++child_procp){
		if(child_procp->p_flags == FREE_SLOT){
			break;
		}
	}
	int child_pid = i;
	assert(child_procp == &proc_table[child_pid]);
	assert(child_pid >= NR_TASKS + NR_NATIVE_PROCS);
	if(i == NR_PROCS)
		return -1;
	assert(i<NR_PROCS);

	//duplicate the process table
	int parent_pid = mm_msg.source;//
	t_16 child_ldt_sel = child_procp->ldt_sel;//p is the free slot, and for the child
	*child_procp=proc_table[parent_pid];
	child_procp->ldt_sel = child_ldt_sel;
	child_procp->p_parent = parent_pid;
	sprintf(child_procp->name, "%s_%d", proc_table[parent_pid].name , child_pid);//son share the name with parent

	//duplicate the process:T, D, S
	struct descriptor *ppd;
	
	//Text segmaent

	ppd = &(proc_table[parent_pid].ldts[INDEX_LDT_C]);
	int caller_T_base = reassembly(ppd->base_high, 24,\
									ppd->base_mid, 16,\
									ppd->base_low);
	int caller_T_limit = reassembly(0,0, \
									(ppd->limit_high_attr2 & 0xF), 16, \
									ppd->limit_low);

	int caller_T_size = (( caller_T_limit + 1) * \
							((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ? 4096:1));
	
	//data and stack segments
	ppd = &(proc_table[parent_pid].ldts[INDEX_LDT_RW]);
	int caller_D2S_base = reassembly(ppd->base_high, 24,\
									ppd->base_mid, 16,\
									ppd->base_low);
	int caller_D2S_limit = reassembly(0,0, \
									(ppd->limit_high_attr2 & 0xF), 16,\
									ppd->limit_low);

	int caller_D2S_size = (( caller_D2S_limit + 1) * \
							((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ? 4096:1));
	
	assert(caller_T_base == caller_D2S_base);
	assert(caller_T_limit == caller_D2S_limit);
	assert(caller_T_size == caller_D2S_size);

	int child_base = alloc_mem(child_pid, caller_T_size);
	printl("alloc MM 0x%x <- 0x%x (0x%x bytes)\n", child_base, caller_T_base, caller_T_size);
	phys_copy((void *)child_base, (void *)caller_T_base, caller_T_size);

	//child's LDT
	init_descriptor(&(child_procp->ldts[INDEX_LDT_C]), child_base, (PROC_IMAGE_SIZE_DEFAULT -1 ) >> LIMIT_4K_SHIFT,\
					DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER <<5);
	init_descriptor(&(child_procp->ldts[INDEX_LDT_RW]), child_base, (PROC_IMAGE_SIZE_DEFAULT -1 ) >> LIMIT_4K_SHIFT,\
					DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER <<5);

	//tell FS, see fs_fork
	MESSAGE msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = child_pid;
	send_recv(BOTH, TASK_FS, &msg2fs);//the fs is waiting for the HD


	//child pid will be returned to the patent proc
	mm_msg.PID = child_pid;

	//birth of the child
	MESSAGE m;
	m.type = SYSCALL_RET;
	m.RETVAL = 0;
	m.PID = 0;
	send_recv(SEND, child_pid, &m);
	return 0;


}
