#include "msg.h"
#include "proc.h"
#include "string.h"
#include "err.h"


int open(const char *pathname, int flags)
{
#ifdef DEBUG 
	    printf("in open: ");
#endif
			
	MESSAGE msg;
	msg.type = OPEN;
	msg.PATHNAME = (void * )pathname;
	msg.FLAGS = flags;
	msg.NAME_LEN = strlen(pathname);

	send_recv(BOTH, TASK_FS,&msg);
	assert(msg.type == SYSCALL_RET);
	return msg.FD;
}
