
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
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
#include "keyboard.h"
#include "tty.h"
#include "console.h"

PRIVATE void page_switch();
PUBLIC void task_tty();
PUBLIC void in_process(TTY* tty_p,t_32 key);
PUBLIC void syn_cursor();
void tty_read(TTY *tty_p);
void tty_write(TTY *tty_p);
void init_tty(TTY *tty_p);
extern KB_INPUT kb_in;
void put_key2tty(TTY *tty_p, t_32 key);
void write_tty(TTY * tty_p,char *buf,int len);
int sys_write(char *buf,int len,PROCESS *p_proc);
/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	init_keyboard();
	set_cursor_length();
	TTY *tty_p;

	for (tty_p  = tty_table ; tty_p  < tty_table+NR_TTY; ++tty_p ){
		init_tty(tty_p);
	}
	current_tty=tty_table;
	switch_console(current_tty->console_p);

	while (1) {/* forever. yes, forever, there's something which is some kind of forever... */
		for (tty_p  = tty_table; tty_p  < tty_table+NR_TTY; ++tty_p ){
			tty_read(tty_p);
			tty_write(tty_p);
		}
		
	}
}

void init_tty(TTY *tty_p)
{
	tty_p->count=0;
	tty_p->head=tty_p->buffer;
	tty_p->tail=tty_p->buffer;

	int nr=tty_p-tty_table;
	tty_p->console_p=console_table+nr;
	init_console(tty_p->console_p);

}
void tty_read(TTY *tty_p)
{
	if(tty_p==current_tty){
		while(kb_in.count >0){
			keyboard_read(tty_p);
		}
	}
}

//------------------------------------------------------------------------
//                           tty_write()
void tty_write(TTY *tty_p)
{
	char ch='a';
	while(tty_p->count >0 ){
		ch=*(tty_p->tail);
		tty_p->tail++;
		if(tty_p->tail==tty_p->buffer+TTY_BUFFER_SIZE){
			tty_p->tail=tty_p->buffer;
		}
		tty_p->count--;
		out_char(tty_p->console_p,ch);
	}
}

void put_key2tty(TTY *tty_p, t_32 key)
{
		if(tty_p->count < TTY_BUFFER_SIZE){
			*(tty_p->head)=key;
			tty_p->head++;
			if(tty_p->head == tty_p->buffer+TTY_BUFFER_SIZE){
				tty_p->head=tty_p->buffer;
			}
			tty_p->count++;
		}
	
}
PUBLIC void in_process(TTY * tty_p,t_32 key)
{//got key into the tty buffer
	TTY *temp_tty_p;
	if(!(key&FLAG_EXT)){//if can be printed, put it to console buffer
		put_key2tty(tty_p,key);
	}
	else{
		int raw_code=key&MASK_RAW;
		switch(raw_code){
			case UP:
				if((key&FLAG_SHIFT_L)|| (key & FLAG_SHIFT_R)){
					scroll_screen(current_tty->console_p,SCROLL_UP);
				}
				break;
			case DOWN:
				if((key&FLAG_SHIFT_L)|| (key & FLAG_SHIFT_R)){
					scroll_screen(current_tty->console_p,SCROLL_DOWN);
				}
				break;
			case ENTER:
				put_key2tty(tty_p,'\n');
				break;
			case BACKSPACE:
				put_key2tty(tty_p,'\b');
				break;
			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				if((key&FLAG_SHIFT_L) || (key&FLAG_SHIFT_R)){
					int nr=raw_code-F1;
					temp_tty_p = tty_table+nr;
					switch_console(temp_tty_p->console_p );
				}
				break;
			default:
				break;

		}
	}
}

PUBLIC void syn_cursor()
{
	set_cursor_length();	
	set_cursor_location(disp_pos/2);
}
PRIVATE void page_switch()
{
	set_console_start_addr(80*15);
}
void write_tty(TTY * tty_p,char *buf,int len)
{
	char *p=buf;
	int i=len;
	while(i){
		out_char(tty_p->console_p,*p);
		p++;
		i--;
	}
}
int sys_write(char *buf,int len,PROCESS *p_proc)
{
	write_tty(&tty_table[p_proc->nr_tty],buf,len);
	return 0;
}
