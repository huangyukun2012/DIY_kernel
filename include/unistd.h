
int write(int fd, const void *buf, int count);

int read(int fd, void *buf, int count);

int  close(int fd);

int open(const char *pathname, int flags);

int unlink(const char *pathname);

int fork();

int getpid();
