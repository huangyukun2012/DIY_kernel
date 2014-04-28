#ifndef __STDIO_H__
#define __STDIO_H__
int printl(const char *fmt, ...);
extern int printf(const char *fmt,...);
//#define printl printf
extern int vsprintf(char *buf,const char *fmt,va_list args);
int sprintf(char *buf, const char *fmt, ...);

#define O_CREAT     1
#define O_RDWR      2                                                                                                                                

#define SEEK_SET    1
#define SEEK_CUR    2
#define SEEK_END    3

#endif
