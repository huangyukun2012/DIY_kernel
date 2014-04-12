#include "msg.h"
#include "proc.h"
#include "err.h"
#include "nostdio.h"
#include "hd.h"
#include "drive.h"
#include "config.h"


void task_fs()
{
	printl("Task FS Begin.\n");
	MESSAGE driver_msg;
	driver_msg.type=DEV_OPEN;
	driver_msg.DEVICE = MINOR( ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH,dd_map[MAJOR(ROOT_DEV)].driver_nr,&driver_msg);
	spin("FS");
}
