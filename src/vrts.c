// src/vrts.c

#include "vrts.h"

#if(VRTS_TICKLESS)
  #include "pwr.h"
#endif

//------------------------------------------------------------------------------------------ Config

// Bottom word of every thread stack: a value a stack running over it would change
#define VRTS_STACK_CANARY 0x5AFEC0DEu

//------------------------------------------------------------------------------------------- State

__attribute__((weak)) void vrts_panic(const char *msg)
{
  (void)msg;
  __disable_irq();
  while(1);
}

volatile uint64_t VrtsTicker;
static uint32_t tick_ms; // system tick period [ms]

// Context switch operands of `PendSV_Handler` in `vrts_pendsv.s`
volatile VRTS_Task_t *vrts_now_thread;
volatile VRTS_Task_t *vrts_next_thread;

//----------------------------------------------------------------------------------------- Threads
#if(VRTS_SWITCHING)

#if(VRTS_THREAD_TIMEOUT_MS)
  static uint32_t hold_timeout;          // longest hold [ticks]
  static volatile uint32_t hold_ticker;  // ticks left before `vrts_panic`
#endif

typedef struct {
  VRTS_Task_t threads[VRTS_THREAD_LIMIT];
  uint32_t i;             // active thread index
  uint32_t count;         // thread count
  volatile bool enabled;  // switching enabled
  volatile bool init;     // `vrts_init` done
} VRTS_t;

static VRTS_t vrts;

static inline uint64_t ticker_get(void);

// Landing pad for a handler that returns, the yield keeps the others running
static void task_finished(void)
{
  while(1) let();
}

bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  if(vrts.count >= VRTS_THREAD_LIMIT) return false;
  VRTS_Task_t *thread = &vrts.threads[vrts.count];
  thread->handler = handler;
  thread->limit = stack;
  stack[0] = VRTS_STACK_CANARY;
  #if defined(__ARM_ARCH_7EM__)
    // M4 with FPU: 17 words = 8 hardware + 1 `EXC_RETURN` + 8 software-saved (r4-r11).
    // `EXC_RETURN` `0xFFFFFFFD`: thread mode, PSP, no FPU context (bit 4 set).
    // PendSV checks bit 4 to decide if lazy FPU state must be stacked on the next switch.
    thread->stack = (uint32_t)(stack + size - 17);
    stack[size - 1] = (1 << 24); // XPSR: Thumb bit
    stack[size - 2] = (uint32_t)handler;
    stack[size - 3] = (uint32_t)&task_finished; // LR
    stack[size - 9] = 0xFFFFFFFD;
    for(int i = 10; i <= 17; i++) stack[size - i] = 0; // r4-r11
  #else
    // M0+: 16 words = 8 hardware + 8 software-saved, no FPU, no `EXC_RETURN` slot
    thread->stack = (uint32_t)(stack + size - 16);
    stack[size - 1] = (1 << 24); // XPSR: Thumb bit
    stack[size - 2] = (uint32_t)handler;
    stack[size - 3] = (uint32_t)&task_finished; // LR
    for(int i = 9; i <= 16; i++) stack[size - i] = 0; // r4-r11
  #endif
  vrts.count++;
  return true;
}

void vrts_init(void)
{
  NVIC_SetPriority(PendSV_IRQn, 3);
  vrts_now_thread = &vrts.threads[vrts.i];
  // PSP at the top of the first thread's frame
  #if defined(__ARM_ARCH_7EM__)
    __set_PSP(vrts_now_thread->stack + 68);
  #else
    __set_PSP(vrts_now_thread->stack + 64);
  #endif
  __set_CONTROL(0x02); // PSP, privileged
  __ISB();
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

// Ready unless waiting: for its tick, or for its condition, which the caller checks for it.
// A met condition ends the wait here, since a one-shot one is spent by this very call.
static bool thread_ready(VRTS_Task_t *thread)
{
  if(!thread->wake) return true;
  if(thread->Free && thread->Free(thread->subject)) {
    thread->wake = 0;
    return true;
  }
  return ticker_get() >= thread->wake;
}

// Next thread ready in turn after `current`, `current` itself checked last; `count` when none is
static uint32_t next_ready(uint32_t current)
{
  uint32_t i = current;
  do {
    i = i + 1 < vrts.count ? i + 1 : 0;
    if(thread_ready(&vrts.threads[i])) return i;
  } while(i != current);
  return vrts.count;
}

#if(VRTS_TICKLESS)

// Nearest tick a thread waits for, `0` when none does
static uint64_t wake_nearest(void)
{
  uint64_t nearest = 0;
  for(uint32_t i = 0; i < vrts.count; i++) {
    uint64_t wake = vrts.threads[i].wake;
    if(wake && (!nearest || wake < nearest)) nearest = wake;
  }
  return nearest;
}

__attribute__((weak)) bool vrts_idle_stop(void)
{
  return false;
}

// Sleep without the tick up to the nearest wait. The part of the tick period already run
// and the sleep go onto the ticker, and what falls short of a tick waits for the next sleep.
// A sleep `PWR_Idle` does not time, its `0`, falls back to `WFI` with the tick running.
static void sleep_core(void)
{
  static uint32_t asleep_us; // time no tick holds yet
  uint64_t wake = wake_nearest();
  uint64_t now = ticker_get();
  if(wake <= now + 1) {
    __WFI();
    return;
  }
  uint64_t span_ms = (wake - now) * tick_ms;
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  // A period run out has its tick pending, and that tick counts it
  uint32_t val = SysTick->VAL;
  if(val) asleep_us += (uint32_t)((uint64_t)(SysTick->LOAD - val) * 1000000u / SystemCoreClock);
  uint32_t slept_us = PWR_Idle(span_ms > 2000 ? 2000 : (uint32_t)span_ms, vrts_idle_stop());
  SysTick->VAL = 0;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
  asleep_us += slept_us;
  uint32_t period_us = tick_ms * 1000;
  VrtsTicker += asleep_us / period_us;
  asleep_us %= period_us;
  if(!slept_us) __WFI();
}

#else

static inline void sleep_core(void) { __WFI(); }

#endif

// No thread ready: the core sleeps until an interrupt, the system tick among them.
// The last check runs with interrupts masked, so one taken after it still ends the sleep:
// `WFI` returns on an interrupt pending behind the mask, which runs once the mask is lifted.
static uint32_t idle(uint32_t current)
{
  while(1) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t next = next_ready(current);
    if(next == vrts.count) sleep_core();
    __set_PRIMASK(primask);
    #if(VRTS_THREAD_TIMEOUT_MS)
      hold_ticker = hold_timeout; // a sleeping core is held by no thread
    #endif
    if(next != vrts.count) return next;
  }
}

void let(void)
{
  if(__get_IPSR() != 0) {
    vrts_panic("let() called from ISR");
    return;
  }
  if(!vrts.enabled) return;
  VRTS_Task_t *now = &vrts.threads[vrts.i];
  if(*now->limit != VRTS_STACK_CANARY) vrts_panic("Thread stack overflow");
  uint32_t next = next_ready(vrts.i);
  if(next == vrts.count) next = idle(vrts.i);
  #if(VRTS_THREAD_TIMEOUT_MS)
    hold_ticker = hold_timeout;
  #endif
  if(next == vrts.i) return;
  vrts_now_thread = now;
  vrts.i = next;
  vrts_next_thread = &vrts.threads[next];
  // A plain write: `ICSR` reads back a pending SysTick,
  // which `|=` would set pending again after the handler took it, so one tick would count twice.
  SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
  __DSB();
}

// Wait of the running thread, which `let` of every thread reads
static void wait_set(uint64_t wake, bool (*Free)(void *), void *subject)
{
  VRTS_Task_t *thread = &vrts.threads[vrts.i];
  thread->Free = Free;
  thread->subject = subject;
  thread->wake = wake;
}

static void wait_clear(void) { vrts.threads[vrts.i].wake = 0; }

// The wait of the running thread was ended by `let`, which found its condition met
static bool wait_met(void) { return !vrts.threads[vrts.i].wake; }

uint8_t vrts_active_thread(void)
{
  return vrts.i;
}

#else

void let(void)
{
  __WFI();
}

static void wait_set(uint64_t wake, bool (*Free)(void *), void *subject)
{
  (void)wake;
  (void)Free;
  (void)subject;
}

static void wait_clear(void) {}

static bool wait_met(void) { return false; }

uint8_t vrts_active_thread(void)
{
  return 0;
}

#endif
//-------------------------------------------------------------------------------------------- Tick

/**
 * @brief Atomic 64-bit read of the ticker on a 32-bit core.
 * @return Current tick value
 */
static inline uint64_t ticker_get(void)
{
  uint32_t hi1, hi2, lo;
  do {
    hi1 = (uint32_t)(VrtsTicker >> 32);
    lo = (uint32_t)VrtsTicker;
    hi2 = (uint32_t)(VrtsTicker >> 32);
  } while(hi1 != hi2);
  return ((uint64_t)hi1 << 32) | lo;
}

uint64_t tick_keep(uint32_t offset_ms)
{
  if(!tick_ms) return ticker_get(); // before `systick_init`
  return ticker_get() + ((offset_ms + (tick_ms - 1)) / tick_ms);
}

uint64_t tick_now(void)
{
  return ticker_get();
}

bool tick_over(volatile uint64_t *tick)
{
  if(!*tick || *tick > ticker_get()) return false;
  *tick = 0;
  return true;
}

bool tick_away(volatile uint64_t *tick)
{
  if(!*tick) return false;
  if(*tick > ticker_get()) return true;
  *tick = 0;
  return false;
}

int32_t tick_diff(uint64_t tick)
{
  return (int32_t)(((int64_t)ticker_get() - tick) * tick_ms);
}

//------------------------------------------------------------------------------------------- Delay

void delay(uint32_t ms)
{
  uint64_t end = tick_keep(ms);
  wait_set(end, NULL, NULL);
  while(end > ticker_get()) let();
  wait_clear();
}

void sleep(uint32_t ms)
{
  uint64_t end = tick_keep(ms);
  while(end > ticker_get()) __WFI();
}

bool timeout(uint32_t ms, bool (*Free)(void *), void *subject)
{
  uint64_t end = tick_keep(ms);
  wait_set(end, Free, subject);
  while(end > ticker_get()) {
    if(Free(subject)) {
      wait_clear();
      return false;
    }
    let();
    if(wait_met()) return false;
  }
  wait_clear();
  // Once more past the deadline: a condition met while another thread held the processor
  return !Free(subject);
}

void delay_until(uint64_t *tick)
{
  if(!*tick) return;
  wait_set(*tick, NULL, NULL);
  while(*tick > ticker_get()) let();
  wait_clear();
  *tick = 0;
}

void sleep_until(uint64_t *tick)
{
  if(!*tick) return;
  while(*tick > ticker_get()) __WFI();
  *tick = 0;
}

//----------------------------------------------------------------------------------------- SysTick

bool systick_init(uint32_t systick_ms)
{
  if(!systick_ms) return false;
  tick_ms = systick_ms;
  uint64_t reload = ((uint64_t)tick_ms * SystemCoreClock) / 1000;
  if(reload == 0 || reload > 0x00FFFFFF) return false; // 24-bit counter
  #if(VRTS_SWITCHING && VRTS_THREAD_TIMEOUT_MS)
  hold_timeout = VRTS_THREAD_TIMEOUT_MS / systick_ms;
  hold_ticker = hold_timeout;
  #endif
  if(SysTick_Config((uint32_t)reload)) return false;
  NVIC_SetPriority(SysTick_IRQn, 2); // above PendSV at 3
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

//-------------------------------------------------------------------------------------------------
