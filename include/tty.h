#ifndef _TTY_H_
#define _TTY_H_

#define TTY_BUFFER_SIZE 256
#define TTY_OUT_BUF_LEN 2

#define NR_TTY 3

struct s_console;

typedef	struct tty{
	t_32	buffer[TTY_BUFFER_SIZE];
	t_32	*head;
	t_32	*tail;
	int 	count;
	int 	caller;
	int		proc2tty;
	char*		reqer_buf;
	int 	reqed_cnt;
	int		sended_cnt;

	struct s_console *console_p;
}TTY;

extern TTY * current_tty;
#endif
