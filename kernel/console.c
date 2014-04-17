#include "type.h"
#include "const.h"
#include "proto.h"
#include "console.h"
#include "global.h"

#define COLOR	0x0C

void out_char(CONSOLE * p_con,char ch);
void init_console(CONSOLE *console_p);
void switch_console(CONSOLE *console_p);
void scroll_screen(CONSOLE *console_p, int direction);
void set_console_start_addr(t_32 addr);
void set_cursor(t_32 addr);
void set_cursor_location(t_32 addr);
void set_cursor_length();

void out_char(CONSOLE * p_con,char ch)
{
	disp_pos=0;
	t_8	*vmem_p=(t_8 *)(V_MEM_BASE + p_con->cursor_location*2);
	switch(ch){
		case '\n':
			if(p_con->cursor_location + SCREEN_WIDETH < p_con->begin_adr+p_con->vmem_limit ){
				int line=(p_con->cursor_location - p_con->begin_adr)/SCREEN_WIDETH+1;
				p_con->cursor_location=line*SCREEN_WIDETH + p_con->begin_adr;
			}
			break;
		case '\b':
			if(p_con->cursor_location > p_con->begin_adr){
				p_con->cursor_location--;
				*(vmem_p-2)=' ';
				*(vmem_p-1)=COLOR;
			}
			break;
		default:
			if(p_con->cursor_location < p_con->begin_adr+p_con->vmem_limit){
				*vmem_p = ch;
				vmem_p++;
				*vmem_p = COLOR;
				vmem_p++;
				p_con->cursor_location++;
			}
			else{
				p_con->cursor_location=p_con->begin_adr;
				vmem_p=(t_8 *)(V_MEM_BASE + p_con->cursor_location*2);
				*vmem_p = ch;
				vmem_p++;
				*vmem_p = COLOR;
				vmem_p++;
				
			}
	}
	while(p_con->cursor_location >= p_con->current_start_addr + SCREEN_SIZE){
		scroll_screen(p_con,SCROLL_DOWN);
	}
	set_cursor(p_con->cursor_location);//display position

}

void init_console(CONSOLE *console_p)
{

	int video_in_double_bytes=V_MEM_SIZE/2;
	int nr_console=console_p - console_table;

	console_p->vmem_limit = video_in_double_bytes/NR_CONSOLE;
	console_p->begin_adr = console_p->vmem_limit *  nr_console;
	console_p->current_start_addr = console_p->begin_adr;
	console_p->cursor_location = console_p->begin_adr;
	if(nr_console==0){
		console_p->cursor_location=disp_pos/2;
	}
	else{
		out_char(console_p, nr_console+'0');
		out_char(console_p, '#');

	}
	set_console_start_addr(console_p->current_start_addr);
	set_cursor(console_p->cursor_location);
}

//------------------------------------------------------
void switch_console(CONSOLE *console_p)
{
	if(console_p<console_table || console_p>=console_table+NR_CONSOLE)
		return ;
	current_tty=tty_table+(console_p - console_table );
	set_cursor(console_p->cursor_location);
	set_console_start_addr(console_p->current_start_addr );
}

//-----------------------------------------------------------
void scroll_screen(CONSOLE *console_p, int direction)
{
	if(direction == SCROLL_UP ){
		if(console_p->current_start_addr - SCREEN_WIDETH >= console_p->begin_adr){
			console_p->current_start_addr -= SCREEN_WIDETH;
		}
	}
	else if(direction == SCROLL_DOWN){
		if(console_p->current_start_addr + SCREEN_WIDETH <= console_p->begin_adr + console_p->vmem_limit){
			console_p->current_start_addr +=SCREEN_WIDETH;
		}
	}
	else
	{

	}
	set_console_start_addr(console_p->current_start_addr);
	set_cursor(console_p->cursor_location );
}

void set_console_start_addr(t_32 addr)
{
	
	disable_int();
	out_byte(CRTC_ADDR_REG,START_ADDR_H );
	out_byte(CRTC_DATA_REG,(addr>>8)&0xFF);
	out_byte(CRTC_ADDR_REG,START_ADDR_L);
	out_byte(CRTC_DATA_REG,(addr)&0xFF);
	enable_int();
}

void set_cursor(t_32 addr)
{
	set_cursor_location(addr);
	set_cursor_length();
}
void set_cursor_location(t_32 addr)
{//location of cursor
	disable_int();
	
	out_byte(CRTC_ADDR_REG,CURSOR_H );
	out_byte(CRTC_DATA_REG,(addr>>8)&0xFF);
	out_byte(CRTC_ADDR_REG,CURSOR_L);
	out_byte(CRTC_DATA_REG,(addr)&0xFF);

	enable_int();
}

void set_cursor_length()
{
	disable_int();
	out_byte(CRTC_ADDR_REG,CURSOR_START_REG);
	out_byte(CRTC_DATA_REG,0x00);
	out_byte(CRTC_ADDR_REG,CURSOR_END_REG);
	out_byte(CRTC_DATA_REG,0x61);
	enable_int();
}
