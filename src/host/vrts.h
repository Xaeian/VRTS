// src/host/vrts.h

#ifndef VRTS_H_
#define VRTS_H_

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

//------------------------------------------------------------------------------------------ Config

#ifndef VRTS_THREAD_LIMIT
  // Max number of threads
  #define VRTS_THREAD_LIMIT 12
#endif

#ifndef VRTS_SWITCHING
  // Enable cooperative thread switching
  #define VRTS_SWITCHING 1
#endif

//------------------------------------------------------------------------------------------ Macros

// Cast of a predicate for `timeout`
#define WAIT_ (bool (*)(void *))
// Time in milliseconds
#define seconds(s) (1000 * (s))
#define minutes(m) (60000 * (m))
// Yield until `flag` is `true`
#define wait_for(flag) while(!(flag)) let()

// OS threads carry their own stacks, the declaration keeps the target shape
#define stack(name, size) static uint32_t name[1]
#define thread(fnc, stack_name) vrts_thread(&fnc, (uint32_t *)stack_name, 0)

//-------------------------------------------------------------------------------------------- Tick

// Deadline `offset_ms` from now, and the current tick
uint64_t tick_keep(uint32_t offset_ms);
uint64_t tick_now(void);

/**
 * @brief Deadline passed, once; `*tick` is cleared to `0` on the trigger.
 * @param[in,out] tick Deadline from `tick_keep`
 * @return `true` once when the deadline passes
 */
bool tick_over(volatile uint64_t *tick);

/**
 * @brief Deadline still ahead; `*tick` is cleared to `0` when it passes.
 * @param[in,out] tick Deadline from `tick_keep`
 * @return `true` while waiting
 */
bool tick_away(volatile uint64_t *tick);

/**
 * @brief Time since a tick.
 * @param[in] tick Reference tick
 * @return Elapsed time [ms]
 */
int32_t tick_diff(uint64_t tick);

//------------------------------------------------------------------------------------------- Delay

// `delay` yields to the other threads, `sleep` holds the thread
void delay(uint32_t ms);
void sleep(uint32_t ms);

/**
 * @brief Yield until a predicate holds or the time runs out.
 * @note The predicate is read once more past the deadline,
 * so one met while another thread held the processor is no timeout.
 * @param[in] ms Time limit [ms]
 * @param[in] Free Predicate, `true` when the wait is over
 * @param[in] subject Argument of `Free`
 * @return `true` when the time ran out
 */
bool timeout(uint32_t ms, bool (*Free)(void *), void *subject);

// Until a deadline from `tick_keep`, cleared to `0` after; `delay` yields, `sleep` holds
void delay_until(uint64_t *tick);
void sleep_until(uint64_t *tick);

//----------------------------------------------------------------------------------------- Threads

/**
 * @brief Register a thread, the OS owns its stack.
 * @param[in] handler Thread function
 * @param[in] stack Ignored
 * @param[in] size Ignored
 * @return `true` when registered, `false` at the thread limit
 */
bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size);

// Hand the core to the next thread
void let(void);
#define yield let

//-------------------------------------------------------------------------------------------- Init

// Tick period, before `vrts_init`
bool systick_init(uint32_t systick_ms);

// Start every registered thread, does not return
void vrts_init(void);

// Thread switching off and on, `vrts_unlock` reports whether the scheduler runs
void vrts_lock(void);
bool vrts_unlock(void);

// Index of the running thread
uint8_t vrts_active_thread(void);

// Fatal scheduler error, an application definition replaces the default, and none may return
void vrts_panic(const char *msg) __attribute__((noreturn));

//-------------------------------------------------------------------------------------------------

// Tick counter. Under `VrtsVirtualTime` the program owns it
extern volatile uint64_t VrtsTicker;

// Clock source. `false` (default): ticks follow the wall clock.
// `true`: the program advances `VrtsTicker` itself,
// so a simulation runs deterministic virtual time
// as fast as the CPU allows instead of in real seconds
extern bool VrtsVirtualTime;

//-------------------------------------------------------------------------------------------------
#endif
