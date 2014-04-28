/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
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
#include "err.h"
#include "math.h"
#include "stdio.h"

//static void page_switch();
void task_tty();
void in_process(TTY* tty_p,t_32 key);
void syn_cursor();
void tty_read(TTY *tty_p);
void tty_write(TTY *tty_p);
void init_tty(TTY *tty_p);
extern KB_INPUT kb_in;
void put_key2tty(TTY *tty_p, t_32 key);
void write_tty(TTY * tty_p,char *buf,int len);
int sys_write(char *buf,int len,PROCESS *p_proc);
void tty_do_read(TTY * tty_p, MESSAGE *msg);
void tty_do_write(TTY * tty_p, MESSAGE *msg);
/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	disp_str("task_tty begins:>>>\n");
	MESSAGE msg;
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
			do{
				tty_read(tty_p);
				tty_write(tty_p);
			}while(tty_p->count);
		}

		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		assert(src != TASK_TTY);

		TTY *dev_ttyp = &tty_table[msg.DEVICE];//the dest device in msg
		switch(msg.type){//similar to hd_task
			case DEV_OPEN://return without any operation
				reset_msg(&msg);
				msg.type = SYSCALL_RET;
				send_recv(SEND, src, &msg);
				break;
			case DEV_READ:
				tty_do_read(dev_ttyp, &msg);
				break;
			case DEV_WRITE:
				tty_do_write(dev_ttyp, &msg);
				break;
			case HARD_INT:
				key_pressed = 0;//not def
				continue;
			default:
				dump_msg("TTY: unknown msg", &msg);
				break;

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
{//out put all the char in tty
	char ch='a';
	int backspace_nr=0;
	while(tty_p->count >0 ){
		ch=*(tty_p->tail);
		tty_p->tail++;
		if(tty_p->tail==tty_p->buffer+TTY_BUFFER_SIZE){
			tty_p->tail=tty_p->buffer;
		}
		tty_p->count--;//get char

		if(tty_p->reqed_cnt){
			if(ch >=' ' && ch<='~'){//printable
				out_char(tty_p->console_p,ch); 
				char *dest = tty_p->reqer_buf + tty_p->sended_cnt;
				*dest = ch;
				tty_p->sended_cnt++;
				tty_p->reqed_cnt--;
			}
			else if(ch == '\b' && tty_p->sended_cnt){//recover the last char
				backspace_nr++;
				out_char(tty_p->console_p,ch); 
				tty_p->sended_cnt--;
				tty_p->reqed_cnt++;
			}

			if(ch == '\n' || tty_p->reqed_cnt == 0){//enter key
				out_char(tty_p->console_p,ch); 
				MESSAGE msg;
				msg.type = RESUME_PROC;//und
				msg.PROC_NR = tty_p->proc2tty;
				msg.CNT = tty_p->sended_cnt;
				send_recv(SEND, tty_p->caller , &msg);
				tty_p->reqed_cnt = 0;
			}
		}
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

/*PRIVATE void page_switch()
{
	set_console_start_addr(80*15);
}*/
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
int sys_printx(int _unused1, int _unused2, char * s, struct proc *proc_p)
{
	const char *p;
	char ch;

	char reenter_err[]="? k_reenter is incorect for unknow reason";

	if(k_reenter ==0 )//called in ring0
		p=va2la(proc2pid(proc_p),s);
	else if(k_reenter > 0){//called in ring1~3
		p=s;
	}
	else//never
		p=reenter_err;
	
	if(*p==MAG_CH_PANIC ||
			(*p==MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS])){
		disable_int();
		char *v=(char *)V_MEM_BASE;
		const char *q=p+1;

		while(v< (char *)(V_MEM_BASE + V_MEM_SIZE)){
			*v++=*q++;
			*v++=RED_CHAR;
			if(!*q){
				while( ((int )v- V_MEM_BASE ) % (SCREEN_WIDETH *16)){
					v++;
					*v++=GRAY_CHAR;
				}
				q=p+1;
			}
		}
		/*__asm__ __volatile__("hlt");*/
	}

	while( (ch=*p++) != 0){
		if(ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT )
			continue;
		//int nr=proc_p->nr_tty;
		out_char(tty_table[0].console_p, ch);
	}
	return 0;
}

/***********invoke when task_tty receive DEV_READ msg
 *    when FS receive msg from userproc and send it to tty;
 * will send it imediatly with type SUSPEND_PROC
 * */
void tty_do_read(TTY * tty_p, MESSAGE *msg)
{
	//unpack the message
	tty_p->caller = msg->source;//FS
	tty_p->proc2tty = msg->PROC_NR;//the sender
	tty_p->reqer_buf = va2la(tty_p->proc2tty , msg->BUF);
	tty_p->sended_cnt = 0;
	tty_p->reqed_cnt = msg->CNT;

		//pack msg and send it
	msg->type = SUSPEND_PROC;
	send_recv(SEND, tty_p->caller, msg);
}

/* write the content to tty buffer from proc buffer who want to receive the eontent*/

void tty_do_write(TTY *tty_p, MESSAGE * msg)
{
	char buf[TTY_OUT_BUF_LEN];//und
	char *p = (char *)va2la(msg->PROC_NR, msg->BUF);
	int i = msg->CNT;

	int j;

	while(i){
		int bytes = min(TTY_OUT_BUF_LEN, i);
		phys_copy(va2la(TASK_TTY, buf), (void *)p, bytes);
		for(j= 0; j<bytes; j++){
			out_char(tty_p->console_p, buf[j]);
		}
		i -= bytes;
		p += bytes;

		}
		
	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}
