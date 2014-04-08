#ifndef __ERR_H__
#define __ERR_H__


/* assert and panic*/
#define ASSERT
#ifdef	ASSERT//if
void	assertion_failure(char *exp, char *myfile, char * base_file, int file);
#define assert(exp) if(exp);\
	else assertion_failure(#exp,__FILE__, __BASE_FILE__, __LINE__)
#else//else
#define assert(exp) disp_str("assert is NULL")
#endif//end


/*used by assert and panic*/
#define MAG_CH_ASSERT '\002'
#define MAG_CH_PANIC '\003'

void assertion_failure(char *exp, char *myfile, char * base_file, int line);
void spin(char *funcname);
void panic(const char *fmt,...);

#endif
