
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            keyboard.c
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
#include "keymap.h"
#include "tty.h"

KB_INPUT	kb_in;
PRIVATE int code_with_E0=0;
PRIVATE int shift_l;
PRIVATE int shift_r;
PRIVATE int alt_l;
PRIVATE int alt_r;
PRIVATE int ctrl_l;
PRIVATE int ctrl_r;
PRIVATE int caps_lock;
PRIVATE int num_lock;
PRIVATE int scroll_lock;
PRIVATE int column;
static int caps_lock;
static int num_lock;
static int scroll_lock;
static void kb_wait();
static void kb_ack();
static void set_leds();
PRIVATE t_8 get_byte_from_kbuf();


int key_pressed = 0;
/*======================================================================*
                            keyboard_handler
 *======================================================================*/
PUBLIC void keyboard_handler(int irq)
{//read scan_code to buffer
	t_8 scan_code=in_byte(KB_DATA);
	if(kb_in.count<KB_IN_BYTES){
		*(kb_in.p_head)=scan_code;
		kb_in.p_head++;
		if(kb_in.p_head==kb_in.buf+KB_IN_BYTES){
			kb_in.p_head=kb_in.buf;
		}
		kb_in.count++;
	}
	key_pressed = 1;
}


/*======================================================================*
                           init_keyboard
*======================================================================*/
PUBLIC void init_keyboard()
{
	kb_in.count=0;
	kb_in.p_head=kb_in.p_tail=kb_in.buf;

	shift_r=shift_l=0;
	alt_r=alt_l=0;
	ctrl_r=ctrl_l=0;

	caps_lock=1;
	num_lock=0;
	scroll_lock=0;

	set_leds();

	put_irq_handler(KEYBOARD_IRQ,keyboard_handler);
	enable_irq(KEYBOARD_IRQ);

}


/*======================================================================*
                           keyboard_read
*======================================================================*/
void keyboard_read(TTY * tty_p)
{//read all byte from keyboard buffer to tty buffer
	t_8 scan_code;
	t_bool  make;

	t_32 *keyrow;//point to the row
	t_32 key;//key is 32b, and scancode is 8b!

	if(kb_in.count>0){
		//we need some initial work here
		scan_code=get_byte_from_kbuf();
		
		if(scan_code==0xE1){//Pause key
			int i;
			t_8 pause_make_scancode[]={0xE1,0x1D,0x45,0xE1,0x9D,0xC5};
			int is_pause_make=1;
			
			for(i=1;i<6;i++){
				if (get_byte_from_kbuf()!=pause_make_scancode[i]){
					is_pause_make=0;
					break;//this scan code is leave out
				}	
			}
			if(is_pause_make){
				key=PAUSEBREAK;
			}
			
		}
		else if(scan_code==0xE0){//controll key
			scan_code=get_byte_from_kbuf();
			
			if(scan_code==0x2A){//print screen is pressed
				if(get_byte_from_kbuf()==0xE0){
					if(get_byte_from_kbuf()==0x37){//if here is not 0x37,then what happened?
						key=PRINTSCREEN;
						make=1;
					}
				}
			}
			else if(scan_code==0xB7){//printscreen is released
				if (get_byte_from_kbuf()==0xE0){
					if (get_byte_from_kbuf()==0xAA){
						key=PRINTSCREEN;
						make=0;
					}
				}
			}
			else {//the other control key 
				code_with_E0=1;
			}
		}

		if((key!=PAUSEBREAK)&&(key!=PRINTSCREEN)){//normal key with one byte

			make=((scan_code & FLAG_BREAK) ? FALSE:TRUE);
			
			keyrow=&keymap[(scan_code&0x7F)*MAP_COLS];
			column=0;
			
			int caps=shift_l || shift_r;

			if(caps_lock){
				if((keyrow[0]>='a') && (keyrow[0]<='z')){
					caps=!caps;
				}
			}
			if(caps)
				column=1;
			if(code_with_E0==1){
				code_with_E0=0;
				column=2;
			}

			key=keyrow[column];
			//not all the key can be printed

			switch(key){
				case SHIFT_L:
					shift_l=make;
					key=0;//because this shift key can not be pinted
					break;
				case SHIFT_R:
					shift_r=make;
					key=0;//
					break;
				case CTRL_L:
					ctrl_l=make;
					key=0;//because this 
					break;
				case CTRL_R:
					ctrl_r=make;
					key=0;//because this 
					break;
				case ALT_L:
					alt_l=make;
					key=0;//because this 
					break;
				case ALT_R:
					alt_r=make;
					key=0;//because this shift 
					break;
				case CAPS_LOCK:
					if(make){
						caps_lock=!caps_lock;
						set_leds();
					}
					break;
				case NUM_LOCK:
					if(make){
						num_lock=!num_lock;
						set_leds();
					}
					break;
				case SCROLL_LOCK:
					if(make){
						scroll_lock=!scroll_lock;
						set_leds();
					}
					break;
				default://normal and so on
					break;
								
			}

			if (make){
				int pad=0;

				if((key>= PAD_SLASH) && (key<=PAD_9) ){
					pad=1;
					switch(key){
						case PAD_SLASH:
							key='\\';
							break;
						case PAD_STAR:
							key='*';
							break;
						case PAD_MINUS:
							key='-';
							break;
						case PAD_PLUS:
							key='+';
							break;
						case PAD_ENTER:
							key=ENTER;
							break;
						default:
							if(num_lock && (key>=PAD_0) && (key<=PAD_9)){
								key=key-PAD_0+'0';
							}
							else if(num_lock && key==PAD_DOT){
								key='.';
							}
							else{
								switch(key){
									case PAD_HOME:
										key=HOME;
										break;
									case PAD_END:
										key=END;
										break;
									case PAD_PAGEUP:
										key=PAGEUP;
										break;
									case PAD_PAGEDOWN:
										key=PAGEDOWN;
										break;
									case PAD_INS:
										key=INSERT;
										break;
									case PAD_UP:
										key=UP;
										break;
									case PAD_DOWN:
										key=DOWN;
										break;
									case PAD_LEFT:
										key=LEFT;
										break;
									case PAD_RIGHT:
										key=RIGHT;
										break;
									case PAD_DOT:
										key=DELETE;
										break;
									default:
										break;
								}
							}
							break;

					}
				}
				key |=(shift_l ? FLAG_SHIFT_L:0);
				key |=(shift_r ? FLAG_SHIFT_R:0);
				key |=(ctrl_l ? FLAG_CTRL_L:0);
				key |=(ctrl_r ? FLAG_CTRL_R:0);
				key |=(alt_l ? FLAG_ALT_L:0);
				key |=(alt_r ? FLAG_ALT_R:0);
				in_process(tty_p,key);
			}
		}
	}

	
}

PRIVATE t_8 get_byte_from_kbuf()
{
	t_8 scan_code;
	while(kb_in.count<=0)
	{
		
	}
	disable_int();
	scan_code=*(kb_in.p_tail);
	kb_in.p_tail++;
	if(kb_in.p_tail==kb_in.buf+KB_IN_BYTES){
		kb_in.p_tail=kb_in.buf;
	}
	kb_in.count--;
	enable_int();
	return scan_code;

}

static void set_leds()
{
	t_8 leds=(caps_lock << 2) | (num_lock << 1) | scroll_lock;

	kb_wait();
	out_byte(PORT_8042_DATA,LED_CODE);
	kb_ack();

	kb_wait();
	out_byte(PORT_8042_DATA,leds);
	kb_ack();
}
static void kb_wait()
{
	t_8 kb_state;
	do{
		kb_state=in_byte(PORT_8042_CONTROL);
	}while(kb_state & 0x02);

}
static void kb_ack()
{
	t_8 kb_read;
	do{
		kb_read=in_byte(PORT_8042_DATA);

	}while(kb_read != KB_ACK);

}
