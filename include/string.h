#ifndef _STRING_H
#define _STRING_H
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            string.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define STR_DEFAULT_LEN 1024

void*	memcpy(void* p_dst, void* p_src, int size);
void	memset(void* p_dst, char ch, int size);
char*	strcpy(char* p_dst, char* p_src);
int	strlen(const char* p_dst);
#define phys_copy	memcpy
#define phys_set	memset

#endif
