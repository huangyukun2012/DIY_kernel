#ifndef _TTY_H_
#define _TTY_H_

#define TTY_BUFFER_SIZE 256

#define NR_TTY 3

struct s_console;

typedef	struct tty{
	t_32	buffer[TTY_BUFFER_SIZE];
	t_32	*head;
	t_32	*tail;
	int 	count;
	struct s_console *console_p;
}TTY;

TTY * current_tty;
#endif
