
/*
 * os_spi.h
 *
 * Created: 07/06/2023 13:13:25
 *  Author: Mariia Z and Okan
 */ 




#ifndef OS_SPI_H_
#define OS_SPI_H_

#include "os_scheduler.h"
#include <avr/io.h>
#include <stdint.h>

//! Initializes the SPI and configures the relevant registers
void os_spi_init();

//! Performs the data transmission
uint8_t os_spi_send(uint8_t input);

//! Receives a dummy byte
uint8_t os_spi_receive();


#endif