#ifndef VRTS_H
#define VRTS_H

#include "main.h" // for overwriting defines
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32g0xx.h"

#ifndef VRTS_TASK_LIMIT
  #define VRTS_TASK_LIMIT 12
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
