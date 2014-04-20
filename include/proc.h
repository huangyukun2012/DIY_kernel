#ifndef _PROC_H_
#define _PROC_H_
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "protect.h"
#include "msg.h"
#include "type.h"
#include "fs.h"
typedef struct s_stackframe {	/* proc_ptr points here				↑ Low			*/
	t_32	gs;		/* ┓						│			*/
	t_32	fs;		/* ┃						│			*/
	t_32	es;		/* ┃						│			*/
	t_32	ds;		/* ┃						│			*/
	t_32	edi;		/* ┃						│			*/
	t_32	esi;		/* ┣ pushed by save()				│			*/
	t_32	ebp;		/* ┃						│			*/
	t_32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	t_32	ebx;		/* ┃						↑栈从高地址往低地址增长*/		
	t_32	edx;		/* ┃						│			*/
	t_32	ecx;		/* ┃						│			*/
	t_32	eax;		/* ┛						│			*/
	t_32	retaddr;	/* return address for assembly code save()	│			*/
	t_32	eip;		/*  ┓						│			*/
	t_32	cs;		/*  ┃						│			*/
	t_32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	t_32	esp;		/*  ┃						│			*/
	t_32	ss;		/*  ┛						┷High			*/
}STACK_FRAME;

typedef struct s_task {
	t_pf_task	initial_eip;
	int		stacksize;
	char		name[32];
}TASK;

typedef struct proc{
	STACK_FRAME			regs;			/* process' registers saved in stack frame */

	t_16				ldt_sel;		/* selector in gdt giving ldt base and limit*/
	DESCRIPTOR			ldts[LDT_SIZE];		/* local descriptors for code and data */
								/* 2 is LDT_SIZE - avoid include protect.h */
	int				ticks;			/* remained ticks */
	int				priority;
	t_32				pid;			/* process id passed in from MM */
	char				name[16];		/* name of the process */

	/*int 			nr_tty;*/
	int 			p_flags;
	MESSAGE			*p_msg;
	int				p_recvfrom;
	int 			p_sendto;
	int				has_int_msg;
	struct	proc	*q_sending;
	struct 	proc	*next_sending;

	//sth with file operation
	struct	file_desc * filp[NR_FILES];
}PROCESS;



#define proc2pid(x)	(x-proc_table)
void * va2la(int pid, void * va);
int ldt_reg_linear(PROCESS  *p,int index);

int	sys_printx(int _unused1, int _unused2, char * s, struct proc *p);	/* t_sys_call */
int sys_sendrec(int function , int sec_dest , MESSAGE *m, struct proc * p);
int send_recv(int function, int src_dest, MESSAGE *msg);

void schedule();
void dump_msg(const char *,MESSAGE *);

/* Number of tasks */
#define NR_TASKS	4
#define NR_USER_PROCS	3
#define NR_PROCS	(NR_USER_PROCS+NR_TASKS )

/*tasks */
#define INVALID_DRIVER -20
#define INTERRUPT -10
#define TASK_TTY 0
#define TASK_SYS 1
#define	TASK_HD 2
#define TASK_FS 3
#define ANY	(NR_PROCS+10)
#define NO_TASK (NR_PROCS + 20)

/* stacks of tasks */
#define STACK_SIZE_TTY		0x8000
#define STACK_SIZE_SYS		0x8000
#define STACK_SIZE_HD		0x8000
#define STACK_SIZE_FS		0x8000
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TESTB	0x8000
#define STACK_SIZE_TESTC	0x8000


#define STACK_SIZE_TOTAL	(STACK_SIZE_TTY + \
				STACK_SIZE_SYS + \
				STACK_SIZE_HD +\
				STACK_SIZE_FS + \
				STACK_SIZE_TESTA + \
				STACK_SIZE_TESTB + \
				STACK_SIZE_TESTC)



#define SENDING   0x02  /* set when proc trying to send */
#define RECEIVING 0x04  /* set when proc trying to recv */

void inform_int(int task_nr);
#endif
