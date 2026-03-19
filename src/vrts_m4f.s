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
  // push {lr}
  // bl vrts_not_exist_fnc
  // pop {lr}
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