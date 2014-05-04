#include "msg.h"
#include "proc.h"
#include "string.h"
#include "err.h"
#include "stdio.h"

int unlink(const char *pathname);
int fork();
int execv(const char *path, char *argv[]);
int execl(const char* path, const char *arg, ...);

int open(const char *pathname, int flags)
{
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

int fork()
{
#if DEBUG
	printl("in fork>>>");
#endif
	MESSAGE msg;
	msg.type = FORK;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	assert(msg.RETVAL == 0);
	
	return msg.PID;
}

int getpid()
{
	MESSAGE msg;
	msg.type = GET_PID;

	send_recv(BOTH, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.PID;
}

int wait(int *re)
{
	MESSAGE msg;
	msg.type = WAIT;
	send_recv(BOTH, TASK_MM, &msg);
	*re = msg.STATUS;
	return (msg.PID == NO_TASK? -1:msg.PID);
}

void exit(int status)
{
	MESSAGE msg;
	msg.type = EXIT;
	msg.STATUS = status;
	send_recv(BOTH, TASK_MM , &msg);
	assert(msg.RETVAL == SYSCALL_RET);
}

/**************************************************************
 *               execl
 **************************************************************
	@function:change the params to exec
	@input:pathname of the elf file
			arg:
	@return:The result of exec
**************************************************************/
int hykexecl(const char* path, const char *arg)
{
	va_list parg = (va_list )(&arg);// 可变参数函数的一般写法
	char **p =(char **)parg;
	return execv(path, p);
}
int execl(const char *path, const char *arg, ...)
{
	va_list parg = (va_list)(&arg);
	char **p = (char**)parg;
	return execv(path, p);
}

/**************************************************************
 *               execv
 **************************************************************
	@function:Send msg to MM_task
	@input:
		1>path name of the elf
		2>the prarams of cmd
	@return:msg return
**************************************************************/

int hykexecv(const char *path, char *argv[])
{
	char **p = argv;
	char arg_stack[PROC_ORIGIN_STACK];
	int stacklen=0;

	while(*p++){
		assert(stacklen + 2*sizeof(char *) < PROC_ORIGIN_STACK);//The first papam and then end tag
		stacklen +=sizeof(char *);
	}

	*( (int *)(&arg_stack[stacklen]) )=0;
	stacklen +=sizeof(char *);
	
	char **q = (char **)arg_stack;
	for(p=argv; *p !=0;p++){
		*q++ = &arg_stack[stacklen];
		assert(stacklen + strlen(*p) + 1 < PROC_ORIGIN_STACK);
		strcpy(&arg_stack[stacklen], *p);
		printl("%s*",*p);
		stacklen +=strlen(*p);
		arg_stack[stacklen]=0;
		stacklen ++;
	}

	MESSAGE msg;
	msg.type = EXEC;
	msg.PATHNAME = (void *)path;
	msg.NAME_LEN = strlen(path);
	msg.BUF = (void *)arg_stack;
	msg.BUF_LEN = stacklen;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;

}

int execv(const char *path, char * argv[])
{
	char **p = argv;
	char arg_stack[PROC_ORIGIN_STACK];
	int stack_len = 0;

	while(*p++) {
		assert(stack_len + 2 * sizeof(char*) < PROC_ORIGIN_STACK);
		stack_len += sizeof(char*);
	}

	*((int*)(&arg_stack[stack_len])) = 0;
	stack_len += sizeof(char*);

	char ** q = (char**)arg_stack;
	for (p = argv; *p != 0; p++) {
		*q++ = &arg_stack[stack_len];

		assert(stack_len + strlen(*p) + 1 < PROC_ORIGIN_STACK);
		strcpy(&arg_stack[stack_len], *p);
		stack_len += strlen(*p);
		arg_stack[stack_len] = 0;
		stack_len++;
	}

	MESSAGE msg;
	msg.type	= EXEC;
	msg.PATHNAME	= (void*)path;
	msg.NAME_LEN	= strlen(path);
	msg.BUF		= (void*)arg_stack;
	msg.BUF_LEN	= stack_len;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}
/**************************************************************
 *               stat
 **************************************************************
	@function:get the stat of pathname
	@input:pathname, statp
	@return:0 if sucess
**************************************************************/
int stat(const char *pathname, struct stat *statp)
{
	MESSAGE msg;
	msg.type = STAT;
	msg.BUF = (void *)statp;
	msg.NAME_LEN = strlen(pathname);
	msg.PATHNAME = (void *)pathname;

	send_recv(BOTH,TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return 0;
}
