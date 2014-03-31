#ifndef CONSOLE_H
#define	CONSOLE_H

#define NR_CONSOLE	3
typedef struct s_console{
	unsigned int current_start_addr;
	unsigned int original_adr;
	unsigned int vmem_limit;
	unsigned int cursor_location;
}CONSOLE;

void out_char(CONSOLE *console_p,char ch);
#endif
