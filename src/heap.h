// src/heap.h

#ifndef HEAP_H_
#define HEAP_H_

#include <stddef.h>
#include <string.h>
#include "main.h"

//------------------------------------------------------------------------------------------ Config

#ifndef HEAP_SIZE
  // Heap memory region [B]
  #define HEAP_SIZE 8192
#endif

#ifndef HEAP_ALIGN
  // Allocation alignment [B]
  #define HEAP_ALIGN 8
#endif

#ifndef HEAP_NEW_BLOCK
  // Initial capacity and growth step of a thread's garbage collector list [pointers]
  #define HEAP_NEW_BLOCK 16
#endif

//--------------------------------------------------------------------------------------- Allocator

// Initialize heap, call once before `heap_alloc`
void heap_init(void);

/**
 * @brief Allocate a block. A full heap is a `vrts_panic`, never a `NULL`.
 * @param[in] size Number of bytes
 * @return Pointer to the block, aligned to `HEAP_ALIGN`
 */
void *heap_alloc(size_t size);

/**
 * @brief Resize a block, the content is kept.
 * @param[in] ptr Pointer from `heap_alloc`, `NULL` allocates
 * @param[in] size New size [B], `0` frees
 * @return Pointer to the resized block, `NULL` after a free
 */
void *heap_reloc(void *ptr, size_t size);

/**
 * @brief Release a block.
 * @param[in] ptr Pointer from `heap_alloc`, `NULL` is ignored
 */
void heap_free(void *ptr);

//------------------------------------------------------------------------------- Garbage collector

/**
 * @brief Allocate a block owned by the calling thread's garbage collector.
 *   Every block taken this way goes back with the next `heap_clear` of that thread.
 * @param[in] size Number of bytes, `0` returns `NULL`
 * @return Pointer to the block, a full heap is a `vrts_panic` as in `heap_alloc`
 */
void *heap_new(size_t size);

// Release every `heap_new` block of the calling thread
void heap_clear(void);

//-------------------------------------------------------------------------------------- Attributes

// A block of `heap_alloc` goes back through `heap_free` or `heap_reloc`, which `-Wall`
// and `-fanalyzer` then hold every caller to; each names the others, so they come last.
// gcc 11 is the first to take a deallocator in `malloc`, an older compiler goes without.
#if defined(__GNUC__) && __GNUC__ >= 11
void *heap_alloc(size_t size)
  __attribute__((malloc(heap_free, 1), malloc(heap_reloc, 1), returns_nonnull));
void *heap_reloc(void *ptr, size_t size)
  __attribute__((malloc(heap_free, 1), malloc(heap_reloc, 1)));
#endif

//-------------------------------------------------------------------------------------------------
#endif
