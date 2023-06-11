
/*
 * os_spi.c
 *
 * Created: 07/06/2023 13:13:16
 *  Author: Mariia Z and Okan
 */ 




#include "os_spi.h"



void os_spi_init(){
	// MOSI, SCK as output 
	DDRB &= 0b10111111;
	DDRB |= 0b10110000;
	
	// Set SPI2X in state register
	SPSR |= 0b00000001; 

	// Set SPE and MSTR
	SPCR |= 0b01010000;
}

uint8_t os_spi_send(uint8_t input){
	os_enterCriticalSection();
	SPDR = input;
	
	// Wait for the end of transmission 
	while((SPSR & (1 << SPIF)) == 0){}
	
	uint8_t result = SPDR;
	
	os_leaveCriticalSection();
	
	return result;
}


uint8_t os_spi_receive(){
	return os_spi_send(0xFF);
}