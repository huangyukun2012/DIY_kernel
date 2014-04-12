# 1 "fs/main.c"
# 1 "<built-in>"
# 1 "<命令行>"
# 1 "fs/main.c"
# 1 "include/msg.h" 1


# 1 "include/type.h" 1
# 12 "include/type.h"
typedef unsigned long long t_64;
typedef unsigned int t_32;
typedef unsigned short t_16;
typedef unsigned char t_8;
typedef int t_bool;

typedef unsigned int t_port;

typedef void (*t_pf_int_handler) ();
typedef void (*t_pf_task) ();
typedef void (*t_pf_irq_handler) (int irq);

typedef void* t_sys_call;
typedef char* va_list;
# 4 "include/msg.h" 2
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
 t_64 m3l1;
 t_64 m3l2;
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




enum msgtype{
 HARD_INT=1,
 GET_TICKS,
 DEV_OPEN=1001,
 DEV_CLOSE,
 DEV_READ,
 DEV_WRITE,
 DEV_IOCTL
};
# 2 "fs/main.c" 2
# 1 "include/proc.h" 1
# 9 "include/proc.h"
# 1 "include/protect.h" 1
# 13 "include/protect.h"
typedef struct descriptor
{
 t_16 limit_low;
 t_16 base_low;
 t_8 base_mid;
 t_8 attr1;
 t_8 limit_high_attr2;
 t_8 base_high;
}DESCRIPTOR;


typedef struct s_gate
{
 t_16 offset_low;
 t_16 selector;
 t_8 dcount;


 t_8 attr;
 t_16 offset_high;
}GATE;

typedef struct s_tss {
 t_32 backlink;
 t_32 esp0;
 t_32 ss0;
 t_32 esp1;
 t_32 ss1;
 t_32 esp2;
 t_32 ss2;
 t_32 cr3;
 t_32 eip;
 t_32 flags;
 t_32 eax;
 t_32 ecx;
 t_32 edx;
 t_32 ebx;
 t_32 esp;
 t_32 ebp;
 t_32 esi;
 t_32 edi;
 t_32 es;
 t_32 cs;
 t_32 ss;
 t_32 ds;
 t_32 fs;
 t_32 gs;
 t_32 ldt;
 t_16 trap;
 t_16 iobase;

}TSS;
# 10 "include/proc.h" 2
# 1 "include/msg.h" 1
# 11 "include/proc.h" 2

typedef struct s_stackframe {
 t_32 gs;
 t_32 fs;
 t_32 es;
 t_32 ds;
 t_32 edi;
 t_32 esi;
 t_32 ebp;
 t_32 kernel_esp;
 t_32 ebx;
 t_32 edx;
 t_32 ecx;
 t_32 eax;
 t_32 retaddr;
 t_32 eip;
 t_32 cs;
 t_32 eflags;
 t_32 esp;
 t_32 ss;
}STACK_FRAME;

typedef struct s_task {
 t_pf_task initial_eip;
 int stacksize;
 char name[32];
}TASK;

typedef struct proc{
 STACK_FRAME regs;

 t_16 ldt_sel;
 DESCRIPTOR ldts[2];

 int ticks;
 int priority;
 t_32 pid;
 char name[16];
 int nr_tty;

 int p_flags;
 MESSAGE *p_msg;
 int p_recvfrom;
 int p_sendto;
 int has_int_msg;
 struct proc *q_sending;
 struct proc *next_sending;
}PROCESS;




void * va2la(int pid, void * va);
int ldt_reg_linear(PROCESS *p,int index);

int sys_printx(int _unused1, int _unused2, char * s, struct proc *p);
int sys_sendrec(int function , int sec_dest , MESSAGE *m, struct proc * p);
int send_recv(int function, int src_dest, MESSAGE *msg);

void schedule();
void dump_msg(const char *,MESSAGE *);
# 3 "fs/main.c" 2
# 1 "include/err.h" 1







void assertion_failure(char *exp, char *myfile, char * base_file, int file);
# 20 "include/err.h"
void assertion_failure(char *exp, char *myfile, char * base_file, int line);
void spin(char *funcname);
void panic(const char *fmt,...);
# 4 "fs/main.c" 2
# 1 "include/nostdio.h" 1


extern int printf(const char *fmt,...);

extern int vsprintf(char *buf,const char *fmt,va_list args);
# 5 "fs/main.c" 2
# 1 "include/hd.h" 1
# 22 "include/hd.h"
struct part_ent {
 t_8 boot_ind;
# 33 "include/hd.h"
 t_8 start_head;



 t_8 start_sector;






 t_8 start_cyl;







 t_8 sys_id;







 t_8 end_head;



 t_8 end_sector;






 t_8 end_cyl;







 t_32 start_sect;




 t_32 nr_sects;



} PARTITION_ENTRY;
# 219 "include/hd.h"
struct hd_cmd {
 t_8 features;
 t_8 sector_count;
 t_8 lba_low;
 t_8 lba_mid;
 t_8 lba_high;
 t_8 device;
 t_8 command;
};

struct part_info {
 t_32 base;
 t_32 size;
};
# 244 "include/hd.h"
struct hd_info
{






 int open_cnt;
 struct part_info primary[(4 + 1)];
 struct part_info logical[(16 * 4)];
};
# 279 "include/hd.h"
void task_hd();
# 6 "fs/main.c" 2
# 1 "include/drive.h" 1



# 1 "include/proc.h" 1
# 5 "include/drive.h" 2
# 16 "include/drive.h"
struct dev_drv_map{
 int drive_nr;
};

struct dev_drv_map dd_map[]={
 {-20 },
 {-20 },

 {2},
 {0},
 {-20 },
 {-20 },
};
# 7 "fs/main.c" 2


void task_fs()
{
 printf("Task FS Begin.\n");
 MESSAGE driver_msg;
 driver_msg.type=DEV_OPEN;
 driver_msg.u.m3.m3i4 = (((3 << 8 ) | MINOR_BOOT) & 0xFF);
 if(dd_map[((((3 << 8 ) | MINOR_BOOT)>>8) & 0xFF)].driver_nr != -20); else assertion_failure("dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER","fs/main.c", "fs/main.c", 15);
 send_recv(3,dd_map[((((3 << 8 ) | MINOR_BOOT)>>8) & 0xFF)].driver_nr,&driver_msg);
 spin("FS");
}
