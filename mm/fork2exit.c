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
	if(i == NR_PROCS){
		printl("Err: The proc_table is too full to fork\n");
		return -1;
	}

	assert(i<NR_PROCS);

	//duplicate the process table
	int parent_pid = mm_msg.source;//
	t_16 child_ldt_sel = child_procp->ldt_sel;//p is the free slot, and for the child
	*child_procp=proc_table[parent_pid];// strcut 赋值
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
#if DEBUG 
	printl("fork:alloc MM 0x%x <- 0x%x (0x%x bytes)\n", child_base, caller_T_base, caller_T_size);
#endif
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


void do_exit(int status)
{
	int i;
	int pid= mm_msg.source;
	int parent_pid = proc_table[pid].p_parent;
	struct proc *p = &proc_table[pid];

	/* fs exit :see fs_exit() */
	MESSAGE msg2fs;
	msg2fs.type = EXIT;
	msg2fs.PID = pid;
	send_recv(BOTH, TASK_FS, &msg2fs);
	/* free memory */
	free_mem(pid);
	p->exit_status = status;

	/* clean up proc_table */
	if(proc_table[parent_pid].p_flags & WAITING ){
		proc_table[parent_pid].p_flags &= ~WAITING;
		cleanup(&proc_table[pid]);
	}
	else{//parent is not waiting
			proc_table[pid].p_flags |= HANGING;		
	}

	/* handle the its child proc */
	for(i=0;i<NR_PROCS;i++){
		if(proc_table[i].p_parent == pid){
			proc_table[i].p_parent = INIT;
			if(proc_table[INIT].p_flags == WAITING && proc_table[i].p_flags == HANGING){
				proc_table[i].p_flags &= ~HANGING;
				cleanup(&proc_table[i]);
			}
		}
	}
}

/* Do the last jod:send its parent a message to unblock it, and clean up its proc_table */
void cleanup(struct proc *procp)
{
	MESSAGE msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(procp);
	msg2parent.STATUS = procp->exit_status;
	send_recv(SEND, procp->p_parent, &msg2parent);
	procp->p_flags = FREE_SLOT; 
}

/*P invoke wait(), which will invoke do_wait(), then the MM will do the following routine:
	<1>iterate the proc_table,
		if proc A is the child of P, and A's state is HANGING
			-repley to P:cleanup() will send p a msg to unblock it
			-release A's proc_table entry
			-return (MM go on the main loop)
	<2>if no child of p is HANGING
		set P's WAITING bit
	<3>if P has no child at all
		-reply to P with err
	<4>return 
*/
void do_wait()
{
	int pid = mm_msg.source;
	int i;
	int children=0;
	struct proc *procp = proc_table;//procp is child
	for (i = 0; i < NR_PROCS; ++i){
		if(procp->p_parent == pid){
			children++;
			if(procp->p_flags & HANGING){
				cleanup(procp);
				return;
			}
		}
		procp++;
	}
	if(children){//将父进程设置成等待状态
		proc_table[pid].p_flags |= WAITING;
	}
	else{
		MESSAGE msg;
		msg.type = SYSCALL_RET;
		msg.PID = NO_TASK;
		send_recv(SEND, pid , &msg);
	}
}
