// hal/stm32/sys/vrts.c

#include "vrts.h"

//------------------------------------------------------------------------------------------------- Panic

__attribute__((weak)) void vrts_panic(const char *msg)
{
  (void)msg;
  __disable_irq();
  while(1);
}

//------------------------------------------------------------------------------------------------- Globals

volatile uint64_t VrtsTicker;
static uint32_t tick_ms; // time in ms for a single ticker tick

volatile VRTS_Task_t *vrts_now_thread; // Current thread
volatile VRTS_Task_t *vrts_next_thread; // Next thread

//------------------------------------------------------------------------------------------------- Threads

#if(VRTS_SWITCHING)

#if(VRTS_THREAD_TIMEOUT_MS)
  static uint32_t hold_timeout;
  static volatile uint32_t hold_ticker;
#endif

// Structure to manage threads in the VRTS system
typedef struct {
  VRTS_Task_t threads[VRTS_THREAD_LIMIT];
  uint32_t i; // Active thread
  uint32_t count; // Thread count
  volatile bool enabled; // Switching VRTS enabled flag
  volatile bool init; // VRTS initialization flag
} VRTS_t;

static VRTS_t vrts;

static void VRTS_TaskFinished(void)
{
  while(1) __WFI();
}

bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  if(vrts.count >= VRTS_THREAD_LIMIT - 1) return false;
  VRTS_Task_t *thread = &vrts.threads[vrts.count];
  thread->handler = handler;
  #if defined(STM32WB)
    thread->stack = (uint32_t)(stack + size - 17);
    stack[size - 1] = (1 << 24); // XPSR: Default value
    stack[size - 2] = (uint32_t)handler; // PC: Point to the handler function
    stack[size - 3] = (uint32_t)&VRTS_TaskFinished; // LR: handler return
    stack[size - 9] = 0xFFFFFFFD; // EXC_RETURN
    // Zero r4-r11 (software saved)
    for(int i = 10; i <= 17; i++) stack[size - i] = 0;
  #else
    thread->stack = (uint32_t)(stack + size - 16);
    stack[size - 1] = 0x01000000; // XPSR: Default value
    stack[size - 2] = (uint32_t)handler; // PC: Point to the handler function
    stack[size - 3] = (uint32_t)&VRTS_TaskFinished; // LR: handler return
    // Zero r4-r11 (software saved): positions size-9 to size-16
    for(int i = 9; i <= 16; i++) stack[size - i] = 0;
  #endif
  vrts.count++;
  return true;
}

void vrts_init(void)
{
  NVIC_SetPriority(PendSV_IRQn, 3);
  vrts_now_thread = &vrts.threads[vrts.i];
  #if defined(STM32WB)
    __set_PSP(vrts_now_thread->stack + 68); // Set PSP to the top of thread's stack
  #else
    __set_PSP(vrts_now_thread->stack + 64); // Set PSP to the top of thread's stack
  #endif
  __set_CONTROL(0x02); // Switch to PSP, privileged mode
  __ISB(); // Exec. ISB after changing CONTROL (recommended)
  vrts.enabled = true;
  vrts.init = true;
  vrts_now_thread->handler();
}

void vrts_lock(void)
{
  vrts.enabled = false;
}

bool vrts_unlock(void)
{
  if(!vrts.init) return false;
  vrts.enabled = true;
  return true;
}

void let(void)
{
  // Guard: forbid let() from ISR context
  if(__get_IPSR() != 0) {
    vrts_panic("let() called from ISR");
    return;
  }
  if(!vrts.enabled) return;
  vrts_now_thread = &vrts.threads[vrts.i];
  vrts.i++;
  if(vrts.i >= vrts.count) vrts.i = 0;
  vrts_next_thread = &vrts.threads[vrts.i];
  #if(VRTS_THREAD_TIMEOUT_MS)
    hold_ticker = hold_timeout;
  #endif
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  __DSB();
}

#else
void let(void)
{
  __WFI();
}
#endif

uint8_t vrts_active_thread(void)
{
  #if(VRTS_SWITCHING)
    return vrts.i;
  #else
    return 0;
  #endif
}

//------------------------------------------------------------------------------------------------- Tick

static inline uint64_t vrts_ticker_get(void)
{
  // Atomic 64-bit read on 32-bit core
  uint32_t hi1, hi2, lo;
  do {
    hi1 = (uint32_t)(VrtsTicker >> 32);
    lo  = (uint32_t)VrtsTicker;
    hi2 = (uint32_t)(VrtsTicker >> 32);
  } while(hi1 != hi2);
  return ((uint64_t)hi1 << 32) | lo;
}

uint64_t tick_keep(uint32_t offset_ms)
{
  if(!tick_ms) return vrts_ticker_get(); // Guard: before systick_init
  return vrts_ticker_get() + ((offset_ms + (tick_ms - 1)) / tick_ms);
}

uint64_t tick_now(void)
{
  return vrts_ticker_get();
}

bool tick_over(uint64_t *tick)
{
  if(!*tick || *tick > vrts_ticker_get()) return false;
  *tick = 0;
  return true;
}

bool tick_away(uint64_t *tick)
{
  if(!*tick) return false;
  if(*tick > vrts_ticker_get()) return true;
  *tick = 0;
  return false;
}

int32_t tick_diff(uint64_t tick)
{
  return (int32_t)(((int64_t)vrts_ticker_get() - tick) * tick_ms);
}

//------------------------------------------------------------------------------------------------- Delay

void delay(uint32_t ms)
{
  uint64_t end = tick_keep(ms);
  while(end > vrts_ticker_get()) let();
}

void sleep(uint32_t ms)
{
  uint64_t end = tick_keep(ms);
  while(end > vrts_ticker_get()) __WFI();
}

bool timeout(uint32_t ms, bool (*Free)(void *), void *subject)
{
  uint64_t end = tick_keep(ms);
  while(end > vrts_ticker_get()) {
    if(Free(subject)) return false;
    let();
  }
  return true;
}

void delay_until(uint64_t *tick)
{
  if(!*tick) return;
  while(*tick > vrts_ticker_get()) let();
  *tick = 0;
}

void sleep_until(uint64_t *tick)
{
  if(!*tick) return;
  while(*tick > vrts_ticker_get()) __WFI();
  *tick = 0;
}

//------------------------------------------------------------------------------------------------- Init

bool systick_init(uint32_t systick_ms)
{
  if(!systick_ms) return false;
  tick_ms = systick_ms;
  // Integer math: (tick_ms * SystemCoreClock) / 1000
  // Use uint64_t to avoid overflow for high clocks
  uint64_t reload = ((uint64_t)tick_ms * SystemCoreClock) / 1000;
  if(reload == 0 || reload > 0x00FFFFFF) return false; // SysTick 24-bit limit
  #if(VRTS_SWITCHING && VRTS_THREAD_TIMEOUT_MS)
    hold_timeout = VRTS_THREAD_TIMEOUT_MS / systick_ms;
    hold_ticker = hold_timeout;
  #endif
  if(SysTick_Config((uint32_t)reload)) return false;
  NVIC_SetPriority(SysTick_IRQn, 2); // Higher prio than PendSV(3)
  return true;
}

void SysTick_Handler(void)
{
  VrtsTicker++;
  #if(VRTS_SWITCHING && VRTS_THREAD_TIMEOUT_MS)
    if(vrts.init) {
      hold_ticker--;
      if(!hold_ticker) vrts_panic("Thread overran core time limit");
    }
  #endif
}