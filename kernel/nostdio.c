#include "type.h"
#include "proc.h"
#include "proto.h"
#include "protect.h"
#include "string.h"
#include "const.h"


int printf(const char *fmt,...);
int vsprintf(char *buf,const char *fmt,va_list args);

int printf(const char *fmt,...)
{
	int i;
	char buf[256];

	va_list arg=(va_list) (fmt + 4);
	i=vsprintf(buf,fmt,arg);
	write(buf,i);
	return i;
}

int vsprintf(char *buf,const char *fmt,va_list args)
{
	char *p;
	char tmp[256];
	va_list p_next_arg=args;

	for(p=buf;*fmt;fmt++){
		if(*fmt!='%'){
			*p++=*fmt;
			continue;
		}

		fmt++;

		switch(*fmt){
			case 'x':
				itoa(tmp,*((int *)p_next_arg));
				strcpy(p,tmp);
				p_next_arg+=4;
				p+=strlen(tmp);
				break;
			case 's':
				break;
			default:
				break;
		}
	}

	return (p-buf);
}

