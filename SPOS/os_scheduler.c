/*! \file
 *  \brief Scheduling module for the OS.
 *
 * Contains everything needed to realise the scheduling between multiple processes.
 * Also contains functions to start the execution of programs.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "lcd.h"

#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/delay.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
//#warning IMPLEMENT STH. HERE

Process os_processes[MAX_NUMBER_OF_PROCESSES];

//! Index of process that is currently executed (default: idle)
//#warning IMPLEMENT STH. HERE

ProcessID currentProc;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
// #warning IMPLEMENT STH. HERE

SchedulingStrategy currStrategy;

//! Count of currently nested critical sections
//#warning IMPLEMENT STH. HERE

uint8_t criticalSectionCount;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect) __attribute__((naked));

void os_dispatcher(void);

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
    //#warning IMPLEMENT STH. HERE
	
	// if a process is interrupted, save the context on the stack
	saveContext();
	
	// save the stack pointer
	os_processes[currentProc].sp.as_int = SP;
	
	// set the stack pointer to scheduler stack
	SP = BOTTOM_OF_ISR_STACK;
	os_processes[currentProc].checksum = os_getStackChecksum(currentProc);
	
	// If esc and enter pressed
	if(os_getInput() == 0b00001001){
		os_waitForNoInput();
		os_taskManMain();
	}
	
	// set the state of the current process to RUNNING
	if ( os_processes[os_getCurrentProc()].state != OS_PS_UNUSED ) {
		os_processes[os_getCurrentProc()].state = OS_PS_READY;
		}
	
	// Select the next process based on the current scheduling strategy
	switch(os_getSchedulingStrategy()){
		case OS_SS_EVEN:
			currentProc = os_Scheduler_Even(os_processes, currentProc);
			break;
		
		case OS_SS_RANDOM:
			currentProc = os_Scheduler_Random(os_processes, currentProc);
			break;
		
		case OS_SS_ROUND_ROBIN:
			currentProc = os_Scheduler_RoundRobin(os_processes, currentProc);
			break;
			
		case OS_SS_RUN_TO_COMPLETION:
			currentProc = os_Scheduler_RunToCompletion(os_processes, currentProc);
			break;
		
		case OS_SS_INACTIVE_AGING:
			currentProc = os_Scheduler_InactiveAging(os_processes, currentProc);
			break;
	}
	
	if(os_processes[currentProc].checksum != os_getStackChecksum(currentProc)){
		os_error("The stack is inconsistent");
	}
	
	// set the state of the new current process to RUNNING
	os_processes[currentProc].state = OS_PS_RUNNING;
	
	// set the stack pointer to the stack of the next process
	SP = os_processes[currentProc].sp.as_int;
	
	// restore the runtime context
	restoreContext();
}

/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
void idle(void) {
   // #warning IMPLEMENT STH. HERE
   // infinite output of "." on the LCD
    while(1){
	    lcd_writeChar('.');
		// wait as many millisecs as the define default_output_delay says
	    _delay_ms(DEFAULT_OUTPUT_DELAY);
    }
}

/*!
 *  This function is used to execute a program that has been introduced with
 *  os_registerProgram.
 *  A stack will be provided if the process limit has not yet been reached.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param program  The function of the program to start.
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favourable
 *                   - 255 means most favourable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process or INVALID_PROCESS as specified in
 *          defines.h on failure
 */
ProcessID os_exec(Program *program, Priority priority) {
    //#warning IMPLEMENT STH. HERE
	
	os_enterCriticalSection();
	uint8_t pid = 0;
	for(int i = 0; i<MAX_NUMBER_OF_PROCESSES; i++){
		if(os_processes[pid].state != OS_PS_UNUSED){
			pid++;
		}else{break;}
	}
		
	if(pid == MAX_NUMBER_OF_PROCESSES){
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}
		
	if(program == NULL){
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}
	
	Process prog;
	prog.program = program;
	prog.priority = priority;
	prog.state = OS_PS_READY;
	prog.sp.as_int = PROCESS_STACK_BOTTOM(pid);
	
	prog.sp.as_ptr[0] = (uint8_t)(((uint16_t)(&os_dispatcher))&0x00FF);
	prog.sp.as_ptr[-1] = (uint8_t)(((uint16_t)(&os_dispatcher))>>8);
	

	for(int i = 0; i < 33; i++) {
		prog.sp.as_ptr[-2-i] = 0;
		prog.sp.as_int--;
	}
	prog.sp.as_int-=2;
	os_processes[pid] = prog;
	os_processes[pid].checksum = os_getStackChecksum(pid);
	
	
	os_resetProcessSchedulingInformation(pid);
	
	os_leaveCriticalSection();
	return pid;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
    //#warning IMPLEMENT STH. HERE
	//set Variable for currProc on 0 (idle)
	currentProc = 0;
	
	//change state of idle to OS_PS_running
	os_processes[currentProc].state = OS_PS_RUNNING;
	
	//set stack pointer to the process stack of idle
	SP = os_processes[currentProc].sp.as_int;
	//jump to idle with restoreContext
	restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void) {
    //#warning IMPLEMENT STH. HERE
	
	// Set all processes' states in os_pocesses to OS_PS_UNUSED
	for(uint8_t i = 0; i<MAX_NUMBER_OF_PROCESSES; i++){
		os_processes[i].state = OS_PS_UNUSED;
	}
	
	os_exec(idle, DEFAULT_PRIORITY);
	
	while(autostart_head != NULL){
		os_exec(autostart_head->program, DEFAULT_PRIORITY);
		autostart_head = autostart_head->next;
	}
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process* os_getProcessSlot(ProcessID pid) {
    return os_processes + pid;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
    //#warning IMPLEMENT STH. HERE
	
	return currentProc;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy) {
  //  #warning IMPLEMENT STH. HERE
  currStrategy = strategy;
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    //#warning IMPLEMENT STH. HERE
	return currStrategy;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behaviour when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
  //  #warning IMPLEMENT STH. HERE
  //every time a critical section has been entered we increment criticalSectionCount and if we leave one we decrement, because of uint8 
  // the variable is only able to 255 (max nesting depth), everything above 255 will cause an error
  if (criticalSectionCount == 255){
	  os_error("Overflow on critical sections");
	  return;
  }
  //Saves the state of the Global interrupt bit (highest bit in SREG) in the local variable "a" and set every bit except for the GIEB on 0
  uint8_t a = (SREG & 0b10000000);
  // sets GIEB to 0 / deactivates the GIEB, therefore global interrupts will be deactivated
  SREG &= 0b011111111;
  //increment variable criticalSectionCount which is following the nesting depth
  criticalSectionCount++;
  //deactivate scheduler through changing a bit in the register TIMSK2
  TIMSK2 &= 0b11111101;
  //restore the previous saved state of the GIEB in SREG
  SREG |= a;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
    //#warning IMPLEMENT STH. HERE
	// same as above, but in this case if we increment when the counter is on 0 we will reach an underflow
	 if (criticalSectionCount == 0){
		 os_error("Underflow on critical sections");
		 return;
	 }
	 //function works the same as above when entering an Critical section, but this time we decrement CriticalSectionCount, because
	// thats when we leave a critical section.
	 uint8_t a = (SREG & 0b10000000);
	 SREG &= 0b011111111;
	 criticalSectionCount--;
	 // when cricital section is 0 there is no further critical sections we have entered, the scheduler will be activated through setting the OCIE2A
	 // bit
	
	 if(criticalSectionCount == 0){
		 TIMSK2 |= 0b00000010;
	 }
	 // restore state of GIEB
	 SREG |= a;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
   // #warning IMPLEMENT STH. HERE
   
   //var tmp will be set on 11, thats the beginning value of the checksum
   StackChecksum tmp = 11;
   //declare a stack pointer c
   StackPointer c;
   
   //stack pointer c will be set on the beginning of the process stack with the ID "pid"
   c.as_int = PROCESS_STACK_BOTTOM(pid);
   //iterate through every byte of the process stack from to beginning to the 35th bit
   while(c.as_int >= PROCESS_STACK_BOTTOM(pid)-35){
	   //update checksum through XOR operation 
	   tmp ^= *c.as_ptr;
	   //decrement c to switch to the next byte in the stack
	   c.as_int --;
   }
   //return checksum
   return tmp;
}

bool os_kill(ProcessID pid)
{
	// Check if the provided process ID is out of bounds (lower or upper limit). If it is, return false.
	if(pid == 0 || pid > MAX_NUMBER_OF_PROCESSES)
	return false;
	else
	{
		// Enter critical section to avoid simultaneous access issues
		os_enterCriticalSection();
		
		// Set the state of the process to unused, effectively "killing" it
		os_processes[pid].state = OS_PS_UNUSED;
		
		// If the process being killed is the currently running one, reset the critical section count
		if(pid == os_getCurrentProc())
		criticalSectionCount = 1;
		
		// Leave the critical section
		os_leaveCriticalSection();
		
		// Wait for the task manager to be open and for the current process to be different from the one being killed
		while (pid == os_getCurrentProc() && !os_taskManOpen());
		
		// Return true to signal a successful kill operation
		return true;
	}
}



void os_dispatcher(void){
	// Get the current PID
	ProcessID currProc = os_getCurrentProc();
	
	// Save the pointer to the current process' program
	Program* currProg = os_processes[currProc].program;
	//Call the program
	(*currProg)();
	os_kill(currProc);
}