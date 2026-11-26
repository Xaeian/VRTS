// src/vrts_pendsv.s

// Context switch of VRTS: `PendSV_Handler` saves the running thread on its own stack,
// keeps the stack top in `vrts_now_thread` and resumes `vrts_next_thread`.
// The core picks the variant: Cortex-M4F also saves `s16-s31` of a thread that used the FPU.

#if defined(__ARM_ARCH_6M__)

.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

PendSV_Handler:
  mrs r3, PRIMASK
  cpsid i
  mrs r0, psp
  subs r0, #16
  stmia r0!, {r4-r7}
  mov r4, r8
  mov r5, r9
  mov r6, r10
  mov r7, r11
  subs r0, #32
  stmia r0!, {r4-r7}
  subs r0, #16
  ldr r2, =vrts_now_thread
  ldr r1, [r2]
  str r0, [r1]
  ldr r2, =vrts_next_thread
  ldr r1, [r2]
  ldr r0, [r1]
  ldmia r0!, {r4-r7}
  mov r8, r4
  mov r9, r5
  mov r10, r6
  mov r11, r7
  ldmia r0!, {r4-r7}
  msr psp, r0
  isb
  msr PRIMASK, r3
  ldr r0, =0xFFFFFFFD
  bx r0

.size PendSV_Handler, .-PendSV_Handler

#elif defined(__ARM_ARCH_7EM__)

.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

PendSV_Handler:
  cpsid f
  tst lr,0x04
  ite eq
  mrseq r0,msp
  mrsne r0,psp
  tst lr,0x10
  it eq
  vstmdbeq r0!,{s16-s31}
  stmdb r0!,{r4-r11,lr}
  tst lr,0x04
  it eq
  msreq msp,r0
  ldr r2,=vrts_now_thread
  ldr r1,[r2]
  str r0,[r1]
  ldr r2,=vrts_next_thread
  ldr r1,[r2]
  ldr r0,[r1]
  ldmia r0!,{r4-r11,lr}
  tst lr,0x10
  it eq
  vldmiaeq r0!,{s16-s31}
  mrs r1,control
  tst lr,0x04
  ittee eq
  biceq r1,0x03
  msreq msp,r0
  orrne r1,0x02
  msrne psp,r0
  msr control,r1
  isb
  cpsie f
  bx lr

.size PendSV_Handler, .-PendSV_Handler

#else
  #error "VRTS has no context switch for this core"
#endif
