.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

PendSV_Handler:
	cpsid	i
	mrs r0, psp
	subs	r0, #16
	stmia	r0!,{r4-r7}
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	subs	r0, #32
	stmia	r0!,{r4-r7}
	subs	r0, #16
	ldr r2, =vrts_now_thread
	ldr r1, [r2]
	str r0, [r1]
	ldr	r2, =vrts_next_thread
	ldr	r1, [r2]
	ldr	r0, [r1]
	ldmia	r0!,{r4-r7}
	mov r8, r4
	mov r9, r5
	mov r10, r6
	mov r11, r7
	ldmia	r0!,{r4-r7}
	msr psp, r0
	ldr r0, =0xFFFFFFFD
	cpsie	i
	bx	r0

.size PendSV_Handler, .-PendSV_Handler
