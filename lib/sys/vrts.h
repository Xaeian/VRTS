#ifndef VRTS_H_
#define VRTS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32g0xx.h"
#include "main.h"

#ifndef VRTS_THREAD_LIMIT
  #define VRTS_THREAD_LIMIT 12
#endif

#ifndef VRTS_SWITCHING
  #define VRTS_SWITCHING 1
#endif

#define WAIT_ (bool (*)(void *))

#define seconds(ms)  (1000 * ms)
#define minutes(min) (60 * 1000 * min)

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
uint64_t gettick(uint32_t offset_ms);
void delay_until(uint64_t *tick);
void sleep_until(uint64_t *tick);
bool waitfor(uint64_t *tick);
int32_t watch(uint64_t tick);

void VRTS_Init(void);
void VRTS_Lock(void);
bool VRTS_Unlock(void);
uint8_t VRTS_ActiveThread(void);
bool SYSTICK_Init(uint32_t systick_ms);

#endif
