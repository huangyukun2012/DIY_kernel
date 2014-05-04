#ifndef __MM_H__
#define __MM_H__
#include "msg.h"
#include "proc.h"
#include "proto.h"

extern MESSAGE mm_msg;
void task_mm();
int alloc_mem(int pid, int memsize);
void init_mm();
int do_fork();
void do_wait();
void do_exit(int );
int do_exec();
int free_mem(int pid);
void cleanup(struct proc *procp);


extern char *  mmbuf;
extern const int   MMBUF_LEN;
extern int memory_size;
#endif
