
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

#include "tty.h"
#include "console.h"
#include "proc.h"
#include "protect.h"
#include "proto.h"
extern	struct proc	proc_table[];
extern	struct s_task		task_table[];
extern	struct s_task		user_proc_table[];
extern	char		task_stack[];

extern 	TTY			tty_table[];
extern	CONSOLE		console_table[];
extern	t_pf_irq_handler	irq_table[];

extern	t_sys_call		sys_call_table[];

extern 	int		ticks;
extern	struct proc *p_proc_ready;

EXTERN	int		disp_pos;
EXTERN	t_8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	t_8		idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	t_32		k_reenter;

EXTERN	TSS		tss;

#endif
