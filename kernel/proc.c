
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"


/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS*	p;
	int		greatest_priority = 0;
	
	p_proc_ready->ticks--;
	if(p_proc_ready->ticks>0)
		return;

	while (1==1) {
		for (p=proc_table; p<proc_table+NR_PROCS; p++) {
			if (p->ticks <=0 ) 
				continue;
			if(p->priority > greatest_priority){
				greatest_priority=p->priority;
				p_proc_ready=p;
			}
		}

		if (p_proc_ready->ticks<=0) {
			for (p=proc_table; p<proc_table+NR_PROCS; p++) {
				p->ticks = p->priority;
			}
		}
		else
			return;
	}
}


/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

