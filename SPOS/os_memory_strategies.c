/*
 * os_memory_strategies.c
 *
 * Created: 25/05/2023 16:49:06
 *  Author: Mariia Z und Okan D
 */ 





#include "os_memory_strategies.h"
#include "defines.h"
#include "os_memory.h"



MemAddr os_MemAlloc_FirstFit(Heap* heap, uint16_t size){
	uint16_t allocAddr = 0;
	
	while(allocAddr < heap->sizeMap*2) {
		uint8_t nibVal = getNibbleVal(heap, allocAddr);
		
		// A free nibble found
		if(nibVal == 0){
			
			// Set the beginning of the block
			uint16_t start = allocAddr;
			
			// Allocate the space size-times
			while((allocAddr < heap->sizeMap*2) && (allocAddr - start < size)){
				// Move to the next address
				allocAddr++;
				
				// Stop if the nibble at the allocAddr is not free
				if(getNibbleVal(heap, allocAddr) != 0){
					break;
				}
			}
			
			// Check if enough space was allocated
			if(allocAddr - start >= size){
				//return getAddrForNibble(heap, start);
				return heap->firstUseAddr + start;
			}
			
			
		} else {
			allocAddr++;
		}
	}
	
	// Otherwise
	return 0;
}


MemAddr os_MemAlloc_NextFit(Heap* heap, uint16_t size){
	// Complete later
	return 0;
}


MemAddr os_MemAlloc_BestFit(Heap* heap, uint16_t size){
	// Complete later
	return 0;
}


MemAddr os_MemAlloc_WorstFit(Heap* heap, uint16_t size){
	// Complete later
	return 0;
}
