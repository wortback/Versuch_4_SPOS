/*
 * os_memory.h
 *
 * Created: 25/05/2023 17:45:19
 *  Author: Mariia Z
 */ 





#ifndef _OS_MEMORY_H
#define _OS_MEMORY_H

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"



//! Allocates memory in the heap
MemAddr os_malloc(Heap *heap, size_t size);

//! Reallocates memory in the heap
MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size);

//! Garbage collector for the heap cleanup 
void os_freeProcessMemory(Heap* heap, ProcessID pid);

//! Frees the allocated memory 
void os_free(Heap *heap, MemAddr addr);

//! Heap-map size getter
size_t os_getMapSize(Heap const *heap);

//! Usable heap size getter
size_t os_getUseSize(Heap const *heap);

//! Heap-map start getter
MemAddr os_getMapStart(Heap const *heap);

//! Usable heap start getter
MemAddr os_getUseStart(Heap const *heap);

//! Allocation strategy setter
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

//! Allocation strategy getter
AllocStrategy os_getAllocationStrategy(Heap const *heap);

uint8_t getNibbleVal(Heap const* heap, uint8_t nibbleaddr);

//! Frees the chunk only if the owner
void os_freeAsOwner(Heap *heap, MemAddr addr, ProcessID owner);

//! Get the value of a map entry (used by allocation strategies)
MemValue os_getMapEntry(Heap const *heap, MemAddr addr);

//! Chunk size at location getter
uint16_t os_getChunkSize(Heap const *heap, MemAddr addr);

//! Get nibble on the address
uint8_t getNibble(Heap const* heap, MemAddr addr);


#endif