// lib/col/heap.c

#include "heap.h"
#include "vrts.h"

//------------------------------------------------------------------------------------------------- Allocator

static uint8_t Heap[((HEAP_SIZE + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1))] __attribute__((aligned(HEAP_ALIGN))); // Heap memory region
static heap_block_t *FreeList = (heap_block_t*)Heap; // Pointer to the first block in the free list

void heap_init(void)
{
  FreeList->size = ((HEAP_SIZE + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1)) - sizeof(heap_block_t); // One free block covers the whole heap
  FreeList->next = NULL; // No next block
  FreeList->free = true; // Mark the block as free
}

void *heap_alloc(size_t size)
{
  size = (size + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1);
  heap_block_t *curr = FreeList; // Start from the first free block
  // Traverse the free list to find a block large enough
  while(curr) {
    if(curr->free && curr->size >= size) {
      // If the block is bigger than needed, split it
      if(curr->size > size + sizeof(heap_block_t)) {
        heap_block_t *new_block = (heap_block_t*)((uint8_t*)curr + sizeof(heap_block_t) + size);
        new_block->size = curr->size - size - sizeof(heap_block_t); // Size of the remaining free space
        new_block->free = true; // Mark the new block as free
        new_block->next = curr->next; // Link new block with rest of list
        curr->next = new_block; // Insert the new block right after current
        curr->size = size; // Shrink current block to the requested size
      }
      curr->free = 0; // Mark current block as allocated
      return (uint8_t*)curr + sizeof(heap_block_t); // Return pointer just after the block header
    }
    curr = curr->next; // Move to the next block if this one isn't suitable
  }
  vrts_panic("Heap allocation failed"); // Not return
  return NULL;
}

void heap_free(void *ptr)
{
  if(!ptr) return; // Nothing to free if pointer is NULL
  heap_block_t *curr = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t)); // Get the block header
  curr->free = 1; // Mark this block as free
  // Coalesce with next block if it's free
  heap_block_t *next = curr->next;
  if(next && next->free) {
    curr->size += sizeof(heap_block_t) + next->size; // Merge sizes including the header
    curr->next = next->next; // Remove next block from list
  }
}

void *heap_reloc(void *ptr, size_t size)
{
  size = (size + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1);
  if(!ptr) return heap_alloc(size); // Behaves like malloc
  if(size == 0) { // Behaves like free
    heap_free(ptr);
    return NULL;
  }
  heap_block_t *curr = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
  if(curr->size >= size) return ptr; // Current block already big enough
  void *new_ptr = heap_alloc(size); // Allocate new block
  if(!new_ptr) return NULL;
  memcpy(new_ptr, ptr, curr->size); // Copy data from old block to new one
  heap_free(ptr); // Free old block
  return new_ptr;
}

//------------------------------------------------------------------------------------------------- Garbage-collector

// Stacks, one for each thread for multi-threading mode or single stack for single-threaded mode
heap_new_t *Stacks[VRTS_SWITCHING ? VRTS_THREAD_LIMIT : 1];

static heap_new_t *heap_get_stack(void)
{
  uint8_t active_thread = vrts_active_thread();
  heap_new_t *stack = Stacks[active_thread];
  if(stack) return stack;
  stack = heap_alloc(sizeof(heap_new_t));
  stack->var = heap_alloc(sizeof(void*) * HEAP_NEW_BLOCK);
  stack->count = 0;
  stack->limit = HEAP_NEW_BLOCK;
  Stacks[active_thread] = stack;
  return stack;
}

void *heap_new(size_t size)
{
  if(!size) return NULL;
  heap_new_t *stack = heap_get_stack();
  // Expand stack->var if no space left
  if(stack->count >= stack->limit) {
    uint16_t new_limit = stack->limit + HEAP_NEW_BLOCK;
    void **new_var = heap_reloc(stack->var, sizeof(void*) * new_limit);
    if(!new_var) return NULL;
    stack->var = new_var;
    stack->limit = new_limit;
  }
  void *pointer = heap_alloc(size);
  if(!pointer) return NULL;
  stack->var[stack->count++] = pointer;
  return pointer;
}

void heap_clear(void)
{
  uint8_t active_thread = vrts_active_thread();
  heap_new_t *stack = Stacks[active_thread];
  if(!stack) return;
  for(uint16_t i = 0; i < stack->count; i++) heap_free(stack->var[i]);
  stack->count = 0;
}

//-------------------------------------------------------------------------------------------------