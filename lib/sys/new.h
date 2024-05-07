#ifndef NEW_H_
#define NEW_H_

#include <stdlib.h>
#include <stdint.h>
#include "stm32g0xx.h"
#include "main.h"
#include "vrts.h"

//-------------------------------------------------------------------------------------------------

#ifndef NEW_COUNT_LIMIT
  #define NEW_COUNT_LIMIT 32 // limit of variables allocated by the 'new' function
#endif
#ifndef NEW_SIZE_LIMIT
  #define NEW_SIZE_LIMIT 8000 // total allocated memory limit for function 'new'
#endif
#ifndef ALOC_COUNT_LIMIT
  #define ALOC_COUNT_LIMIT 32 // limit of variables allocated by the 'new' function
#endif

//-------------------------------------------------------------------------------------------------

#if(VRTS_SWITCHING)
  void NEW_Init(uint8_t thread_nbr);
#endif
void *new_static(size_t size);
void *new(size_t size);
void clear();
void *aloc(size_t size);
void dloc(void **pointer);

typedef struct {
  void *var[NEW_COUNT_LIMIT];
  uint16_t count;
  uint16_t size;
} NEW_t;

//-------------------------------------------------------------------------------------------------

#endif
