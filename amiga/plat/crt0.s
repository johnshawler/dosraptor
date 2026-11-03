	.text
	.section .text.start,"ax"
	.globl _start
_start:
	move.l	4.w,%a6
	move.l	%a6,SysBase
	move.l	%sp,saved_sp
	move.l	#0x40000,%d0		/* own 256 KB stack */
	move.l	#0x10001,%d1		/* MEMF_PUBLIC|MEMF_CLEAR */
	jsr	-198(%a6)		/* AllocMem */
	tst.l	%d0
	beq.s	1f
	move.l	%d0,stack_base
	add.l	#0x40000,%d0
	move.l	%d0,%sp
	jsr	amiga_main
	move.l	%d0,-(%sp)
	jsr	_exit_to_os
1:	move.l	saved_sp,%sp
	moveq	#20,%d0
	rts
	.globl _exit_to_os
_exit_to_os:
	move.l	4(%sp),%d7
	move.l	saved_sp,%sp
	move.l	SysBase,%a6
	move.l	stack_base,%a1
	move.l	#0x40000,%d0
	jsr	-210(%a6)		/* FreeMem */
	move.l	%d7,%d0
	rts
	.bss
	.globl SysBase, stack_base, saved_sp
SysBase:	.long 0
stack_base:	.long 0
saved_sp:	.long 0
	.text
	.globl vbl_isr_stub
vbl_isr_stub:
	movem.l	%d2-%d7/%a2-%a6,-(%sp)
	jsr	tsm_vbl
	movem.l	(%sp)+,%d2-%d7/%a2-%a6
	moveq	#1,%d0
	rts
	.globl cia_isr_stub
cia_isr_stub:
	movem.l	%d2-%d7/%a2-%a6,-(%sp)
	jsr	tsm_cia
	movem.l	(%sp)+,%d2-%d7/%a2-%a6
	moveq	#0,%d0
	rts
