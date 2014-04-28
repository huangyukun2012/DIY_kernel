#ifndef __CONFIG_H__
#define	__CONFIG_H__
#include "hd.h"

#define MINOR_BOOT MINOR_hd2a

#define BOOT_PARAM_ADDR         0x900  /* physical address */                                                                                    
#define BOOT_PARAM_MAGIC        0xB007 /* magic number */
#define BI_MAG              0
#define BI_MEM_SIZE         1
#define BI_KERNEL_FILE          2


#endif
