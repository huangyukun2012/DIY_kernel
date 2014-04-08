
#include "const.h"
#include "type.h"
#include "tty.h"
#include "err.h"
#include "nostdio.h"

void assertion_failure(char *exp, char *file, char * base_file, int line)
{	
	printl("%c  assert(%s) failed: file: %s, base_file: %s,ln%d",MAG_CH_ASSERT,exp, file, base_file, line);
	spin("assertion_failure()");
	__asm__ __volatile__("ud2");

}

void spin(char *funcname)
{
	printl("\nspin in %s. . . \n",funcname);
	while(1){
	}
}

//===========================panic(const char *fmt,. . .)====================
void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	va_list arg=(va_list)((char*)&fmt+4);
	i=vsprintf(buf,fmt,arg);
	printl("%c !!panic!! %s",MAG_CH_PANIC,buf);

	__asm__ __volatile__("ud2");
}

