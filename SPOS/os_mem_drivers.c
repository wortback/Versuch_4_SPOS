/*
 * os_mem_drivers.c
 * Contains all functions of the low level storage
 *
 * Created: 23/05/2023 22:07:46
 *  Author: Mariia Z
 */ 

#include "os_mem_drivers.h"
#include "defines.h"

MemDriver intSRAM__;

// Don't have to do anything during the initialization
void internal_init(void) {}

// Reads a value from the internal SRAM
MemValue internal_read(MemAddr addr) {
	return *((MemValue*)addr);
}

// Returns a value on the given address
void internal_write(MemAddr addr, MemValue value) {
	*((MemValue*)addr) = value;
}

void initMemoryDriver(void) {
	intSRAM__.init = &internal_init;
	intSRAM__.read = &internal_read;
	intSRAM__.write = &internal_write;
	intSRAM__.size = AVR_MEMORY_SRAM; 
	intSRAM__.firstAddr = AVR_SRAM_START; 
}