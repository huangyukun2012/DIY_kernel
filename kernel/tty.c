
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
/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	init_keyboard();
	TTY *tty_p;

	for (tty_p  = tty_table ; tty_p  < tty_table+NR_TTY; ++tty_p ){
		init_tty(tty_p);
	}
	current_tty=tty_table;

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

}
void tty_read(TTY *tty_p)
{
	if(tty_p==current_tty){
		keyboard_read(tty_p);
	}
}

//------------------------------------------------------------------------
//                           tty_write()
void tty_write(TTY *tty_p)
{
	if(tty_p->count >0 ){
		char ch=*(tty_p->tail);
		tty_p->tail++;
		if(tty_p->tail==tty_p->buffer+TTY_BUFFER_SIZE){
			tty_p->tail=tty_p->buffer;
		}
		tty_p->count--;
		out_char(tty_p->console_p,ch);
	}
}


PUBLIC void in_process(TTY * tty_p,t_32 key){
	if(!(key&FLAG_EXT)){
		if(tty_p->count < TTY_BUFFER_SIZE){
			*(tty_p->head)=key;
			tty_p->head++;
			if(tty_p->head == tty_p->buffer+TTY_BUFFER_SIZE){
				tty_p->head=tty_p->buffer;
			}
			tty_p->count++;
		}
	}
	else{
		int raw_code=key&MASK_RAW;
		switch(raw_code){
			case UP:
				if((key&FLAG_SHIFT_L)|| (key & FLAG_SHIFT_R)){
					page_switch();
				}
				break;
			case DOWN:
				if((key&FLAG_SHIFT_L)|| (key & FLAG_SHIFT_R)){
					//page_switch();
				}
				break;
			default:
				break;

		}
	}
}
PUBLIC void rename_in_process(TTY * tty_p,t_32 key){
	char output[2]={0,0};
	if(!(key&FLAG_EXT)){
		output[0]=key&0xff;
		disp_str(output);
		
		syn_cursor();
	}
	else{
		int raw_code=key&MASK_RAW;
		switch(raw_code){
			case UP:
				if((key&FLAG_SHIFT_L)|| (key & FLAG_SHIFT_R)){
					page_switch();
				}
				break;
			case DOWN:
				if((key&FLAG_SHIFT_L)|| (key & FLAG_SHIFT_R)){
					//page_switch();
				}
				break;
			default:
				break;

		}
	}
}
PUBLIC void syn_cursor(){
	disable_int();
	
	out_byte(CRTC_ADDR_REG,CURSOR_START_REG);
	out_byte(CRTC_DATA_REG,0x00);
	out_byte(CRTC_ADDR_REG,CURSOR_END_REG);
	out_byte(CRTC_DATA_REG,0x61);

	out_byte(CRTC_ADDR_REG,CURSOR_H);
	out_byte(CRTC_DATA_REG,(((disp_pos/2)>>8))&0xFF);
	out_byte(CRTC_ADDR_REG,CURSOR_L);
	out_byte(CRTC_DATA_REG,((disp_pos/2))&0xFF);

	enable_int();
	}
PRIVATE void page_switch()
{
	disable_int();
	
	out_byte(CRTC_ADDR_REG,START_ADDR_H);
	out_byte(CRTC_DATA_REG,((80*15)>>8)&0xFF);
	out_byte(CRTC_ADDR_REG,CURSOR_L);
	out_byte(CRTC_DATA_REG,(80*15)&0xFF);

	enable_int();
}
