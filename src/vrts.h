// src/vrts.h

#ifndef VRTS_H_
#define VRTS_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#if defined(STM32G0)
  #include <stm32g0xx.h>
#elif defined(STM32WB)
  #include <stm32wbxx.h>
#elif defined(STM32G4)
  #include <stm32g4xx.h>
#endif
#include "main.h"

//------------------------------------------------------------------------------------------ Config

#ifndef VRTS_THREAD_LIMIT
  // Max number of threads
  #define VRTS_THREAD_LIMIT 12
#endif

#ifndef VRTS_SWITCHING
  // Cooperative thread switching, off makes `let` a plain `__WFI`
  #define VRTS_SWITCHING 1
#endif

#ifndef VRTS_THREAD_TIMEOUT_MS
  // Longest hold of the core by one thread before `vrts_panic` [ms], `0` = off
  #define VRTS_THREAD_TIMEOUT_MS 2000
#endif

#ifndef VRTS_TICKLESS
  // Idle without the system tick, timed by `PWR_Idle` of the platform up to the nearest wait.
  // A `timeout` condition no interrupt sets, such as a pin, is then seen only at its deadline.
  #define VRTS_TICKLESS 0
#endif

//------------------------------------------------------------------------------------------ Macros

// Cast a `bool (*)(MODULE_t *)` query to the `timeout` callback signature
#define WAIT_ (bool (*)(void *))(void (*)(void))
// Convert seconds or minutes to milliseconds
#define seconds(s) (1000 * (s))
#define minutes(m) (60000 * (m))
// Yield until `flag` is `true`, the thread taking the core on every round to read it
#define wait_for(flag) while(!(flag)) let()

// Declare an 8-byte aligned stack of `size` words
#define stack(name, size) \
  static uint32_t name[8 * (((size) + 7) / 8)] __attribute__((aligned(8)))

// Register a thread on a stack declared with `stack`
#define thread(fnc, stack_name) \
  vrts_thread(&fnc, (uint32_t *)stack_name, sizeof(stack_name) / sizeof(*(stack_name)))

// Alias for `let`
#define yield let

//------------------------------------------------------------------------------------------- Types

/**
 * @brief One thread of the scheduler.
 *   A thread inside `delay`, `delay_until` or `timeout` waits for a tick or a condition,
 *   which `let` of the others checks without switching to it.
 * @param stack Saved stack pointer, top of the context frame, first for `PendSV_Handler`
 * @param handler Thread entry
 * @param limit Bottom word of the stack, holding a canary `let` checks
 * @param wake Tick the thread waits for, `0` = ready
 * @param Free Condition that ends the wait before `wake`, `NULL` = none
 * @param subject Argument of `Free`
 */
typedef struct {
  volatile uint32_t stack;
  void (*handler)(void);
  uint32_t *limit;
  uint64_t wake;
  bool (*Free)(void *);
  void *subject;
} VRTS_Task_t;

//----------------------------------------------------------------------------------------- Globals

// System tick counter, one step per `systick_init` period
extern volatile uint64_t VrtsTicker;

//-------------------------------------------------------------------------------------------- Tick

// Deadline `offset_ms` from now, a tick value for the checks below
uint64_t tick_keep(uint32_t offset_ms);

// Current system tick
uint64_t tick_now(void);

/**
 * @brief One-shot expiry check, `*tick` resets to `0` when it fires.
 * @param[in,out] tick Deadline from `tick_keep`, `0` = disarmed
 * @return `true` once when the deadline passes, `false` otherwise
 */
bool tick_over(volatile uint64_t *tick);

/**
 * @brief Pending check, `*tick` resets to `0` on expiry.
 * @param[in,out] tick Deadline from `tick_keep`, `0` = disarmed
 * @return `true` while waiting, `false` once expired or disarmed
 */
bool tick_away(volatile uint64_t *tick);

/**
 * @brief Time since a reference tick.
 * @param[in] tick Reference tick
 * @return Elapsed time [ms], negative for a deadline still ahead
 */
int32_t tick_diff(uint64_t tick);

//------------------------------------------------------------------------------------------- Delay

// Wait `ms` yielding to other threads, or blocking with `__WFI`
void delay(uint32_t ms);
void sleep(uint32_t ms);

/**
 * @brief Yield until the condition holds or `ms` pass.
 * @note The condition is read once more past the deadline,
 * so one met while another thread held the processor is no timeout.
 * `let` of other threads reads it too, and its first `true` ends the wait;
 * with all threads waiting, one no interrupt sets, such as a pin, shows at the next tick.
 * @param[in] ms Timeout [ms]
 * @param[in] Free Callback returning `true` when the condition is met
 * @param[in] subject Pointer passed to `Free`
 * @return `true` on timeout, `false` when the condition was met
 */
bool timeout(uint32_t ms, bool (*Free)(void *), void *subject);

// Wait for the deadline yielding or blocking, `*tick` resets to `0` after
void delay_until(uint64_t *tick);
void sleep_until(uint64_t *tick);

//----------------------------------------------------------------------------------------- Threads

/**
 * @brief Register a thread.
 * @param[in] handler Thread function
 * @param[in] stack Stack memory
 * @param[in] size Stack size in 32-bit words, at least `80` on M0+ and `128` on M4
 * @return `true` on success, `false` when the thread limit is reached
 */
bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size);

// Hand the core to the next thread ready in turn, the caller itself when no other is;
// with none ready the core sleeps until an interrupt, the system tick among them.
void let(void);

// Start the system tick, call before `vrts_init`
bool systick_init(uint32_t systick_ms);

// Enter the first thread, never returns
void vrts_init(void);

// Stop and resume thread switching, resume reports `false` before `vrts_init`
void vrts_lock(void);
bool vrts_unlock(void);

// Whether the idle of `VRTS_TICKLESS` may take Stop 1, asked with interrupts masked;
// weak, `false` by default. Timers, DMA and the ADC stop with the clock.
bool vrts_idle_stop(void);

// Index of the running thread, `0` with switching off
uint8_t vrts_active_thread(void);

// Fatal scheduler error, weak: the default masks interrupts and halts, and none may return
void vrts_panic(const char *msg) __attribute__((noreturn));

//-------------------------------------------------------------------------------------------------
#endif
