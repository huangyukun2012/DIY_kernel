
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


PRIVATE void page_switch();
PUBLIC void task_tty();
PUBLIC void in_process(t_32 key);
PUBLIC void syn_mouse();

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	while (1) {/* forever. yes, forever, there's something which is some kind of forever... */
		keyboard_read();
	}
}

PUBLIC void in_process(t_32 key){
	char output[2]={0,0};
	if(!(key&FLAG_EXT)){
		output[0]=key&0xff;
		disp_str(output);
		
		syn_mouse();
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
PUBLIC void syn_mouse(){
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
