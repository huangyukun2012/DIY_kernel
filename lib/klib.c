
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            klib.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "config.h"
#include "kernel.h"
#include "err.h"
#include "stdio.h"

#include "elf.h"
char * itoa(char * str, int num);/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
void delay(int time);

/*======================================================================*
                               itoa
 *======================================================================*/
char * itoa(char * str, int num)/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
{
	char *	p = str;
	char	ch;
	int	i;
	t_bool	flag = FALSE;

	*p++ = '0';
	*p++ = 'x';

	if(num == 0){
		*p++ = '0';
	}
	else{	
		for(i=28;i>=0;i-=4){
			ch = (num >> i) & 0xF;
			if(flag || (ch > 0)){
				flag = TRUE;
				ch += '0';
				if(ch > '9'){
					ch += 7;
				}
				*p++ = ch;
			}
		}
	}

	*p = 0;

	return str;
}


/*======================================================================*
                               disp_int
 *======================================================================*/
void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}

/*======================================================================*
                               delay
 *======================================================================*/
void delay(int time)
{
	int i, j, k;
	for(k=0;k<time;k++){
		/*for(i=0;i<10000;i++){	for Virtual PC	*/
		for(i=0;i<10;i++){/*	for Bochs	*/
			for(j=0;j<10000;j++){}
		}
	}
}

void get_boot_params(struct boot_params *bootp)
{
	int *p = (int *)BOOT_PARAM_ADDR;
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	bootp->mem_size = p[BI_MEM_SIZE];
	bootp->kernel_file = (unsigned char *)(p[BI_KERNEL_FILE]);
	//the head of kernel file is "\177ELF"	/
	assert(memcmp(bootp->kernel_file, ELFMAG, SELFMAG) == 0);//elfmag '\177ELF';SELFMAG:4
}

int get_kernel_map(unsigned int *b, unsigned int *l)
{
	struct boot_params bp;
	get_boot_params(&bp);

	Elf32_Ehdr *elfheader = (Elf32_Ehdr*)(bp.kernel_file);

	//the krenel should be in elf format
	if(memcmp(elfheader->e_ident, ELFMAG, SELFMAG) !=0){
		return -1;
	}

	*b = ~0;
	unsigned int t = 0;
	int i;
	for (i = 0; i < elfheader->e_shnum; i++){
		Elf32_Shdr *section_header=(Elf32_Shdr *)(bp.kernel_file + elfheader->e_shoff + i*elfheader->e_shentsize);
		if(section_header->sh_flags & SHF_ALLOC){
			int bottom = section_header->sh_addr;
			int top= section_header->sh_addr + section_header->sh_size;
			if(*b>bottom)
				*b= bottom;
			if(t<top)
				t= top;
		}
		
	}
	assert(*b<t);
	*l= t-*b-1;
	return 0;
}
