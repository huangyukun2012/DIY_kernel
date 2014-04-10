#ifndef __MSG__H__
#define __MSG__H__
#include "type.h"
struct mess1{
	int m1i1;
	int m1i2;
	int m1i3;
	int m1i4;
};
struct mess2{
	void *m2p1;
	void *m2p2;
	void *m2p3;
	void *m2p4;
};
struct mess3{
	int m3i1;
	int m3i2;
	int m3i3;
	int m3i4;
	t_64	m3l1;
	t_64	m3l2;
	void *m3p1;
	void *m3p2;
};
typedef struct mess{
	int source;
	int type;
	union{
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	}u;
}MESSAGE;

#define SEND 1
#define RECEIVE 2
#define BOTH 3
enum msgtype{
	HARD_INT=1,
	GET_TICKS,
	DEV_OPEN=1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};
#define RETVAL	u.m3.m3i1

#endif
