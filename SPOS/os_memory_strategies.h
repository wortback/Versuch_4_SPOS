/*
 * os_memory_strategies.h
 *
 * Created: 25/05/2023 16:49:21
 *  Author: Mariia Z & Okan D
 */ 





#ifndef _OS_MEMORY_STRATEGIES_H
#define _OS_MEMORY_STRATEGIES_H

#include "os_memheap_drivers.h"




//! First Fit strategy
MemAddr os_MemAlloc_FirstFit(Heap* heap, uint16_t size);

//! Next Fit strategy
MemAddr os_MemAlloc_NextFit(Heap* heap, uint16_t size);

//! Best Fit strategy
MemAddr os_MemAlloc_BestFit(Heap* heap, uint16_t size);

//! Worst Fit strategy
MemAddr os_MemAlloc_WorstFit(Heap* heap, uint16_t size);




#endif