
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "string.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"
#include "msg.h"
#include "err.h"
#include "nostdio.h"


int ldt_reg_linear(struct proc *p,int index);
void * va2la(int pid, void * va);
static int msg_send(struct proc *current, int dest, MESSAGE *m);
static int msg_receive(struct proc * current_pro,int src, MESSAGE *m);
int sys_sendrec(int function, int src_dest, MESSAGE *m, struct proc *p);
/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	struct proc *p;
	int		greatest_ticks = 0;
	
	p_proc_ready->ticks--;
	if(p_proc_ready->ticks>0 && p_proc_ready->p_flags==0){
		return;
	}

	while (1==1) {
		for (p=proc_table; p<proc_table+NR_PROCS; p++) {//find the bigest tick
			if (p->ticks <=0 || p->p_flags!=0) 
				continue;
			if(p->ticks > greatest_ticks){
				greatest_ticks=p->ticks ;
				p_proc_ready=p;
			}
		}

		if (greatest_ticks<=0) {//not found one 
			for (p=proc_table; p<proc_table+NR_PROCS; p++) {
				if(p->p_flags ==0)
					p->ticks = p->priority;
			}
		}
		else{
			return;
		}
	}
}



/*======================================================================*
 						sys_sendrec()
 * =====================================================================*/
int sys_sendrec(int function, int src_dest, MESSAGE *m, struct proc *p)
{
	assert(k_reenter==0);//make sure we are not in ring0
	assert((src_dest>=0 && src_dest< NR_PROCS) ||
			src_dest==ANY ||
			src_dest == INTERRUPT);

	int ret=0;
	int caller=proc2pid(p);
	MESSAGE *mla=(MESSAGE *)va2la(caller,m);
	mla->source=caller;

	assert(mla->source != src_dest);

	if(function==SEND){
		ret=msg_send(p,src_dest,m);
		if(ret!=0)
			return ret;
	}
	else if (function== RECEIVE){
		ret=msg_receive(p,src_dest,m);
		if(ret!=0){
			return ret;
		}
	}
	else {
		panic("{sys_sendrec} invalid function: %d (SEND:%d, RECEIVE:%d).",function,SEND, RECEIVE);
	}

	return 0;
}
///return the linear address of a ldt-segment for a proc
int ldt_reg_linear(struct proc *p,int index)
{
	struct descriptor *d=&p->ldts[index];
	return d->base_high<<24 | d->base_mid << 16 | d->base_low;
}

void * va2la(int pid, void * va)
{
	struct proc *p=&proc_table[pid];
	t_32 seg_base= ldt_reg_linear(p,INDEX_LDT_RW);
	t_32 la=seg_base+ (t_32)va;

	if(pid<NR_PROCS){
		assert(la==(t_32)va);
	}

	return (void *)la;
}

//======================================
void reset_msg(MESSAGE *p)
{
	memset(p,0,sizeof(MESSAGE));
}

//===============================================
void block(struct proc*p)
{
	assert(p->p_flags);
	schedule();
}
//===================================================
void unblock(struct proc *p)
{
	assert(p->p_flags==0);
}

//==================================================
static deadlock(int src,int dest)
{//A----B----C----D---A
	struct proc *p=proc_table+dest;
	while(1){
		if(p->p_flags & SENDING){
			if(p->p_sendto == src){
				/*print the chain*/
				p=proc_table+dest;
				printl("=_=%s",p->name);
				do{
					assert(p->p_msg);
					p=proc_table+p->p_sendto;
					printl("->%s",p->name);
				}while(p!=proc_table+src);
				printl("=_=");
				return 1;
			}
			p=proc_table+p->p_sendto;
		}
		else{
			break;
		}
	}
	return 0;
}
//====================================================
static int msg_send(struct proc *current, int dest, MESSAGE *m)
{
	struct proc *sender=current;
	struct proc *p_dest=proc_table+dest;
#if DEBUG
	printl("%ds%d ",current-proc_table,dest);
#endif
	assert(proc2pid(sender)!=dest);
	if(deadlock(proc2pid(sender),dest)){
		panic(">>DEADLOCK<< %s->%s",sender->name,p_dest->name);
	}
	
	if((p_dest->p_flags && RECEIVING) &&
		(p_dest->p_recvfrom== proc2pid(sender) || p_dest->p_recvfrom==ANY)){//dest is waiting for to receive
		assert(p_dest->p_msg);
		assert(m);

		phys_copy(va2la(dest,p_dest->p_msg), va2la(proc2pid(sender),m), sizeof(MESSAGE));
		p_dest->p_msg=0;
		p_dest->p_flags &= ~RECEIVING;
		p_dest->p_recvfrom=NO_TASK;
		unblock(p_dest);

		assert(p_dest->p_flags ==0);
		assert(p_dest->p_msg ==0);
		assert(p_dest->p_recvfrom== NO_TASK);
		assert(p_dest->p_sendto== NO_TASK);
		
		assert(sender->p_flags == 0);
		assert(sender->p_msg ==  0);
		assert(sender->p_recvfrom == NO_TASK );
		assert(sender->p_sendto == NO_TASK);
	}
	else{//dest is not waiting for the msg
		sender->p_flags |= SENDING;
		assert(sender->p_flags == SENDING);
		sender->p_sendto=dest;
		sender->p_msg=m;

		//append to the sending queue
		struct proc *p;
		if(p_dest->q_sending){
			p=p_dest->q_sending;
			while(p->next_sending)
				p=p->next_sending;
			p->next_sending=sender;
		}
		else{
			p_dest->q_sending=sender;
		}
		sender->next_sending=0;
		block(sender);

		assert(sender->p_flags == SENDING);
		assert(sender->p_msg !=0);
		assert(sender->p_recvfrom ==NO_TASK);
		assert(sender->p_sendto ==dest);
	}
	return 0;

}

static int msg_receive(struct proc * current_pro,int src, MESSAGE *m)
{
	struct proc * who_want_to_receive=current_pro;
	struct proc * p_from=0;
	struct proc * prev=0;
	int copy_enable=0;
	assert(proc2pid(current_pro)!=src);
	if((who_want_to_receive->has_int_msg)&& 
			((src==ANY) || (src==INTERRUPT))){
		MESSAGE msg;
		reset_msg(&msg);
		msg.source=INTERRUPT;
		msg.type=HARD_INT;
		assert(m);
		phys_copy(va2la(proc2pid(who_want_to_receive), m), &msg, sizeof(MESSAGE));

		who_want_to_receive->has_int_msg=0;
		
		assert(who_want_to_receive->p_flags==0);
		assert(who_want_to_receive->p_msg == 0);
		assert(who_want_to_receive->p_sendto == NO_TASK);
		assert(who_want_to_receive->has_int_msg == 0);

		return 0;
	}
	
	if(src==ANY){//take the first one
		if(who_want_to_receive->q_sending){
			p_from=who_want_to_receive->q_sending;
			copy_enable=1;

			assert(who_want_to_receive->p_flags==0);
			assert(who_want_to_receive->p_msg==0);//no message
			assert(who_want_to_receive->p_recvfrom ==NO_TASK);
			assert(who_want_to_receive->p_sendto == NO_TASK);
			assert(who_want_to_receive->q_sending !=0);

			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg !=0);//has massage
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(who_want_to_receive));

		}
	}
	else//take a certain one
	{
		p_from=&proc_table[src];
		if((p_from->p_flags & SENDING) && 
				(p_from->p_sendto == proc2pid(who_want_to_receive))){
			copy_enable=1;

			struct proc * p=who_want_to_receive->q_sending;
			assert(p);

			while(p){
				assert(p_from->p_flags & SENDING);
				if(proc2pid(p)==src){
					p_from=p;
					break;
				}
				prev=p;
				p=p->next_sending;
			}

			assert(who_want_to_receive->p_flags ==0);
			assert(who_want_to_receive->p_msg == 0);
			assert(who_want_to_receive->p_recvfrom == NO_TASK);
			assert(who_want_to_receive->p_sendto ==NO_TASK);
			assert(who_want_to_receive->q_sending !=0);

			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg !=0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(who_want_to_receive));
		}
	}
	if(copy_enable){
		if(p_from == who_want_to_receive->q_sending){
			assert(prev==0);
			who_want_to_receive->q_sending = p_from->next_sending;
			p_from->next_sending=0;
		}
		else{//not the first
			assert(prev);
			prev->next_sending=p_from->next_sending;
			p_from->next_sending=0;
		}
		assert(m);
		assert(p_from->p_msg);
#if DEBUG
		printl("ub:%d-f%dr%dt%d  ",who_want_to_receive-proc_table,who_want_to_receive->p_flags,p_from-proc_table,p_from->p_msg->type );
#endif	
		//copy mesg
		phys_copy( va2la(proc2pid(who_want_to_receive),m),
				va2la(proc2pid(p_from),p_from->p_msg),
				sizeof(MESSAGE));
		p_from->p_msg=0;
		p_from->p_sendto=NO_TASK;
		p_from->p_flags &= ~SENDING;
		unblock(p_from);
		
	}
	else{//no one send to it
		who_want_to_receive->p_flags |=RECEIVING;
		who_want_to_receive->p_msg=m;
		if(src== ANY){
			who_want_to_receive->p_recvfrom=ANY;
		}
		else{
			who_want_to_receive->p_recvfrom=proc2pid(p_from);
		}
#if DEBUG 
		printl("b:%d-f%dr%d  ",who_want_to_receive-proc_table,who_want_to_receive->p_flags,src );
#endif
		block(who_want_to_receive);
//when come here, it is unblocked
			assert(who_want_to_receive->p_flags ==RECEIVING);
			assert(who_want_to_receive->p_msg != 0);
			assert(who_want_to_receive->p_recvfrom != NO_TASK);
			assert(who_want_to_receive->p_sendto ==NO_TASK);
			assert(who_want_to_receive->has_int_msg ==0);
		
	}

	return 0;
}
int send_recv(int function, int src_dest, MESSAGE *msg)
{
	int ret=0;
	if(function == RECEIVE)
		memset(msg,0,sizeof(MESSAGE));
	switch(function){
		case BOTH:
			ret=sendrec(SEND,src_dest,msg);
			if(ret == 0){
				ret=sendrec(RECEIVE,src_dest,msg);
			}
			break;
		case SEND:
		case RECEIVE:
			ret=sendrec(function, src_dest, msg);
			break;
		default:
			assert((function == BOTH) ||
					(function == SEND) || function == RECEIVE);
			break;

	}
	
	return ret;
}


PUBLIC void inform_int(int task_nr)
{
	struct proc* p = proc_table + task_nr;

	if ((p->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))) {
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING; /* dest has received the msg */
		p->p_recvfrom = NO_TASK;
		assert(p->p_flags == 0);
		unblock(p);

		assert(p->p_flags == 0);
		assert(p->p_msg == 0);
		assert(p->p_recvfrom == NO_TASK);
		assert(p->p_sendto == NO_TASK);
	}
	else {
		p->has_int_msg = 1;
	}
}

/*****************************************************************************
 *                                dump_msg
 *****************************************************************************/
PUBLIC void dump_msg(const char * title, MESSAGE* m)
{
	int packed = 0;
	printl("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)m,
	       packed ? "" : "\n        ",
	       proc_table[m->source].name,
	       m->source,
	       packed ? " " : "\n        ",
	       m->type,
	       packed ? " " : "\n        ",
	       m->u.m3.m3i1,
	       m->u.m3.m3i2,
	       m->u.m3.m3i3,
	       m->u.m3.m3i4,
	       (int)m->u.m3.m3p1,
	       (int)m->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}
