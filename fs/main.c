#include "msg.h"
#include "proc.h"
#include "err.h"
#include "nostdio.h"
void task_fs()
{
	printl("Task FS Begin.\n");
	MESSAGE driver_msg;
	driver_msg.type=DEV_OPEN;
	send_recv(BOTH,TASK_HD,&driver_msg);
	spin("FS");
}
