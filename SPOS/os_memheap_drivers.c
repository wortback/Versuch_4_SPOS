/*
 * os_memheap_drivers.c
 *
 * Created: 25/05/2023 16:46:59
 *  Author: Mariia Z und Okan D
 */ 


#include "os_memheap_drivers.h"
#include "os_memory_strategies.h"
#include "defines.h"
#include "os_mem_drivers.h"

//! Internal Heap
Heap intHeap__;

//! External Heap
Heap extHeap__;

//! An array to store current heaps
Heap* heaps[2];


//! Heap initialization
void os_initHeaps() {
	
	// Initialize the heaps array
	heaps[0] = intHeap;
	heaps[1] = extHeap;
	
	//	INTERNAL HEAP INITIALIZATION-------------------------------------------------------------------------------------------------------------
	
	// Calculate the size of the heap. It is the difference between the bottom of the process stack (for the maximum number of processes)
	// and the starting address of the AVR_SRAM, offset by HEAPOFFSET.
	size_t heapSize = PROCESS_STACK_BOTTOM(MAX_NUMBER_OF_PROCESSES) - AVR_SRAM_START - HEAPOFFSET;

	// Setting the driver for the internal heap to the internal SRAM.
	intHeap__.driver = intSRAM;

	// The starting address of the heap map is set to the starting address of the internal SRAM plus the offset.
	intHeap__.firstMapAddr = intSRAM__.firstAddr + HEAPOFFSET;

	// The size of the heap map is set to one-third of the total heap size.
	intHeap__.sizeMap = (heapSize / 3);

	// The starting address for the actual usable heap memory (after the heap map) is set to the starting address of the heap map plus its size.
	intHeap__.firstUseAddr = intHeap__.firstMapAddr + intHeap__.sizeMap;

	// The size of the usable heap memory is set to be two-thirds of the total heap size.
	intHeap__.sizeUse = 2 * intHeap__.sizeMap;

	// The current allocation strategy for the internal heap is set to first-fit.
	intHeap__.currAllocStrat = OS_MEM_FIRST;
	
	intHeap__.nextFitAddrLast = 0;

	// The name of the heap is set to "internal".
	intHeap__.name = "internal";
	
	//heaps[0]->driver->init();

	// All the nibbles of the heap (managed by the driver) are set to 0x00, marking them as free memory blocks.
	// This is done by iterating over the size of the heap map and writing 0x00 to each location.
	for(int i = 0; i < intHeap__.sizeMap; ++i) {
		intHeap__.driver->write(intHeap__.firstMapAddr + i, 0x00);
	}
	
	
	//	EXTERNAL HEAP INITIALIZATION-------------------------------------------------------------------------------------------------------------

	extHeap__.driver = extSRAM;
	extHeap__.firstMapAddr = 0;
	extHeap__.sizeMap = 0xFFFF / 3;
	extHeap__.firstUseAddr = extHeap__.firstMapAddr + extHeap__.sizeMap;
	extHeap__.sizeUse = 2 * extHeap__.sizeMap;
	extHeap__.currAllocStrat = OS_MEM_FIRST;
	extHeap__.nextFitAddrLast = 0;
	extHeap__.name = "external";

	heaps[1]->driver->init();

	for(int i = 0; i < extHeap__.sizeMap; i++) {
		extHeap__.driver->write(extHeap__.firstMapAddr + i, 0x00);
	}
}

//! Returns the heap count
size_t os_getHeapListLength(void) {
	size_t length = 0;
	
	for(int i = 0; i < (sizeof(heaps)/sizeof(heaps[0])); ++i) {
		length++;
	}
	return length;
}

//! Returns the pointer of the heap with this index
Heap* os_lookupHeap(uint8_t index) {
	return heaps[index];
}


