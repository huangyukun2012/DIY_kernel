#include "msg.h"
#include "proc.h"
#include "string.h"
#include "err.h"

int unlink(const char *pathname);

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


int  close(int fd)
{
	MESSAGE msg;

	msg.type=CLOSE;
	msg.FD = fd;
	send_recv(BOTH, TASK_FS, &msg);
	
	return msg.RETVAL;
}



///this isread.hor user proc
int read(int fd, void *buf, int count)
{
	MESSAGE msg;
	msg.type = READ;
	msg.FD = fd;
	msg.BUF = buf;
	msg.CNT = count;
	send_recv(BOTH, TASK_FS, &msg);
	return msg.CNT;

}

int write(int fd, const void *buf, int count)
{
	MESSAGE msg;
	msg.type = WRITE;
	msg.FD = fd;
	msg.BUF = (void *)buf;
	msg.CNT = count;

	send_recv(BOTH, TASK_FS, &msg);
	return msg.CNT;
}

int unlink(const char *pathname)
{
	MESSAGE msg;
	msg.type = UNLINK;
	msg.PATHNAME = (void *)pathname;
	msg.NAME_LEN = strlen(pathname);

	send_recv(BOTH, TASK_FS, &msg);
	return msg.RETVAL;

}
