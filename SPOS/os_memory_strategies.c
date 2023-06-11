/*
 * os_memory_strategies.c
 *
 * Created: 25/05/2023 16:49:06
 *  Author: Mariia Z und Okan D
 */ 


#include "os_memory_strategies.h"
#include "defines.h"
#include "os_memory.h"


MemAddr fitFromAddr(Heap* heap, uint16_t size, uint16_t startAddr){
	uint16_t allocAddr = startAddr;
	
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
	return 0;
}


MemAddr os_MemAlloc_FirstFit(Heap* heap, uint16_t size){
	return fitFromAddr(heap, size, 0);
}


MemAddr os_MemAlloc_NextFit(Heap* heap, uint16_t size){
	MemAddr lastCached = heap->nextFitAddrLast;
	
	MemAddr addr = fitFromAddr(heap, size, lastCached);
	
	if(addr == 0){
		addr = fitFromAddr(heap, size, 0);
	}
	if(addr != 0){
		heap->nextFitAddrLast = (addr - heap->firstUseAddr);
	}
	return addr;
}


MemAddr os_MemAlloc_BestFit(Heap* heap, uint16_t size){
	MemAddr currFound = 0;
	MemAddr minFound = 0;
	uint16_t minSize = 0xffff;
	
	for(MemAddr addr_i = 0; (addr_i < heap->sizeUse); addr_i++){
		currFound = addr_i;
		MemAddr temp;
		
		if((temp = getNibble(heap, addr_i)) == 0){
			do{
				addr_i++;
			} while ((temp = getNibble(heap, addr_i)) == 0 && addr_i < heap->sizeUse);
			
			uint16_t length = addr_i - currFound;
			
			if(length >= size && length < minSize){
				minSize = length;
				minFound = currFound;
			}
			addr_i--;
		}
	}
	
	if(minSize != 0xffff){
		return heap->firstUseAddr + minFound;
	}
	return 0;
}


MemAddr os_MemAlloc_WorstFit(Heap* heap, uint16_t size){
	MemAddr	foundAdr = 0;
	MemAddr	foundNewAdr	= 0;
	size_t curLargestSize = 0;
	size_t newSize = 0;
	MemValue nibbles = 0;
	
	
	if (size == 0 || size > heap->sizeUse) {
		return foundAdr;
	}
	
	for(MemAddr iAddr =  0; iAddr < heap->sizeMap; iAddr++) {
		nibbles = os_getMapEntry(heap, heap->firstMapAddr+iAddr);
		for (int i = 0; i <= 1; i++) {
			MemValue nibble = (nibbles&(0xf0>>(i*4))) >> ((1-i)*4);
			if (nibble == 0) {
				if (foundNewAdr == 0) {
					foundNewAdr = heap->firstUseAddr + iAddr*2+(i);
				}
				newSize++;
				} else if(newSize > curLargestSize && newSize >= size) {
				foundAdr = foundNewAdr;
				curLargestSize = newSize;
				foundNewAdr = 0;
				newSize = 0;
				} else {
				foundNewAdr = 0;
				newSize = 0;
			}
		}
	}
	
	if(newSize > curLargestSize && newSize >= size) {
		foundAdr = foundNewAdr;
	}
	return foundAdr;
}
