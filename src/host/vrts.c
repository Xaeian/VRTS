// src/host/vrts.c

#include "vrts.h"

#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else
  #include <pthread.h>
  #include <sched.h>
  #include <sys/time.h>
  #include <time.h> // `nanosleep`, since `unistd.h` would clash with the `sleep` here
#endif

//------------------------------------------------------------------------------------------- Panic

// `__attribute__((weak))` is not usable on PE/COFF: a weak definition
// in a separate translation unit stays an undefined weak symbol at link time.
#if defined(_WIN32) || defined(_WIN64)
void vrts_panic(const char *msg)
#else
__attribute__((weak)) void vrts_panic(const char *msg)
#endif
{
  fprintf(stderr, "vrts_panic: %s\n", msg);
  while(1);
}

//----------------------------------------------------------------------------------------- Globals

volatile uint64_t VrtsTicker;
bool VrtsVirtualTime;
static uint32_t tick_ms = 1;
static uint64_t start_time_ms;

//-------------------------------------------------------------------------------------------- Time

static uint64_t time_ms_get(void)
{
  #if defined(_WIN32) || defined(_WIN64)
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  return t / 10000;
  #else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
  #endif
}

static inline uint64_t vrts_ticker_get(void)
{
  if(VrtsVirtualTime) return VrtsTicker;
  return (time_ms_get() - start_time_ms) / tick_ms;
}

//--------------------------------------------------------------------------------------- Threading

#if(VRTS_SWITCHING)

// The target runs one thread at a time and hands the core over only inside `let()`.
// Host threads are real OS threads, so that contract is rebuilt on a baton: exactly one
// index owns the CPU, `let()` passes it on round-robin, and every other thread blocks.
// Without it `vrts_lock` guards nothing and the per-thread heap stacks all collapse
// onto index 0, which is not how the same code behaves on the target.
static struct {
  void (*handlers[VRTS_THREAD_LIMIT])(void);
  #if defined(_WIN32) || defined(_WIN64)
  HANDLE threads[VRTS_THREAD_LIMIT];
  CRITICAL_SECTION lock;
  CONDITION_VARIABLE turn;
  #else
  pthread_t threads[VRTS_THREAD_LIMIT];
  pthread_mutex_t lock;
  pthread_cond_t turn;
  #endif
  uint32_t count;
  volatile uint32_t owner;
  volatile bool enabled;
  volatile bool init;
} vrts;

// Index of the calling thread. Thread `0` is the one that called `vrts_init`,
// mirroring the target, where `vrts_init` enters the first handler on the main stack.
static __thread uint32_t vrts_me;

static void sched_lock(void)
{
  #if defined(_WIN32) || defined(_WIN64)
  EnterCriticalSection(&vrts.lock);
  #else
  pthread_mutex_lock(&vrts.lock);
  #endif
}

static void sched_unlock(void)
{
  #if defined(_WIN32) || defined(_WIN64)
  LeaveCriticalSection(&vrts.lock);
  #else
  pthread_mutex_unlock(&vrts.lock);
  #endif
}

static void sched_wait_turn(void)
{
  while(vrts.owner != vrts_me) {
    #if defined(_WIN32) || defined(_WIN64)
    SleepConditionVariableCS(&vrts.turn, &vrts.lock, INFINITE);
    #else
    pthread_cond_wait(&vrts.turn, &vrts.lock);
    #endif
  }
}

static void sched_wake(void)
{
  #if defined(_WIN32) || defined(_WIN64)
  WakeAllConditionVariable(&vrts.turn);
  #else
  pthread_cond_broadcast(&vrts.turn);
  #endif
}

// One idle tick per full round. The target burns the core in a cooperative wait too,
// but a host process that does the same pins a CPU for nothing.
static void sched_idle(void)
{
  #if defined(_WIN32) || defined(_WIN64)
  Sleep(1);
  #else
  nanosleep(&(struct timespec){ .tv_nsec = 1000000 }, NULL);
  #endif
}

#if defined(_WIN32) || defined(_WIN64)
static DWORD WINAPI vrts_wrapper(LPVOID param)
#else
static void *vrts_wrapper(void *param)
#endif
{
  vrts_me = (uint32_t)(uintptr_t)param;
  sched_lock();
  sched_wait_turn();
  sched_unlock();
  vrts.handlers[vrts_me]();
  // Landing pad for a handler that returns, same as the target
  while(1) let();
  #if defined(_WIN32) || defined(_WIN64)
  return 0;
  #else
  return NULL;
  #endif
}

bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  (void)stack;
  (void)size;
  if(!handler) return false;
  if(vrts.count >= VRTS_THREAD_LIMIT) return false;
  vrts.handlers[vrts.count] = handler;
  vrts.count++;
  return true;
}

void vrts_init(void)
{
  if(!vrts.count) return;
  #if defined(_WIN32) || defined(_WIN64)
  InitializeCriticalSection(&vrts.lock);
  InitializeConditionVariable(&vrts.turn);
  #else
  pthread_mutex_init(&vrts.lock, NULL);
  pthread_cond_init(&vrts.turn, NULL);
  #endif
  vrts_me = 0;
  vrts.owner = 0;
  vrts.enabled = true;
  vrts.init = true;
  // Threads start blocked: the baton is with index `0`, which is this one
  for(uint32_t i = 1; i < vrts.count; i++) {
    #if defined(_WIN32) || defined(_WIN64)
    vrts.threads[i] = CreateThread(NULL, 0, vrts_wrapper, (LPVOID)(uintptr_t)i, 0, NULL);
    if(!vrts.threads[i]) vrts_panic("thread create failed");
    #else
    if(pthread_create(&vrts.threads[i], NULL, vrts_wrapper, (void *)(uintptr_t)i)) {
      vrts_panic("thread create failed");
    }
    #endif
  }
  // Does not return, matching the target
  vrts.handlers[0]();
  while(1) let();
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
  if(!vrts.init) { sched_idle(); return; }
  if(!vrts.enabled) return;
  sched_lock();
  uint32_t next = vrts.owner + 1;
  if(next >= vrts.count) next = 0;
  vrts.owner = next;
  // A round has closed: pace the host before anyone runs again
  if(!next) sched_idle();
  if(next == vrts_me) { sched_unlock(); return; }
  sched_wake();
  sched_wait_turn();
  sched_unlock();
}

uint8_t vrts_active_thread(void)
{
  return (uint8_t)vrts.owner;
}

#else

bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  (void)handler;
  (void)stack;
  (void)size;
  return false;
}

void vrts_init(void) {}
void vrts_lock(void) {}
bool vrts_unlock(void) { return true; }
void let(void) {}
uint8_t vrts_active_thread(void) { return 0; }

#endif

//-------------------------------------------------------------------------------------------- Tick

uint64_t tick_keep(uint32_t offset_ms)
{
  return vrts_ticker_get() + ((offset_ms + tick_ms - 1) / tick_ms);
}

uint64_t tick_now(void) { return vrts_ticker_get(); }

bool tick_over(volatile uint64_t *tick)
{
  if(!*tick || *tick > vrts_ticker_get()) return false;
  *tick = 0;
  return true;
}

bool tick_away(volatile uint64_t *tick)
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

//------------------------------------------------------------------------------------------- Delay

void delay(uint32_t ms)
{
  uint64_t end = tick_keep(ms);
  while(end > vrts_ticker_get()) let();
}

void sleep(uint32_t ms)
{
  #if defined(_WIN32) || defined(_WIN64)
  Sleep(ms);
  #else
  nanosleep(&(struct timespec){ .tv_sec = ms / 1000, .tv_nsec = ms % 1000 * 1000000L }, NULL);
  #endif
}

bool timeout(uint32_t ms, bool (*Free)(void *), void *subject)
{
  uint64_t end = tick_keep(ms);
  while(end > vrts_ticker_get()) {
    if(Free(subject)) return false;
    let();
  }
  // Once more past the deadline: a condition met while another thread held the processor
  return !Free(subject);
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
  uint64_t now = vrts_ticker_get();
  if(*tick > now) sleep((uint32_t)((*tick - now) * tick_ms));
  *tick = 0;
}

//-------------------------------------------------------------------------------------------- Init

bool systick_init(uint32_t systick_ms)
{
  if(!systick_ms) return false;
  tick_ms = systick_ms;
  start_time_ms = time_ms_get();
  return true;
}

//-------------------------------------------------------------------------------------------------
