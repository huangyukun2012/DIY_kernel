
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_printx		equ	0	; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_sendrec			equ	1
INT_VECTOR_SYS_CALL	equ	0x90

; 导出符号
global	printx
global	sendrec

bits 32
[section .text]
;============================================================================================================
;printx(char *)
printx:
	mov	eax,_NR_printx
	mov	edx,[esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

;=========================================================================================
;sendrec(int function , int dest_src, MESSAGE *m)
sendrec:
	mov	eax,_NR_sendrec
	mov	ebx,[esp+4];function
	mov	ecx,[esp+8];sec_dest
	mov	edx,[esp+12];p_msg
	int INT_VECTOR_SYS_CALL
	ret

