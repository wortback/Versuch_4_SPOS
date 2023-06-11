/*
 * os_mem_drivers.h
 *
 * Created: 23/05/2023 22:07:15
 *  Author: Mariia Z
 */ 


#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#include <avr/pgmspace.h>
#include "defines.h"
#include <inttypes.h>


//! A type to represent a memory address
typedef uint16_t MemAddr;

//! A type to represent a memory value
typedef uint8_t MemValue;

//! A type for the function that initializes of memory driver
typedef void MemoryDriverInit(void);

//! A type for the function that reads a value on the specified memory address
typedef MemValue MemoryRead(MemAddr addr);

//! A type for the function that writes a value on the specified memory address
typedef void MemoryWrite(MemAddr addr, MemValue value);

//! Pointer to the driver instance
#define intSRAM (&intSRAM__)

//! Pointer to the external memory 
#define extSRAM (&extSRAM__)

//! Initialize the external memory
void external_init(void);


//! A general memory driver
typedef struct {
	MemoryDriverInit *init;
	MemoryRead *read;
	MemoryWrite *write;

	MemAddr firstAddr; // 0x100
	size_t size;
	} MemDriver;

//! Function that initializes a driver instance 
void initMemoryDriver(void);

//! Driver instance
extern MemDriver intSRAM__;

extern MemDriver extSRAM__;

#endif