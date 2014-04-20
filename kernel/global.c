
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"
#include "tty.h"
#include "console.h"
#include "systask.h"
#include "hd.h"
#include "fs.h"
	PROCESS	proc_table[NR_PROCS];
	char	task_stack[STACK_SIZE_TOTAL];

	TASK	task_table[NR_TASKS] = {{task_tty, STACK_SIZE_TTY, "tty"},
										{task_sys, STACK_SIZE_SYS, "sys"},
										{task_hd,STACK_SIZE_HD,"hd"},
										{task_fs,STACK_SIZE_HD,"fs"}};

	TASK	user_proc_table[NR_USER_PROCS] = {
					{TestA, STACK_SIZE_TESTA, "TestA"},
					{TestB, STACK_SIZE_TESTB, "TestB"},
					{TestC, STACK_SIZE_TESTC, "TestC"}};
	t_pf_irq_handler	irq_table[NR_IRQ];
	t_sys_call		sys_call_table[NR_SYS_CALL] = {sys_printx,sys_sendrec};
TTY	tty_table[NR_TTY];
CONSOLE		console_table[NR_CONSOLE];
