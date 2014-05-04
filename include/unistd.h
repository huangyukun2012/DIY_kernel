#ifndef	__UNISTD_H__
#define __UNISTD_H__
#include "stdio.h"
int write(int fd, const void *buf, int count);

int read(int fd, void *buf, int count);

int  close(int fd);

int open(const char *pathname, int flags);

int unlink(const char *pathname);

int fork();

int wait(int * );

void exit(int );

int getpid();

int stat(const char *pathname, struct stat *statp);

int execl(const char* path, const char *arg, ...);

int execv(const char *path, char *argv[]);

#endif
