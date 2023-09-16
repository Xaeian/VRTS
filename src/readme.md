# Only `3` files

[vrts.h](./vrts.h) - Header file

```
#ifndef VRTS_H
#define VRTS_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32g0xx.h"

#ifndef VRTS_THREAD_LIMIT
  #define VRTS_THREAD_LIMIT 12
#endif

#ifndef VRTS_SWITCHING
  #define VRTS_SWITCHING 1
#endif

#define sec(ms) (1000 * ms)

typedef enum {
  VRTS_Status_Idle = 1,
  VRTS_Status_Active = 2,
} VRTS_Status_e;

typedef struct {
  volatile uint32_t stack;
  void (*handler)(void);
  volatile VRTS_Status_e status;
} VRTS_Task_t;

bool thread(void (*handler)(void), uint32_t *stack, uint16_t size);
void let(void);
void delay(uint32_t ms);
void sleep(uint32_t ms);
bool timeout(uint32_t ms, bool (*Free)(void *), void *subject);
uint64_t gettick(uint32_t ms);
bool waitfor(uint64_t *tick);
int32_t watch(uint64_t tick);

void VRTS_Init(void);
void VRTS_Lock(void);
bool VRTS_Unlock(void);
bool SYSTICK_Init(uint32_t systick_ms);

#endif
```

[vrts.c](./vrts.c) - The main library file

```cpp
#include "vrts.h"

volatile uint64_t vrts_ticker;
uint32_t vrts_ms;
volatile VRTS_Task_t *vrts_now_thread;
volatile VRTS_Task_t *vrts_next_thread;

#if(VRTS_SWITCHING)

struct {
  VRTS_Task_t threads[VRTS_THREAD_LIMIT];
  uint32_t thread_nbr;
  uint32_t size;
  bool enabled;
  bool init;
} vrts_state;

static void VRTS_TaskFinished(void)
{
  volatile uint32_t i = 0;
  while(1) i++;
}

bool thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  if(vrts_state.size >= VRTS_THREAD_LIMIT - 1) return false;
  VRTS_Task_t *thread = &vrts_state.threads[vrts_state.size];
  thread->handler = handler;
  thread->stack = (uint32_t)(stack + size - 16);
  thread->status = VRTS_Status_Idle;
  stack[size - 1] = 0x01000000; // XPSR: Default value
  stack[size - 2] = (uint32_t)handler; // PC: Point to the handler function
  stack[size - 3] = (uint32_t)&VRTS_TaskFinished; // LR: Point to a function to be called when the handler returns
  // Next: R12, R3, R2, R1, R0, R7, R6, R5, R4, R11, R10, R9, R8
  vrts_state.size++;
  return true;
}

void VRTS_Init(void)
{
  NVIC_SetPriority(PendSV_IRQn, 3);
  vrts_now_thread = &vrts_state.threads[vrts_state.thread_nbr];
  __set_PSP(vrts_now_thread->stack + 64); // Set PSP to the top of thread's stack
  __set_CONTROL(0x02); // Switch to PSP, privileged mode
  __ISB(); // Exec. ISB after changing CONTORL (recommended)
  vrts_state.enabled = true;
  vrts_state.init = true;
  vrts_now_thread->handler();
}

void VRTS_Lock(void)
{
  vrts_state.enabled = false;
}

bool VRTS_Unlock(void)
{
  if(!vrts_state.init) return false;
  vrts_state.enabled = true;
  return true;
}

void let(void)
{
  if(!vrts_state.enabled) return;
  vrts_now_thread = &vrts_state.threads[vrts_state.thread_nbr];
  vrts_now_thread->status = VRTS_Status_Idle;
  vrts_state.thread_nbr++;
  if(vrts_state.thread_nbr >= vrts_state.size)
    vrts_state.thread_nbr = 0;
  vrts_next_thread = &vrts_state.threads[vrts_state.thread_nbr];
  vrts_next_thread->status = VRTS_Status_Active;
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

#else
void let(void)
{
  __WFI();
}
#endif

uint64_t gettick(uint32_t offset_ms)
{
  return vrts_ticker + ((offset_ms + (vrts_ms - 1)) / vrts_ms);
}

void delay(uint32_t ms)
{
  uint64_t end = gettick(ms);
  while(end > vrts_ticker) let();
}

void sleep(uint32_t ms)
{
  uint64_t end = gettick(ms);
  while(end > vrts_ticker) __WFI();
}

bool timeout(uint32_t ms, bool (*Free)(void *), void *subject)
{
  uint64_t end = gettick(ms);
  while(end > vrts_ticker) {
    if(Free(subject)) {
      return false;
    }
    let();
  }
  return true;
}

bool waitfor(uint64_t *tick)
{
  if(!*tick || *tick > vrts_ticker) return false;
  *tick = 0;
  return true;
}

int32_t watch(uint64_t tick)
{
  return (int32_t)(((int64_t)vrts_ticker - tick) * vrts_ms) ;
}

bool SYSTICK_Init(uint32_t systick_ms)
{
  vrts_ms = systick_ms;
  uint32_t systick_ticks = (uint32_t)((float)systick_ms * SystemCoreClock / 1000);
  if(SysTick_Config(systick_ticks) != 0) return false;
  NVIC_SetPriority(SysTick_IRQn, 3);
  return true;
}

void SysTick_Handler(void)
{
  vrts_ticker++;
}
```

[vrts-pendsv.s](./vrts-pendsv.s) - Assembler file, which includes function for thread switching

```
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
```