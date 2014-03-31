#include "type.h"
#include "const.h"
#include "proto.h"
#include "console.h"
#include "global.h"

#define COLOR	0x0C

void out_char(CONSOLE * p_con,char ch){
	t_8	*vmem_p=(t_8 *)(V_MEM_BASE + disp_pos);

	*vmem_p = ch;
	vmem_p++;
	*vmem_p = COLOR;
	disp_pos +=2;

	syn_cursor();
}
