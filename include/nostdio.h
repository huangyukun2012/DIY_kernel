#ifndef _NOSTDIO_H
#define _NOSTDIO_H
extern int printf(const char *fmt,...);
#define printl printf
extern int vsprintf(char *buf,const char *fmt,va_list args);

#define O_CREAT     1
#define O_RDWR      2                                                                                                                                

#define SEEK_SET    1
#define SEEK_CUR    2
#define SEEK_END    3

extern int open(const char * pathname, int flags);
#endif
