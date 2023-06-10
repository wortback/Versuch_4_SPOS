/*! \file

Scheduling strategies used by the Interrupt Service RoutineA from Timer 2 (in scheduler.c)
to determine which process may continue its execution next.

The file contains five strategies:
-even
-random
-round-robin
-inactive-aging
-run-to-completion
*/

#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>

//----------GLOBALS------------
SchedulingInformation schedulingInfo;



/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 *  \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    // This is a presence task
	if(strategy == OS_SS_ROUND_ROBIN){
	    schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
	} 
	
	else if(strategy == OS_SS_INACTIVE_AGING){
	    for(ProcessID i = 0; i <= MAX_NUMBER_OF_PROCESSES - 1; ++i){
		    schedulingInfo.age[i] = 0;
	    }
    }
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    // This is a presence task
	schedulingInfo.age[id] = 0;
}

/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current) {
  //  #warning IMPLEMENT STH. HERE
  //var rdy will be set on 0, it counts the amount of processes in the state "ready" 
  uint8_t rdy = 0;
  //loop which goes through the processlist and checks for every process if it's ready
  for(uint8_t i = 1; i<MAX_NUMBER_OF_PROCESSES; i++){
	  //if the current checked process is ready "rdy" will be incremented by one
	  if(processes[i].state == OS_PS_READY){
		  rdy++;
	  }
  }
  //if no process is ready return 0 (ID of idle)
  if(rdy == 0){return 0;}
  //a second loop that start on the latest runned process and goes through the rest of processes
  for(uint8_t i = current+1; i<=MAX_NUMBER_OF_PROCESSES; i++){
	//if the end of the processlist has been reached the index will be reseted to the beginning of the list (0 is reserved for idle) 
	  if(i==MAX_NUMBER_OF_PROCESSES){
		  i=1;
	  }
	  //if the current process is on "ready" return its I, that's the next process that will be running
	  if(processes[i].state == OS_PS_READY){
		  return i;
	  }
  }
  //if no process has been found return 0 (idle)
  return 0;
  }



/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current) {
    //#warning IMPLEMENT STH. HERE
	//Var randNum saves random number
	uint16_t randNum = 0;
	//numbers of processes that are ready to be running
	uint8_t ready = 0;
	//loop goes through every process
	for(uint8_t i = 1; i<MAX_NUMBER_OF_PROCESSES; i++){
		//if current process is ready to run increment "ready"
		if(processes[i].state == OS_PS_READY){
			ready++;
		}
	}
	//this checks if no process is ready, if so return process with ID 0 (idle)
	if(ready == 0){
		return 0;
	}
	//generates random number between 0 and ready-1 to choose a random process
	randNum = rand()%ready;
	//sets ready on 0 again to use it for the next loop
	ready = 0;
	//second loop runs again through all processes if current process is ready check if "ready" is = randNum, if so return the ID of the process, 
	//otherwise increment "ready"
	for(uint16_t i = 1; i<MAX_NUMBER_OF_PROCESSES; i++ ){
		if(processes[i].state == OS_PS_READY){
			if(ready == randNum){
				return i;
			}
			ready++;
		}
	}
	// if no process has been chosen return ID of the current process
	return current;
}


/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current) {
    // This is a presence task
	
	// Decrement the timeSlice of a current process till it reaches 0
    if(schedulingInfo.timeSlice > 1 && processes[current].state == OS_PS_READY){
	    schedulingInfo.timeSlice--;
	    return current;
	}
	
	// If 0, select a new process with EVEN strategy
	else {
	    ProcessID next = os_Scheduler_Even(processes, current);
	    schedulingInfo.timeSlice = processes[next].priority;
	    
	    return next;
    }
}

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to its priority.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current) {
    // This is a presence task
	
	// Reset the age of the current process
	schedulingInfo.age[current] = 0;
	
	// Increase the age of every process of its priority 
	for(ProcessID i = 1; i <= MAX_NUMBER_OF_PROCESSES-1; ++i){
		if(processes[i].state == OS_PS_READY){
			schedulingInfo.age[i] += processes[i].priority;
		}
	}
	
	ProcessID next = 0;
	
	for(ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state == OS_PS_READY){
			if(schedulingInfo.age[i] > schedulingInfo.age[next]){
				next = i;
				} else if(schedulingInfo.age[i] == schedulingInfo.age[next] && processes[i].priority > processes[next].priority){
				next = i;
			}
		}
	}
	
	return next;
}

/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current) {
    // This is a presence task
	
	// if the process is not completed yet, continue with the execution
    if(processes[current].state == OS_PS_READY){
	    return current;
    }
    else{
		return os_Scheduler_Even(processes, current);	
	}
}
