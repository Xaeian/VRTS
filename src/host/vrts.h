// hal/host/vrts.h

#ifndef VRTS_H_
#define VRTS_H_

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------------------------- Config

#ifndef VRTS_THREAD_LIMIT
  // Max number of threads
  #define VRTS_THREAD_LIMIT 12
#endif

#ifndef VRTS_SWITCHING
  // Enable cooperative thread switching
  #define VRTS_SWITCHING 1
#endif

//------------------------------------------------------------------------------------------------- Macros

// Type cast for timeout function
#define WAIT_ (bool (*)(void *))
// Convert seconds to milliseconds
#define seconds(s) (1000 * (s))
// Convert minutes to milliseconds
#define minutes(m) (60000 * (m))
// Wait until `flag` is `true`
#define wait_for(flag) while(!(flag)) let()

// Stub stack declaration (host uses OS threads, no stack needed)
#define stack(name, size) static uint32_t name[1]
// Registers a thread (stack ignored on host)
#define thread(fnc, stack_name) vrts_thread(&fnc, (uint32_t *)stack_name, 0)

//------------------------------------------------------------------------------------------------- Tick

// Sets a deadline at current time + offset, returns tick value
uint64_t tick_keep(uint32_t offset_ms);

// Returns current system tick
uint64_t tick_now(void);

/**
 * @brief One-shot expired check — auto-resets `*tick` to 0 on trigger
 * @param[in,out] tick Pointer to deadline set by `tick_keep()`
 * @return `true` once when deadline passes, `false` otherwise
 */
bool tick_over(uint64_t *tick);

/**
 * @brief Continuous pending check — auto-resets `*tick` to 0 on expiry
 * @param[in,out] tick Pointer to deadline set by `tick_keep()`
 * @return `true` while waiting, `false` once expired
 */
bool tick_away(uint64_t *tick);

/**
 * @brief Elapsed time since reference tick
 * @param[in] tick Reference tick to measure from
 * @return Elapsed time in milliseconds
 */
int32_t tick_diff(uint64_t tick);

//------------------------------------------------------------------------------------------------- Delay

// Delays for `ms` milliseconds, yields to other threads
void delay(uint32_t ms);

// Sleeps for `ms` milliseconds, blocks thread switching
void sleep(uint32_t ms);

/**
 * @brief Polls condition until met or timeout expires
 * @param[in] ms Timeout in milliseconds
 * @param[in] Free Callback returning `true` when condition is met
 * @param[in] subject Pointer passed to `Free`
 * @return `true` if timed out, `false` if condition met
 */
bool timeout(uint32_t ms, bool (*Free)(void *), void *subject);

// Delays until tick reached, yields to other threads. Resets `*tick` to 0
void delay_until(uint64_t *tick);

// Sleeps until tick reached, blocks thread switching. Resets `*tick` to 0
void sleep_until(uint64_t *tick);

//------------------------------------------------------------------------------------------------- Threads

/**
 * @brief Registers a new thread
 * @param[in] handler Thread function
 * @param[in] stack Ignored on host (OS manages stack)
 * @param[in] size Ignored on host
 * @return `true` on success, `false` if thread limit reached
 */
bool vrts_thread(void (*handler)(void), uint32_t *stack, uint16_t size);

// Yields control to the next thread
void let(void);

// Alias for `let()`
#define yield let

//------------------------------------------------------------------------------------------------- Init

// Initializes SysTick — call before `vrts_init()`
bool systick_init(uint32_t systick_ms);

// Initializes VRTS and starts all threads
void vrts_init(void);

// Disables thread switching
void vrts_lock(void);

// Enables thread switching if VRTS is initialized
bool vrts_unlock(void);

// Returns index of currently active thread (always 0 on host)
uint8_t vrts_active_thread(void);

// Called on fatal VRTS error — weak, override to handle panics
void vrts_panic(const char *msg);

//-------------------------------------------------------------------------------------------------

extern volatile uint64_t VrtsTicker;

#endif