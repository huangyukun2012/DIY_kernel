
#include "string.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"
#include "msg.h"
#include "err.h"
#include "stdio.h"
#include "debug.h"

int send_recv(int function, int src_dest, MESSAGE *msg);

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
