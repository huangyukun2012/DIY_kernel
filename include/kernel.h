#ifndef __KERNEL_H__
#define __KERNEL_H__ 

struct boot_params {
	    int     mem_size;   /* memory size */
		unsigned char * kernel_file;    /* addr of kernel file */
};

int get_kernel_map(unsigned int *b, unsigned int *l);
void get_boot_params(struct boot_params *bootp);

#endif
