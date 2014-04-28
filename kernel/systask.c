#include "msg.h"
#include "proc.h"
#include "const.h"
#include "err.h"
#include "global.h"
#include "stdio.h"

void task_sys()
{
	MESSAGE msg;
	//disp_str("\ntask_sys begin>>>\n");
	printl("task_sys begin>>>\n");
	while(1){
		send_recv(RECEIVE,ANY,&msg);
		int src=msg.source;
		switch (msg.type){
			case GET_TICKS:
				msg.RETVAL=ticks;
				send_recv(SEND,src,&msg);
				break;
			case HARD_INT:
				printl("got hard\n");
				break;
			case GET_PID:
				msg.type= SYSCALL_RET;
				msg.PID = src;
				send_recv(SEND, src, &msg);
				break;
			default:
				printl("%d ",msg.type);
				panic("is unknown msg type in task_sys");
				break;
		}
	}
}
