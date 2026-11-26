// src/heap.c

#include "heap.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "vrts.h"

//--------------------------------------------------------------------------------------- Allocator

// Heap size rounded up to whole alignment units
#define HEAP_SIZE_ALIGNED ((HEAP_SIZE + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1))

// Block header, the payload follows it. The header size decides the payload alignment,
// so it is padded to `HEAP_ALIGN`: on a 32-bit target the fields alone take 12 bytes
// and would hand out 4-aligned memory
typedef struct heap_block {
  size_t size;              // payload size [B]
  struct heap_block *next;  // next block in address order
  bool free;
} __attribute__((aligned(HEAP_ALIGN))) heap_block_t;

_Static_assert(sizeof(heap_block_t) % HEAP_ALIGN == 0, "header breaks payload alignment");

static uint8_t heap[HEAP_SIZE_ALIGNED] __attribute__((aligned(HEAP_ALIGN)));
static heap_block_t *const first = (heap_block_t *)heap;

static inline heap_block_t *block_of(void *ptr)
{
  return (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
}

// Absorb the free neighbour above `block`
static void merge_next(heap_block_t *block)
{
  heap_block_t *next = block->next;
  if(next && next->free) {
    block->size += sizeof(heap_block_t) + next->size;
    block->next = next->next;
  }
}

void heap_init(void)
{
  first->size = HEAP_SIZE_ALIGNED - sizeof(heap_block_t);
  first->next = NULL;
  first->free = true;
}

void *heap_alloc(size_t size)
{
  size = (size + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1);
  for(heap_block_t *block = first; block; block = block->next) {
    // Blocks sit in address order, so neighbours freed at different moments coalesce here
    while(block->free && block->next && block->next->free) merge_next(block);
    if(!block->free || block->size < size) continue;
    if(block->size > size + sizeof(heap_block_t)) {
      // Split: the remainder becomes a free block right after this one
      heap_block_t *rest = (heap_block_t *)((uint8_t *)block + sizeof(heap_block_t) + size);
      rest->size = block->size - size - sizeof(heap_block_t);
      rest->free = true;
      rest->next = block->next;
      block->next = rest;
      block->size = size;
    }
    block->free = false;
    return (uint8_t *)block + sizeof(heap_block_t);
  }
  vrts_panic("Heap allocation failed");
}

void heap_free(void *ptr)
{
  if(!ptr) return;
  heap_block_t *block = block_of(ptr);
  block->free = true;
  merge_next(block);
}

void *heap_reloc(void *ptr, size_t size)
{
  size = (size + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1);
  if(!ptr) return heap_alloc(size);
  if(size == 0) {
    heap_free(ptr);
    return NULL;
  }
  heap_block_t *block = block_of(ptr);
  if(block->size >= size) return ptr;
  void *moved = heap_alloc(size);
  memcpy(moved, ptr, block->size);
  heap_free(ptr);
  return moved;
}

//------------------------------------------------------------------------------- Garbage collector

// Pointers a thread took with `heap_new`, one list per thread
typedef struct {
  void **var;
  uint16_t count;
  uint16_t limit; // capacity of `var`, grows by `HEAP_NEW_BLOCK`
} heap_list_t;

static heap_list_t *lists[VRTS_SWITCHING ? VRTS_THREAD_LIMIT : 1];

// List of the calling thread, created on first use
static heap_list_t *thread_list(void)
{
  uint8_t thread = vrts_active_thread();
  heap_list_t *list = lists[thread];
  if(list) return list;
  list = heap_alloc(sizeof(heap_list_t));
  list->var = heap_alloc(sizeof(void *) * HEAP_NEW_BLOCK);
  list->count = 0;
  list->limit = HEAP_NEW_BLOCK;
  lists[thread] = list;
  return list;
}

void *heap_new(size_t size)
{
  if(!size) return NULL;
  heap_list_t *list = thread_list();
  if(list->count >= list->limit) {
    uint16_t limit = list->limit + HEAP_NEW_BLOCK;
    list->var = heap_reloc(list->var, sizeof(void *) * limit);
    list->limit = limit;
  }
  void *ptr = heap_alloc(size);
  list->var[list->count++] = ptr;
  return ptr;
}

void heap_clear(void)
{
  heap_list_t *list = lists[vrts_active_thread()];
  if(!list) return;
  for(uint16_t i = 0; i < list->count; i++) heap_free(list->var[i]);
  list->count = 0;
}

//-------------------------------------------------------------------------------------------------
