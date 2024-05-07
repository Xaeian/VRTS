#include "vrts.h"
#include "new.h"

volatile uint64_t vrts_ticker;
uint32_t vrts_ms, vrts_overflow;
volatile VRTS_Task_t *vrts_now_thread;
volatile VRTS_Task_t *vrts_next_thread;
uint16_t vrts_tick2ms; // How many clock-ticks make up 1ms

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
  NEW_Init(vrts_state.size);
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

uint8_t VRTS_ActiveThread(void)
{
  #if(VRTS_SWITCHING)
    return vrts_state.thread_nbr;
  #else
    return 0;
  #endif
}

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

void delay_until(uint64_t *tick)
{
  if(!*tick) return;
  while(*tick > vrts_ticker) let();
  *tick = 0;
}

void sleep_until(uint64_t *tick)
{
  if(!*tick) return;
  while(*tick > vrts_ticker) __WFI();
  *tick = 0;
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
  vrts_tick2ms = SystemCoreClock / 1000;
  vrts_overflow = (uint32_t)((float)vrts_ms * vrts_tick2ms);
  if(SysTick_Config(vrts_overflow) != 0) return false;
  NVIC_SetPriority(SysTick_IRQn, 3);
  return true;
}

void SysTick_Handler(void)
{
  vrts_ticker++;
}
