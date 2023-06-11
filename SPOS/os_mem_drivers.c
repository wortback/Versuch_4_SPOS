/*
 * os_mem_drivers.c
 * Contains all functions of the low level storage
 *
 * Created: 23/05/2023 22:07:46
 *  Author: Mariia Z
 */ 

#include "os_mem_drivers.h"
#include "defines.h"

#include "os_scheduler.h"
#include "os_spi.h"
#include "util.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "os_core.h"




MemDriver intSRAM__;

MemDriver extSRAM__;

// Don't have to do anything during the initialization
void internal_init(void) {}
	
// External SRAM
void external_init(void){
	os_enterCriticalSection();
	DDRB |= (1 << 4);
	
	os_spi_init();
	
	if((PINB & 0b0001000) == 1) {
		os_error("extSRAM as slave!");
	}
	
	PORTB &= ~(1 << 4);
	os_spi_send(0x01);
	
	// Byte mode activation
	os_spi_send(0x00);
	
	PORTB |= (1 << 4);
	os_leaveCriticalSection();
}

// Reads a value from the internal SRAM
MemValue internal_read(MemAddr addr) {
	return *((MemValue*)addr);
}

MemValue external_read(MemAddr addr) {
	os_enterCriticalSection();
	
	PORTB &= ~(1 << 4);
	os_spi_send(0x03);
	os_spi_send(0x00);
	os_spi_send(addr >> 8);
	os_spi_send(addr & 0xff);
	
	uint8_t result = os_spi_receive();
	PORTB |= (1 << 4);
	
	os_leaveCriticalSection();

	return result;
}

// Returns a value on the given address
void internal_write(MemAddr addr, MemValue value) {
	*((MemValue*)addr) = value;
}

void external_write(MemAddr addr, MemValue value) {
	os_enterCriticalSection();
	PORTB &= ~(1 << 4);
	
	os_spi_send(0x02);
	os_spi_send(0x00);
	os_spi_send(addr >> 8);
	os_spi_send(addr & 0xff);
	os_spi_send(value);
	
	PORTB |= (1 << 4);
	os_leaveCriticalSection();
}



void initMemoryDriver(void) {
	// Internal SRAM
	intSRAM__.init = &internal_init;
	intSRAM__.read = &internal_read;
	intSRAM__.write = &internal_write;
	intSRAM__.size = AVR_MEMORY_SRAM; 
	intSRAM__.firstAddr = AVR_SRAM_START; 
	
	
	// External SRAM
	extSRAM__.init = &external_init;
	extSRAM__.read = &external_read;
	extSRAM__.write = &external_write;
	extSRAM__.size = 0xFFFF;
	extSRAM__.firstAddr = 0;
}