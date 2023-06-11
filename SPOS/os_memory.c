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
		nibble = (nibble&(0x0F<<(nib*4))) | procID<<((1 - nib)*4); // keep read low nibble and replace high nibble if procMemory is even otherwise reverse
		
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
	
	/* Check if no address found*/
	if(procMemory == 0) {
		os_leaveCriticalSection();
		return procMemory;
	}

	/* For efficient Garbage Collection */
	if(heap == intHeap) {
		MemAddr	mapAdr = heap->firstMapAddr + (procMemory - heap->firstUseAddr)/2;
		Process* curProcess = os_getProcessSlot(os_getCurrentProc());
		// Case: Process first time calling malloc
		if( curProcess->allocFrameStartInt > curProcess->allocFrameEndInt  ) {
			curProcess->allocFrameStartInt	= mapAdr;
			curProcess->allocFrameEndInt	= mapAdr + size/2;
		}
		// Case: New start Addr smaller than allocFrameStart
		else if( mapAdr < curProcess->allocFrameStartInt) {
			curProcess->allocFrameStartInt = mapAdr;
		}
		// Case: New end Addr larger than allocFrameEnd
		else if( (mapAdr + size/2) > curProcess->allocFrameEndInt) {
			curProcess->allocFrameEndInt = mapAdr + size/2;
		}
		} else if(heap == extHeap) {
		MemAddr	mapAdr = heap->firstMapAddr + (procMemory - heap->firstUseAddr)/2;
		Process* curProcess = os_getProcessSlot(os_getCurrentProc());
		// Case: Process first time calling malloc
		if( curProcess->allocFrameStartExt > curProcess->allocFrameEndExt  ) {
			curProcess->allocFrameStartExt	= mapAdr;
			curProcess->allocFrameEndExt	= mapAdr + size/2;
		}
		// Case: New start Addr smaller than allocFrameStart
		else if( mapAdr < curProcess->allocFrameStartExt) {
			curProcess->allocFrameStartExt = mapAdr;
		}
		// Case: New end Addr larger than allocFrameEnd
		else if( (mapAdr + size/2) > curProcess->allocFrameEndExt) {
			curProcess->allocFrameEndExt = mapAdr + size/2;
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
	
	size_t toFreeProcSize = os_getChunkSize(heap, useAddr);
	
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
		
		
		
		/* For efficient garbage collection */
		mapAddr	= heap->firstMapAddr + (useAddr - heap->firstUseAddr)/2;
		if(heap == intHeap) {
			Process* curProcess = os_getProcessSlot(owner);

			// Case: Process owns only one Chunk at the moment
			// if( (curProcess->allocFrameStartInt == mapAddr) && (curProcess->allocFrameEndInt == mapAddr + (toFreeProcSize/2))) {
			if( (curProcess->allocFrameStartInt == mapAddr) && ((curProcess->allocFrameEndInt == mapAddr + (toFreeProcSize/2)) || (curProcess->allocFrameEndInt == mapAddr + (toFreeProcSize/2) - 1))) {
				nibble = os_getMapEntry(heap, curProcess->allocFrameEndInt);
				if (!( (ProcessID)(nibble>>4) == owner || (ProcessID)(nibble&0x0F) == owner)) {
					curProcess->allocFrameStartInt	= heap->firstMapAddr + heap->sizeMap - 1;
					curProcess->allocFrameEndInt	= heap->firstMapAddr;
				}
			}

			// Case: We free the chunk which is set to allocFrameStart
			if((curProcess->allocFrameStartInt == mapAddr) && ((ProcessID)getHighNibble(heap, mapAddr) != owner)) {
				mapAddr += (toFreeProcSize/2);
				while(mapAddr <= curProcess->allocFrameEndInt) {
					nibble = os_getMapEntry(heap, mapAddr);
					if((ProcessID)(nibble>>4) == owner || (ProcessID)(nibble&0x0F) == owner) {
						curProcess->allocFrameStartInt = mapAddr;
						break;
					}
					mapAddr++;
				}
			}
			// Case: We free the chunk which is set to allocFrameEnd
			else if(curProcess->allocFrameEndInt == mapAddr + (toFreeProcSize/2)) {
				MemAddr tmp = 0;
				while(mapAddr >= curProcess->allocFrameStartInt) {
					nibble = os_getMapEntry(heap, mapAddr);
					if((tmp == 0) && (nibble != 0x00)) {
						tmp = mapAddr;
					}

					if((ProcessID)(nibble&0x0F) == owner) {
						switch(tmp) {
							case 0: curProcess->allocFrameEndInt	= mapAddr; break;
							default: curProcess->allocFrameEndInt	= tmp; break;
						}
						break;
						} else if(((ProcessID)(nibble&0x0F) != owner) && ((nibble&0x0F) != 0x0F)) {
						tmp = 0;
					}

					if((ProcessID)(nibble>>4) == owner) {
						switch(tmp) {
							case 0: curProcess->allocFrameEndInt	= mapAddr; break;
							default: curProcess->allocFrameEndInt	= tmp; break;
						}
						break;
						} else if((nibble>>4) != 0x0F) {
						tmp = 0;
					}

					mapAddr--;
				}
			}

		}
		else if (heap == extHeap) {
			Process* curProcess = os_getProcessSlot(owner);

			// Case: Process owns only one Chunk at the moment
			//if( (curProcess->allocFrameStartExt == mapAddr) && (curProcess->allocFrameEndExt == mapAddr + (toFreeProcSize/2))) {
			if( (curProcess->allocFrameStartExt == mapAddr) && ((curProcess->allocFrameEndExt == mapAddr + (toFreeProcSize/2)) || (curProcess->allocFrameEndExt == mapAddr + (toFreeProcSize/2) - 1))) {
				nibble = os_getMapEntry(heap, curProcess->allocFrameEndExt);
				if (!( (ProcessID)(nibble>>4) == owner || (ProcessID)(nibble&0x0F) == owner)) {
					curProcess->allocFrameStartExt	= heap->firstMapAddr + heap->sizeMap - 1;
					curProcess->allocFrameEndExt	= heap->firstMapAddr;
				}
			}

			// Case: We free the chunk which is set to allocFrameStart
			if(curProcess->allocFrameStartExt == mapAddr && ((ProcessID)getHighNibble(heap, mapAddr) != owner)) {
				mapAddr += (toFreeProcSize/2);
				while(mapAddr <= curProcess->allocFrameEndExt) {
					nibble = os_getMapEntry(heap, mapAddr);
					if((ProcessID)(nibble>>4) == owner || (ProcessID)(nibble&0x0F) == owner) {
						curProcess->allocFrameStartExt = mapAddr;
						break;
					}
					mapAddr++;
				}
			}
			// Case: We free the chunk which is set to allocFrameEnd
			else if(curProcess->allocFrameEndExt == mapAddr + (toFreeProcSize/2)) {
				MemAddr tmp = 0;
				while(mapAddr >= curProcess->allocFrameStartExt) {
					nibble = os_getMapEntry(heap, mapAddr);
					if((tmp == 0) && (nibble != 0x00)) {
						tmp = mapAddr;
					}

					if((ProcessID)(nibble&0x0F) == owner) {
						switch(tmp) {
							case 0: curProcess->allocFrameEndExt	= mapAddr; break;
							default: curProcess->allocFrameEndExt	= tmp; break;
						}
						break;
						} else if(((ProcessID)(nibble&0x0F) != owner) && ((nibble&0x0F) != 0x0F)) {
						tmp = 0;
					}

					if((ProcessID)(nibble>>4) == owner) {
						switch(tmp) {
							case 0: curProcess->allocFrameEndExt	= mapAddr; break;
							default: curProcess->allocFrameEndExt	= tmp; break;
						}
						break;
						} else if((nibble>>4) != 0x0F) {
						tmp = 0;
					}

					mapAddr--;
				}
			}
		}
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
uint16_t getStartOfBlock(Heap const* heap, uint16_t addr) {
	/*while(getNibbleVal(heap, addr) == 0xF){ // While the nibble value is 0xF (indicating a used block)
		addr--; // Decrement the address
	}
	return addr; // Return the start address of the block */
		while(addr > 0 && getNibble(heap,addr) == 0xf){
			addr--;
		}
		return addr;
}

// Get the size of the memory block at the given address
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr){
	uint16_t temp = addr - heap->firstUseAddr;
	addr = getStartOfBlock(heap, temp); // Get the start address of the block
	uint16_t start = addr; // Store the start address
	
	do {
		addr++; // Increment the address
	} while(getNibbleVal(heap, addr) == 0xF); // While the nibble value is 0xF (indicating a used block)
	
	return (uint16_t)(addr - start); // Calculate and return the size of the block
}

uint8_t getNibble(Heap const* heap, MemAddr addr){
	MemAddr currentAddr = convertToMemAddr(heap, addr);
	MemValue value = heap->driver->read(currentAddr);
	if(addr % 2 == 0){
		return (uint8_t)(value >> 4);
		} else {
		return value & 0x0f;
	}
}

MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size){
	os_enterCriticalSection();
	
	uint16_t nib = (addr - heap->firstUseAddr);

	uint16_t nibbleStartAddr = getStartOfBlock(heap, nib);
	addr = heap->firstUseAddr + nibbleStartAddr;
	MemAddr orgAddr = addr;

	ProcessID curProc = os_getCurrentProc();
	if(curProc == getNibble(heap, nibbleStartAddr)){
		uint16_t chunkSize = os_getChunkSize(heap, addr);

		if(size == chunkSize){

			} else if(size < chunkSize){
			for(uint16_t nib = nibbleStartAddr + size; nib < nibbleStartAddr + chunkSize; nib++){
				MemAddr curMemAddr = convertToMemAddr(heap,nib);
				MemValue fullByte = heap->driver->read(curMemAddr);
				if(nib % 2 == 0){
					fullByte = (fullByte & 0x0f) | (0 << 4);
					} else {
					fullByte = (fullByte & 0xf0) | 0;
				}
				heap->driver->write(curMemAddr,fullByte);
			}
			} else {
			uint16_t avail = chunkSize;
			uint16_t curNibbleAddr = nibbleStartAddr + chunkSize;
			while(getNibble(heap, curNibbleAddr) == 0 && avail < size && curNibbleAddr < heap->sizeUse){
				avail++;
				curNibbleAddr++;
			}

			if(avail >= size){
				for(uint16_t nib = nibbleStartAddr + chunkSize; nib < curNibbleAddr; nib++){
					//setNibble(heap, nib, 0xf);
					MemAddr curMemAddr = convertToMemAddr(heap,nib);
					MemValue fullByte = heap->driver->read(curMemAddr);
					if(nib % 2 == 0){
						fullByte = (fullByte & 0x0f) | (0xf << 4);
						} else {
						fullByte = (fullByte & 0xf0) | 0xf;
					}
					heap->driver->write(curMemAddr,fullByte);
				}

				if(heap->lastNibble[curProc] < nibbleStartAddr + size){
					heap->lastNibble[curProc] = nibbleStartAddr + size;
				}
				} else {
				curNibbleAddr = nibbleStartAddr - 1;
				while(getNibble(heap, curNibbleAddr) == 0 && curNibbleAddr >= 0){
					avail++;
					if(curNibbleAddr == 0){
						break;
						} else {
						curNibbleAddr--;
					}
				}
				if(getNibble(heap, curNibbleAddr) != 0){
					curNibbleAddr++;
				}
				if(avail < size){
					addr = 0;
					} else {
					//addr = getAddrForNibble(heap, curNibbleAddr); 
					addr = heap->firstUseAddr + curNibbleAddr;

					for(MemAddr offset = 0; offset < chunkSize; offset++){
						uint8_t byte = heap->driver->read(orgAddr + offset);
						heap->driver->write(addr + offset, byte);
					}

					//setNibble(heap, curNibbleAddr, curProc);
					MemAddr curMemAddr = convertToMemAddr(heap,curNibbleAddr);
					MemValue fullByte = heap->driver->read(curMemAddr);
					if(curNibbleAddr % 2 == 0){
						fullByte = (fullByte & 0x0f) | (curProc << 4);
						} else {
						fullByte = (fullByte & 0xf0) | curProc;
					}
					heap->driver->write(curMemAddr, fullByte);
					
					for(uint16_t nib = curNibbleAddr + 1; nib < (curNibbleAddr + size); nib++){
						//setNibble(heap, nib, 0xf);
						MemAddr curMemAddr = convertToMemAddr(heap,nib);
						MemValue fullByte = heap->driver->read(curMemAddr);
						if(nib % 2 == 0){
							fullByte = (fullByte & 0x0f) | (0xf << 4);
							} else {
							fullByte = (fullByte & 0xf0) | 0xf;
						}
						heap->driver->write(curMemAddr,fullByte);
					}

					for(uint16_t nib = curNibbleAddr + size; nib < nibbleStartAddr + chunkSize; nib++){
						//setNibble(heap, nib, 0);
						MemAddr curMemAddr = convertToMemAddr(heap,nib);
						MemValue fullByte = heap->driver->read(curMemAddr);
						if(nib % 2 == 0){
							fullByte = (fullByte & 0x0f) | (0 << 4);
							} else {
							fullByte = (fullByte & 0xf0) | 0;
						}
						heap->driver->write(curMemAddr,fullByte);
					}

					if(heap->lastNibble[curProc] < curNibbleAddr + size){
						heap->lastNibble[curProc] = curNibbleAddr + size;
					}

					if(heap->firstNibble[curProc] > curNibbleAddr){
						heap->firstNibble[curProc] = curNibbleAddr;
					}
				}

			}
		}
	}

	if(addr == 0){
		addr = os_malloc(heap, size);
		if(addr != 0){
			for(uint16_t offset = 0; offset < os_getChunkSize(heap,orgAddr); offset++){
				uint8_t byte = heap->driver->read(orgAddr + offset);
				heap->driver->write(addr + offset, byte);
			}
			os_free(heap,orgAddr);
		}
	}

	os_leaveCriticalSection();

	return addr;
}

void os_freeProcessMemory(Heap* heap, ProcessID pid){
	os_enterCriticalSection();

	uint16_t min = heap->firstNibble[pid];
	uint16_t max = heap->lastNibble[pid];

	for(uint16_t nib = min; nib < max; nib++){
		if(getNibble(heap, nib) == pid){
			do {
				//setNibble(heap, nib, 0);
				MemAddr curMemAddr = convertToMemAddr(heap,nib);
				MemValue fullByte = heap->driver->read(curMemAddr);
				if(nib % 2 == 0){
					fullByte = (fullByte & 0x0f) | (0 << 4);
					} else {
					fullByte = (fullByte & 0xf0) | 0;
				}
				heap->driver->write(curMemAddr,fullByte);
				nib++;
			} while(nib < max && getNibble(heap, nib) == 0xf);
			nib--;
		}
	}
	os_leaveCriticalSection();
}
