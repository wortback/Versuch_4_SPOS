/*
 * os_memory.c
 *
 * Created: 25/05/2023 17:45:02
 *  Author: Mariia Z & Okan D
 */ 




#include "os_memory.h"
#include "os_memory_strategies.h"
#include "util.h"
#include "os_core.h"

// ---------------------------------------------------
//	Private Functions Declarations
// ---------------------------------------------------
void setMapEntry(Heap const *heap, MemAddr addr, MemValue value);


// ---------------------------------------------------

MemAddr convertToMemAddr(Heap const* heap, uint16_t nibbleaddr){
	return heap->firstMapAddr + nibbleaddr/2;
}

//! Allocates memory in the heap
MemAddr os_malloc(Heap *heap, size_t size) {
	os_enterCriticalSection();
	MemAddr procMemory = 0;
	
	// Allocate bytes depending on the current allocation strategy
	switch(os_getAllocationStrategy(heap)) {
		case OS_MEM_FIRST: 
			procMemory = os_MemAlloc_FirstFit(heap, size); 
			break;
		case OS_MEM_NEXT:
			procMemory = os_MemAlloc_NextFit(heap, size);
			break;
		case OS_MEM_BEST: 
			procMemory = os_MemAlloc_BestFit(heap, size);
			break;
		case OS_MEM_WORST: 
			procMemory = os_MemAlloc_WorstFit(heap, size);
			break;
	}
	
	
	// If the needed space cam be allocated
	if (procMemory != 0) {
		
		// Check if proMemory is a high or a low nibble
		int nib = procMemory % 2;
		size_t index = 1;
		
		// Calculate the size of the map address ???
		MemAddr	mapAddr = heap->firstMapAddr + (procMemory - heap->firstUseAddr)/2;
		
		// Get nibble
		MemValue nibble = os_getMapEntry(heap, mapAddr);


		// If it is the high nibble -> set the low nibble to F cuz it's also allocated
		if (nib == 0 && size > 1) {
			nibble = (nibble | 0x0F);
			// Plus one position allocated
			index+=1;
		}

		// Get current process ID
		ProcessID procID = os_getCurrentProc();
		
		// Set the nibble value to the PID of the process which got the memory allocated
		nibble = (nibble&(0x0F<<(nib*4))) | procID<<((1 - nib)*4); // keep read low nibble and replace high nibble if pMem is even otherwise reverse
		
		// Set a new value of the nibble at mapAddr
		setMapEntry(heap, mapAddr, nibble);
		
		// Allocate the $size$ amount of memory
		for (; index + 2 <= size; index+=2) {
			nibble = 0xFF;
			setMapEntry(heap, (mapAddr + (MemAddr)(index + nib)/2), nibble);
		}
		
		// if there is a nibble left, allocate it as well
			// only happens if we begun in the hight nibble
		if (index + 1 <= size) {
			nibble = os_getMapEntry(heap, mapAddr + (MemAddr)(index + 1)/2);
			nibble |= 0xF0;
			setMapEntry(heap, (mapAddr + (MemAddr)(index + 1)/2), nibble);
		}
	}
	os_leaveCriticalSection();
	
	return procMemory;
}


//! Low nibble setter
void setLowNibble(Heap const *heap, MemAddr addr, MemValue value) {
	// Save the value on the address for later
	MemValue temp = heap->driver->read(addr);
	
	// Set low nibble on the address -> preserve highest 4 bits and change only the lowest ones
	heap->driver->write(addr, ((temp & 0xF0) | (value & 0x0F)));
}

//! Hight nibble setter
void setHighNibble(Heap const *heap, MemAddr addr, MemValue value) {
	// Save the value on the address for later
	MemValue temp = heap->driver->read(addr);
	
	// Set high nibble on the address -> preserve lowest 4 bits and change only the highest ones
	heap->driver->write(addr, ((temp & 0x0F) | (value & 0xF0)));
}

//! Low nibble getter
MemValue getLowNibble(Heap const *heap, MemAddr addr) {
	MemValue temp = (heap->driver->read(addr) & 0x0F);
	return temp;
}

//! High nibble getter
MemValue getHighNibble(Heap const *heap, MemAddr addr) {
	MemValue temp = (heap->driver->read(addr) & 0xF0) >> 4;
	return temp;
}

//! Sets a heap map entry at the given heap
void setMapEntry(Heap const *heap, MemAddr addr, MemValue value) {
	heap->driver->write(addr, value);
}

//! Get the value of a map entry (used by allocation strategies)
MemValue os_getMapEntry(Heap const *heap, MemAddr addr) {
	MemValue temp = heap->driver->read(addr);
	return temp;
}

//! The first byte of chunk's address getter
MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr) {
	// Are we in the high or low nibble
	int nib = addr % 2;
	
	// get the map address
	MemAddr	mapAddr = heap->firstMapAddr + (addr - heap->firstUseAddr)/2;
	
	// if low nibble
	if(nib == 1) {
		if(getLowNibble(heap, mapAddr) == 0) {
			return addr;
		}
		
	// if high nibble
	} else {
		if(getHighNibble(heap, mapAddr) == 0) {
			return addr;
		}
	}
	
	// if high nibble and the low nibble is free
	if(nib == 1 && (getLowNibble(heap, mapAddr) != 0x0F)) {
		return addr;
	} else if(nib == 1 && (getHighNibble(heap, mapAddr) != 0x0F)) {
		return (addr-1);
	} else if(nib == 0 && (getHighNibble(heap, mapAddr) != 0x0F)) {
		return addr;
	}
	
	if(nib == 1) {
		addr = addr-2;
		} else {
		addr = addr-1;
	}
	mapAddr = mapAddr-1;
	
	MemValue cur = 0;
	for(; mapAddr >= heap->firstMapAddr; mapAddr--) {
		cur = os_getMapEntry(heap, mapAddr);
		for(int i = 0; i <= 1; i++) {
			if((cur&(0x0F << (i*4))) != 0x0F<<(i*4)) {
				return addr;
			}
			addr--;
		}
	}
	return 0;
}

//! Frees the chunk only if the owner
void os_freeAsOwner(Heap *heap, MemAddr addr, ProcessID owner) {
	// Find the first byte of the chunk related to the address
	MemAddr useAddr = os_getFirstByteOfChunk(heap, addr);
	if(useAddr != 0x0) { // If the address is not null

		int	nib = useAddr % 2; // Compute which nibble to work with, based on the address
		MemAddr	mapAddr	= heap->firstMapAddr + (useAddr - heap->firstUseAddr)/2; // Compute the corresponding map address
		MemValue nibble = os_getMapEntry(heap, mapAddr); // Get the current value at the map address
		MemValue newNibbles = 0; // Placeholder for the new value of the nibbles
		ProcessID heapOwner	= 0; // Placeholder for the owner of the heap block
		
		// Shift and mask the nibble to find the owner of the heap block
		heapOwner = (nibble>>((1-nib)*4)) & 0x0F;

		// If the owner of the heap block does not match the provided owner, exit the function
		if (heapOwner != owner) {
			return;
		}

		// If the address corresponds to the lower nibble
		if (nib == 1) {
			// Set the lower nibble to be free
			nibble &= 0xF0;
			setMapEntry(heap, mapAddr, nibble);
			// Move to the next map address
			mapAddr++;
			} else {
			// Set the upper nibble to be free
			nibble |= 0xF0;
			setMapEntry(heap, mapAddr, nibble);
		}
		
		// Iterate over the remaining map addresses
		do  {
			// Get the current value at the map address
			nibble = os_getMapEntry(heap, mapAddr);
			newNibbles = nibble;

			// If the upper nibble is not free and lower nibble is free, then set the upper nibble free as well
			if ((newNibbles & 0xF0) == 0xF0) {
				newNibbles &= 0x0F;
				
				// If the lower nibble is also free
				if ((newNibbles & 0x0F) == 0x0F)
				// Set the lower nibble free as well
				newNibbles &= 0xF0;
			}

			// If the nibble value has changed
			if (newNibbles != nibble) {
				// Set the map entry to the new nibbles
				setMapEntry(heap, mapAddr, newNibbles);
				// If both nibbles are free, move to the next map address
				if (newNibbles == 0)
				mapAddr++;
				else break;
			}
			// Continue until the nibble value doesn't change or until the end of the map addresses is reached
		} while (newNibbles != nibble && mapAddr < heap->firstUseAddr);
	}
}


//! Function used by processes to free their own allocated memory
void os_free(Heap *heap, MemAddr addr) {
	os_enterCriticalSection();
	os_freeAsOwner(heap, addr, os_getCurrentProc());
	os_leaveCriticalSection();
}

//! Heap map start getter
MemAddr os_getMapStart(Heap const *heap) {
	return heap->firstMapAddr;
}

//! Usable heap start getter
MemAddr os_getUseStart(Heap const *heap) {
	return heap->firstUseAddr;
}

//! Size of the heap map getter
size_t os_getMapSize(Heap const *heap) {
	return heap->sizeMap;
}

//! Size of the usable heap getter
size_t os_getUseSize(Heap const *heap) {
	return heap->sizeUse;
}

//! New allocation strategy setter
void os_setAllocationStrategy(Heap *heap, AllocStrategy newAllocStrat) {
	heap->currAllocStrat = newAllocStrat;
}

//! Current allocation strategy getter
AllocStrategy os_getAllocationStrategy(Heap const *heap) {
	return heap->currAllocStrat;
}

// Get the value of the specified nibble address in the heap's memory
uint8_t getNibbleVal(Heap const* heap, uint8_t nibbleaddr){
	// Convert the nibble address to the corresponding memory address
	MemAddr curMemAddr = convertToMemAddr(heap, nibbleaddr);
	// Read the full byte value from the memory address
	MemValue fullByte = heap->driver->read(curMemAddr);
	
	if(nibbleaddr % 2 == 0){ // If the nibble address is even (upper nibble)
		return (uint8_t)(fullByte >> 4); // Return the upper nibble value
		} else { // If the nibble address is odd (lower nibble)
		return fullByte & 0x0F; // Return the lower nibble value
	}
}

// Get the start address of the memory block containing the given address
uint16_t getStartOfBlock(Heap const* heap, uint16_t addr){
	while(getNibbleVal(heap, addr) == 0xF){ // While the nibble value is 0xF (indicating a used block)
		addr--; // Decrement the address
	}
	return addr; // Return the start address of the block
}

// Get the size of the memory block at the given address
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr){
	addr = getStartOfBlock(heap, addr); // Get the start address of the block
	uint16_t start = addr; // Store the start address
	
	do {
		addr++; // Increment the address
	} while(getNibbleVal(heap, addr) == 0xF); // While the nibble value is 0xF (indicating a used block)
	
	return (uint16_t)(addr - start); // Calculate and return the size of the block
}
