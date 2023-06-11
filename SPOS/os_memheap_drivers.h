/*
 * os_memheap_drivers.h
 *
 * Created: 25/05/2023 16:46:47
 *  Author: Mariia Z
 */ 


#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"

typedef uint16_t MemAddr;
typedef uint8_t MemValue;

//! Heap allocation strategies
typedef enum {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;

//! Heap driver
typedef struct {
	// Pointer to the driver associated with the heap
	MemDriver *driver;
	
	// The first map address and the size of the map
	MemAddr firstMapAddr;
	size_t sizeMap;
	
	// The first use area address and the size of the area
	MemAddr firstUseAddr;
	size_t sizeUse;
	
	// Current allocation strategy 
	AllocStrategy currAllocStrat;
	
	// Pointer to the name of the heap
	const char *name;
	
	// For Next Fit strategy
	MemAddr nextFitAddrLast;
	
	uint16_t firstNibble[MAX_NUMBER_OF_PROCESSES];
	uint16_t lastNibble[MAX_NUMBER_OF_PROCESSES];
} Heap;

//! Internal Heap 
extern Heap intHeap__;

//! External Heap
extern Heap extHeap__;

//! Pointer to the Heap intHeap__
#define intHeap (&intHeap__)

//! Pointer to the Heap extHeap__
#define extHeap (&extHeap__)

void os_initHeaps();

size_t os_getHeapListLength(void);

Heap* os_lookupHeap(uint8_t index);


#endif