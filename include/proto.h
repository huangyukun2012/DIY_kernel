#ifndef 	_PROTO_H
#define		_PROTO_H
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "proc.h"
#include "const.h"
#include "msg.h"

/* klib.asm */
PUBLIC void	out_byte(t_port port, t_8 value);
PUBLIC t_8	in_byte(t_port port);
extern void port_read(t_16 port, void* buf, int n);
extern void port_write(t_16 port, void* buf, int n);
PUBLIC void	disable_int();
PUBLIC void	enable_int();
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC t_32	seg2phys(t_16 seg);
PUBLIC void	disable_irq(int irq);
PUBLIC void	enable_irq(int irq);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
PUBLIC void restart();

/* main.c */
PUBLIC void TestA();
PUBLIC void TestB();
PUBLIC void TestC();

/* i8259.c */
PUBLIC void put_irq_handler(int iIRQ, t_pf_irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void milli_delay(int milli_sec);


/* keyboard.c */
PUBLIC void keyboard_handler(int irq);

/* tty.c */
PUBLIC void task_tty();
PUBLIC void in_process();
PUBLIC void syn_cursor();


/************************************************************************/
/*                        以下是系统调用相关                            */
/************************************************************************/


/* syscall.asm */
PUBLIC	void	sys_call();		/* t_pf_int_handler */
int printx(char *s);
#endif
