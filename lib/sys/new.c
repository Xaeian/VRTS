#include "new.h"

#if(VRTS_SWITCHING)
  NEW_t *new_stacks[VRTS_THREAD_LIMIT];
#else
  NEW_t new_stack;
#endif
uint16_t new_size;

void *aloc_var[ALOC_COUNT_LIMIT];
uint16_t aloc_size[ALOC_COUNT_LIMIT];

//-------------------------------------------------------------------------------------------------

void *new_static(size_t size)
{
  new_size += size;
  return malloc(size);
}

#if(VRTS_SWITCHING)
void NEW_Init(uint8_t thread_nbr)
{
  NEW_t *new_stack = new_static(sizeof(NEW_t));
  new_stack->count = 0;
  new_stack->size = 0;
  new_stacks[thread_nbr] = new_stack;
}
#endif

static void MEM_ErrorHandler(void)
{
  __disable_irq();
  volatile uint32_t i = 0;
  while(1) i++;
}

void *new(size_t size)
{
  void *pointer = NULL;
  if(!size) return pointer;
  #if(VRTS_SWITCHING)
    uint8_t active_thread = VRTS_ActiveThread();
    NEW_t *stack = new_stacks[active_thread];
  #else
    NEW_t *stack = &new_stack;
  #endif
  if(stack->count >= NEW_COUNT_LIMIT) MEM_ErrorHandler();
  if((new_size + size) >= NEW_SIZE_LIMIT) MEM_ErrorHandler();
  pointer = malloc(size);
  stack->var[stack->count] = pointer;
  stack->count++;
  stack->size += size;
  new_size += size;
  return pointer;
}

void clear()
{
  #if(VRTS_SWITCHING)
    uint8_t active_thread = VRTS_ActiveThread();
    NEW_t *stack = new_stacks[active_thread];
  #else
    NEW_t *stack = &new_stack;
  #endif
  if(stack->count) {
    for(int i = 0; i < stack->count; i++)
      free(stack->var[i]);
    stack->count = 0;
    new_size -= stack->size;
    stack->size = 0;
  }
}

void *aloc(size_t size)
{
  uint16_t i = 0;
  void *pointer = 0;
  if(!size) return pointer;
  if((new_size + size) >= NEW_SIZE_LIMIT) MEM_ErrorHandler();
  while(aloc_var[i]) {
    i++;
    if(i >= ALOC_COUNT_LIMIT) MEM_ErrorHandler();
  }
  pointer = malloc(size);
  aloc_var[i] = pointer;
  aloc_size[i] = size;
  new_size += size;
  return pointer;
}

void dloc(void **pointer)
{
  if(!pointer) return;
  uint16_t i = 0;
  while(i < ALOC_COUNT_LIMIT) {
    if(*pointer == aloc_var[i]) {
      free(*pointer);
      *pointer = NULL;
      new_size -= aloc_size[i];
      aloc_size[i] = 0;
      aloc_var[i] = NULL;
      return;
    }
    i++;
  }
  MEM_ErrorHandler();
  return;
}

//-------------------------------------------------------------------------------------------------
