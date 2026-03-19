// lib/col/heap.h

#ifndef HEAP_H_
#define HEAP_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"

//------------------------------------------------------------------------------------------------- Config

#ifndef HEAP_SIZE
  // Heap memory region size in bytes
  #define HEAP_SIZE 8192
#endif

#ifndef HEAP_ALIGN
  // Allocation alignment in bytes
  #define HEAP_ALIGN 8
#endif

//------------------------------------------------------------------------------------------------- Allocator

/**
 * @brief Heap memory block header.
 * @param size Size of data area in bytes (excluding header)
 * @param next Pointer to next block in `FreeList`
 * @param free `true` if free, `false` if allocated
 */
typedef struct heap_block {
  size_t size;
  struct heap_block *next;
  bool free;
} heap_block_t;

// Initialize heap — call once before heap_alloc()
void heap_init(void);

/**
 * @brief Allocate a memory block from the heap.
 * @param[in] size Number of bytes to allocate
 * @return Pointer to allocated memory, or `NULL` if unavailable
 */
void *heap_alloc(size_t size);

/**
 * @brief Reallocate (resize) an existing heap block.
 * @param[in] ptr Pointer returned by `heap_alloc()`, or `NULL`
 * @param[in] size New size in bytes (0 = free)
 * @return Pointer to resized block, or `NULL` if allocation failed
 */
void *heap_reloc(void *ptr, size_t size);

/**
 * @brief Free a previously allocated memory block.
 * @param[in] ptr Pointer returned by `heap_alloc()`, or `NULL`
 */
void heap_free(void *ptr);

//------------------------------------------------------------------------------------------------- Garbage-collector

#ifndef HEAP_NEW_BLOCK
  // Initial and growth step for GC stack capacity
  #define HEAP_NEW_BLOCK 16
#endif

/**
 * @brief Memory stack for garbage collector, one per thread.
 * @param var Pointers to dynamically allocated variables
 * @param count Current number of variables in use
 * @param limit Current allocated capacity of `var` (auto-expands)
 */
typedef struct {
  void **var;
  uint16_t count;
  uint16_t limit;
} heap_new_t;

/**
 * @brief Allocate memory tracked by the garbage collector.
 * @param[in] size Number of bytes to allocate
 * @return Pointer to allocated memory, or `NULL` if allocation failed
 */
void *heap_new(size_t size);

// Free all GC-tracked memory for the active thread
void heap_clear(void);

//-------------------------------------------------------------------------------------------------
#endif