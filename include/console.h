#ifndef CONSOLE_H
#define	CONSOLE_H

#define NR_CONSOLE	3
#define SCREEN_WITTH	80*25
#define SCROLL_UP	1
#define	SCROLL_DOWN	-1

typedef struct s_console{
	//all are counted in double bytes!
	unsigned int begin_adr;
	unsigned int current_start_addr;
	unsigned int cursor_location;
	unsigned int vmem_limit;
}CONSOLE;

void out_char(CONSOLE *console_p,char ch);
void init_console(CONSOLE *console_p);
void switch_console(CONSOLE *console_p);
void scroll_screen(CONSOLE *console_p, int direction);
void set_console_start_addr(t_32 addr);
void set_cursor(t_32 addr);
void set_cursor_location(t_32 addr);
void set_cursor_length();

#endif
