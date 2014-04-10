#ifndef _NOSTDIO_H
#define _NOSTDIO_H
extern int printf(const char *fmt,...);
#define printl printf
extern int vsprintf(char *buf,const char *fmt,va_list args);
#endif
