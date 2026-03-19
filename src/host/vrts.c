// hal/host/vrts.c

#include "vrts.h"
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#else
  #include <pthread.h>
  #include <sched.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif

//------------------------------------------------------------------------------------------------- Panic

__attribute__((weak)) void vrts_panic(const char *msg)
{
  fprintf(stderr, "vrts_panic: %s\n", msg);
  while(1);
}

//------------------------------------------------------------------------------------------------- Globals

volatile uint64_t VrtsTicker;
static uint32_t tick_ms = 1;
static uint64_t start_time_ms;

//------------------------------------------------------------------------------------------------- Time

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
  return (time_ms_get() - start_time_ms) / tick_ms;
}

//------------------------------------------------------------------------------------------------- Threading

#if(VRTS_SWITCHING)

static struct {
  void (*handlers[VRTS_THREAD_LIMIT])(void);
  #if defined(_WIN32) || defined(_WIN64)
    HANDLE threads[VRTS_THREAD_LIMIT];
  #else
    pthread_t threads[VRTS_THREAD_LIMIT];
  #endif
  uint32_t count;
  volatile bool enabled;
} vrts;

#if defined(_WIN32) || defined(_WIN64)
static DWORD WINAPI vrts_wrapper(LPVOID param)
{
  void (*handler)(void) = (void (*)(void))param;
  handler();
  return 0;
}
#else
static void *vrts_wrapper(void *param)
{
  void (*handler)(void) = (void (*)(void))param;
  handler();
  return NULL;
}
#endif

bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  (void)stack; (void)size;
  if(vrts.count >= VRTS_THREAD_LIMIT) return false;
  vrts.handlers[vrts.count] = handler;
  #if defined(_WIN32) || defined(_WIN64)
    vrts.threads[vrts.count] = CreateThread(NULL, 0, vrts_wrapper, (LPVOID)handler, CREATE_SUSPENDED, NULL);
    if(!vrts.threads[vrts.count]) return false;
  #endif
  vrts.count++;
  return true;
}

void vrts_init(void)
{
  vrts.enabled = true;
  #if defined(_WIN32) || defined(_WIN64)
    for(uint32_t i = 0; i < vrts.count; i++) {
      ResumeThread(vrts.threads[i]);
    }
  #else
    for(uint32_t i = 0; i < vrts.count; i++) {
      pthread_create(&vrts.threads[i], NULL, vrts_wrapper, (void *)vrts.handlers[i]);
    }
  #endif
}

void vrts_lock(void) { vrts.enabled = false; }
bool vrts_unlock(void) { vrts.enabled = true; return true; }

void let(void)
{
  #if defined(_WIN32) || defined(_WIN64)
    Sleep(1);
  #else
    usleep(1000);
  #endif
}

uint8_t vrts_active_thread(void) { return 0; }

#else // !VRTS_SWITCHING

bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size)
{
  (void)handler; (void)stack; (void)size;
  return false;
}

void vrts_init(void) {}
void vrts_lock(void) {}
bool vrts_unlock(void) { return true; }
void let(void) {}
uint8_t vrts_active_thread(void) { return 0; }

#endif

//------------------------------------------------------------------------------------------------- Tick

uint64_t tick_keep(uint32_t offset_ms)
{
  return vrts_ticker_get() + ((offset_ms + tick_ms - 1) / tick_ms);
}

uint64_t tick_now(void) { return vrts_ticker_get(); }

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
  #if defined(_WIN32) || defined(_WIN64)
    Sleep(ms);
  #else
    usleep(ms * 1000);
  #endif
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
  uint64_t now = vrts_ticker_get();
  if(*tick > now) sleep((uint32_t)((*tick - now) * tick_ms));
  *tick = 0;
}

//------------------------------------------------------------------------------------------------- Init

bool systick_init(uint32_t systick_ms)
{
  if(!systick_ms) return false;
  tick_ms = systick_ms;
  start_time_ms = time_ms_get();
  return true;
}

//-------------------------------------------------------------------------------------------------