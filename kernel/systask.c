#include "msg.h"
#include "proc.h"
#include "const.h"
#include "err.h"
#include "global.h"

void task_sys()
{
	MESSAGE msg;
	while(1){
		send_recv(RECEIVE,ANY,&msg);
		int src=msg.source;
		switch (msg.type){
			case GET_TICKS:
				msg.RETVAL=ticks;
				send_recv(SEND,src,&msg);
				break;
			case HARD_INT:
				printf("got hard\n");
				break;
			default:
				printf("%d ",msg.type);
				panic("unknown msg type");
				break;
		}
	}
}
