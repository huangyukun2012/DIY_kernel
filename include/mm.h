#ifndef __MM_H__
#define __MM_H__
#include "msg.h"

extern MESSAGE mm_msg;
void task_mm();
int alloc_mem(int pid, int memsize);
void init_mm();
int do_fork();

extern int memory_size;
#endif
