#include "os_input.h"

#include <avr/io.h>
#include <stdint.h>

/*! \file

Everything that is necessary to get the input from the Buttons in a clean format.

*/

/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 *
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 *
 */
uint8_t os_getInput(void) {
    //#warning IMPLEMENT STH. HERE
	
	// ESC 4, UP 3, DOWN 2, ENTER 1
	
	uint8_t state = ~(PINC);  // Invert to represent high input as 0 and sort out the pins that are no relevant
	state &= 0b11000011;
	uint8_t finalState = (state & 0b00000011) | (state >> 4);	//shift the 7, 6 pins to 3rd and 4th positions
	finalState &= 0b00001111;		// clear irrelevant bits
	return finalState;
}

/*!
 *  Initializes DDR and PORT for input
 */
void os_initInput() {
    //#warning IMPLEMENT STH. HERE
	
	// input configuration
	DDRC &= 0b00000000;
	// pull ups
	PORTC |= 0b11111111;
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void os_waitForNoInput() {
    //#warning IMPLEMENT STH. HERE
	
	while(os_getInput() != 0b00000000);
}

/*!
 *  Endless loop until at least one button is pressed.
 */
void os_waitForInput() {
    //#warning IMPLEMENT STH. HERE
	
	while(os_getInput() == 0b00000000);
}
