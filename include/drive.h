
#ifndef __DRIVE_H__
#define __DRIVE_H__
#include "proc.h"
#define NO_DEV	0
#define DEV_FLOPPY	1
#define DEV_CDROM	2
#define DEV_HD	3
#define DEV_CHAR_TTY	4
#define DEV_SCSI	5

#define MAJOR_SHIFT	8
#define MAKE_DEV(a, b)	((a << MAJOR_SHIFT ) | b)
#define MAJOR(x)	((x>>MAJOR_SHIFT) & 0xFF)
#define MINOR(x)	(x & 0xFF)
struct dev_drv_map{
	int driver_nr;
};

extern struct dev_drv_map dd_map[];
#endif
