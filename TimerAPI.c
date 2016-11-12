
#include "TimerAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>


INT8U FreeTmrCount = 0;
RTOS_TMR *FreeTmrListPtr = NULL;

// Tick Counter
INT32U RTOSTmrTickCtr = 0;

// Hash Table
HASH_OBJ hash_table[HASH_TABLE_SIZE];

// Thread variable for Timer Task
pthread_t thread;

// Semaphore for Signaling the Timer Task
sem_t timer_task_sem;

// Mutex for Protecting Hash Table
pthread_mutex_t hash_table_mutex;

// Mutex for Protecting Timer Pool
pthread_mutex_t timer_pool_mutex;

/*****************************************************
 * Timer API Functions
 *****************************************************
 */

// Function to create a Timer
RTOS_TMR* RTOSTmrCreate(INT32U delay, INT32U period, INT8U option, RTOS_TMR_CALLBACK callback, void *callback_arg, INT8	*name, INT8U *err)
{
	RTOS_TMR *timer_obj = NULL;
	
	// Check Option validity
	if((option != RTOS_TMR_ONE_SHOT) && (option != RTOS_TMR_PERIODIC)) {
		*err = RTOS_ERR_TMR_INVALID_OPT;
		return NULL;
	}

	// Check delay and period validity
	if(option == RTOS_TMR_ONE_SHOT) {
		if(delay <= 0) {
			*err = RTOS_ERR_TMR_INVALID_DLY;
			return NULL;
		}
	}
	else {
		if((period <= 0) || (delay < 0)) {
			*err = RTOS_ERR_TMR_INVALID_DLY;
			return NULL;
		}
	}

	// Allocate a Timer
	timer_obj = alloc_timer_obj();

	if(timer_obj == NULL) {
		// Timers are not available
		*err = RTOS_ERR_TMR_NON_AVAIL;
		return NULL;
	}

	// Fill up the Timer Object
	timer_obj->RTOSTmrType = RTOS_TMR_TYPE;
	timer_obj->RTOSTmrCallback = callback;
	timer_obj->RTOSTmrCallbackArg = callback_arg;
	timer_obj->RTOSTmrDelay = delay;
	timer_obj->RTOSTmrPeriod = period;
	timer_obj->RTOSTmrName = name;
	timer_obj->RTOSTmrOpt = option;
	timer_obj->RTOSTmrState = RTOS_TMR_STATE_STOPPED;

	*err = RTOS_SUCCESS;

	return timer_obj;
}

// Function to Delete a Timer
INT8U RTOSTmrDel(RTOS_TMR *ptmr, INT8U *perr)
{
	// ERROR Checking
	if(ptmr == NULL) {
		*perr = RTOS_ERR_TMR_INVALID;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrType != RTOS_TMR_TYPE) {
		*perr = RTOS_ERR_TMR_INVALID_TYPE;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_UNUSED) {
		*perr = RTOS_ERR_TMR_INACTIVE;
		return RTOS_FALSE;
	}

	switch(ptmr->RTOSTmrState) {
		case RTOS_TMR_STATE_STOPPED:
		case RTOS_TMR_STATE_COMPLETED:
			// Free the Timer Object
			free_timer_obj(ptmr);
			break;

		case RTOS_TMR_STATE_RUNNING:
			// Remove the Timer from the Hash Table List
			remove_hash_entry(ptmr);

			// Free the Timer Object
			free_timer_obj(ptmr);
			break;
		default:
			*perr = RTOS_ERR_TMR_INVALID_STATE;
			return RTOS_FALSE;
			break;
	}

	*perr = RTOS_SUCCESS;
	return RTOS_TRUE;
}

// Function to get the Name of a Timer
INT8* RTOSTmrNameGet(RTOS_TMR *ptmr, INT8U *perr)
{
	// ERROR Checking
	if(ptmr == NULL) {
		*perr = RTOS_ERR_TMR_INVALID;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrType != RTOS_TMR_TYPE) {
		*perr = RTOS_ERR_TMR_INVALID_TYPE;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_UNUSED) {
		*perr = RTOS_ERR_TMR_INACTIVE;
		return RTOS_FALSE;
	}

	*perr = RTOS_SUCCESS;
	// Return the Pointer to the String
	return ptmr->RTOSTmrName;	
}

// To Get the Number of ticks remaining in time out
INT32U RTOSTmrRemainGet(RTOS_TMR *ptmr, INT8U *perr)
{
	// ERROR Checking
	if(ptmr == NULL) {
		*perr = RTOS_ERR_TMR_INVALID;
		return 0;
	}

	if(ptmr->RTOSTmrType != RTOS_TMR_TYPE) {
		*perr = RTOS_ERR_TMR_INVALID_TYPE;
		return 0;
	}

	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_UNUSED) {
		*perr = RTOS_ERR_TMR_INACTIVE;
		return 0;
	}

	*perr = RTOS_SUCCESS;
	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_RUNNING) {
		return (ptmr->RTOSTmrMatch - RTOSTmrTickCtr);
	}
	else {
		return 0;
	}
}

// To Get the state of the Timer
INT8U RTOSTmrStateGet(RTOS_TMR *ptmr, INT8U *perr)
{
	// ERROR Checking
	if(ptmr == NULL) {
		*perr = RTOS_ERR_TMR_INVALID;
		return 0;
	}

	if(ptmr->RTOSTmrType != RTOS_TMR_TYPE) {
		*perr = RTOS_ERR_TMR_INVALID_TYPE;
		return 0;
	}

	*perr = RTOS_SUCCESS;
	return ptmr->RTOSTmrState;
}

// Function to start a Timer
INT8U RTOSTmrStart(RTOS_TMR *ptmr, INT8U *perr)
{
	if(ptmr->RTOSTmrType != RTOS_TMR_TYPE) {
		*perr = RTOS_ERR_TMR_INVALID_TYPE;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_UNUSED) {
		*perr = RTOS_ERR_TMR_INACTIVE;
		return RTOS_FALSE;
	}
	else if((ptmr->RTOSTmrState == RTOS_TMR_STATE_STOPPED) || (ptmr->RTOSTmrState == RTOS_TMR_STATE_COMPLETED)) {
		if(ptmr->RTOSTmrOpt == RTOS_TMR_ONE_SHOT) {
			// For One Shot Timer
			ptmr->RTOSTmrMatch = RTOSTmrTickCtr + ptmr->RTOSTmrDelay;

			insert_hash_entry(ptmr);
		}
		else {
			// For Periodic Timer
			if(ptmr->RTOSTmrDelay <= 0) {
				// Fill up the Time = Period
				ptmr->RTOSTmrMatch = RTOSTmrTickCtr + ptmr->RTOSTmrPeriod;
			}
			else {
				// Fill up the Time = Delay
				ptmr->RTOSTmrMatch = RTOSTmrTickCtr + ptmr->RTOSTmrDelay;
			}
			insert_hash_entry(ptmr);
		}
		ptmr->RTOSTmrState = RTOS_TMR_STATE_RUNNING;
	}
	else if(ptmr->RTOSTmrState == RTOS_TMR_STATE_RUNNING) {
		// No need to make any changes as it is already active
		// Do Nothing
	}
	else {
		// Invalid State
		*perr = RTOS_ERR_TMR_INVALID_STATE;
		return RTOS_FALSE;
	}

	*perr = RTOS_SUCCESS;
	return RTOS_TRUE;
}

// Function to Stop the Timer
INT8U RTOSTmrStop(RTOS_TMR *ptmr, INT8U opt, void *callback_arg, INT8U *perr)
{
	// ERROR Checking
	if(ptmr == NULL) {
		*perr = RTOS_ERR_TMR_INVALID;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrType != RTOS_TMR_TYPE) {
		*perr = RTOS_ERR_TMR_INVALID_TYPE;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_UNUSED) {
		*perr = RTOS_ERR_TMR_INACTIVE;
		return RTOS_FALSE;
	}

	if((opt != RTOS_TMR_OPT_NONE) && (opt != RTOS_TMR_OPT_CALLBACK) && (opt != RTOS_TMR_OPT_CALLBACK_ARG)) {
		*perr = RTOS_ERR_TMR_INVALID_OPT;
		return RTOS_FALSE;
	}

	if(ptmr->RTOSTmrState == RTOS_TMR_STATE_STOPPED) {
		*perr = RTOS_ERR_TMR_STOPPED;
		return RTOS_FALSE;
	}

	// Remove the Timer from the Hash Table List
	remove_hash_entry(ptmr);

	// Change the State to Stopped
	ptmr->RTOSTmrState = RTOS_TMR_STATE_STOPPED;

	switch(opt) {
		case RTOS_TMR_OPT_NONE:
			// Nothing to DO
			break;

		case RTOS_TMR_OPT_CALLBACK:
			// Execute the Callback Function
			ptmr->RTOSTmrCallback(ptmr->RTOSTmrCallbackArg);
			break;

		case RTOS_TMR_OPT_CALLBACK_ARG:
			// Execute the Callback Function with given Argument
			ptmr->RTOSTmrCallback(callback_arg);
			break;
	}
}

// Function called when OS Tick Interrupt Occurs which will signal the RTOSTmrTask() to update the Timers
void RTOSTmrSignal(int signum)
{
	// Received the OS Tick
	// Send the Signal to Timer Task
	sem_post(&timer_task_sem);
}

/*****************************************************
 * Internal Functions
 *****************************************************
 */

// Create Pool of Timers
INT8U Create_Timer_Pool(INT32U timer_count)
{
	int i;
	RTOS_TMR *current_ptr;
	RTOS_TMR *temp_ptr;

	FreeTmrCount = timer_count;
	for(i = 0; i < timer_count; i++) {	
		// Create a Timer Object
		temp_ptr = (RTOS_TMR*) malloc(sizeof(RTOS_TMR));
		
		if(temp_ptr == NULL) {
			return RTOS_MALLOC_ERR;
		}

		if(FreeTmrListPtr == NULL) {
			FreeTmrListPtr = temp_ptr;
			current_ptr = temp_ptr;
			current_ptr->RTOSTmrNext = NULL;
			current_ptr->RTOSTmrPrev = NULL;
		}
		else {
			current_ptr->RTOSTmrNext = temp_ptr;
			temp_ptr->RTOSTmrPrev = current_ptr;
			temp_ptr->RTOSTmrNext = NULL;
			current_ptr = temp_ptr;
		}
	}
	return RTOS_SUCCESS;
}

// Initialize the Hash Table
void init_hash_table(void)
{
	int i;

	for(i = 0; i < HASH_TABLE_SIZE; i++) {
		hash_table[i].timer_count = 0;
		hash_table[i].list_ptr = NULL;
	}
}

// Insert a Timer Object in the Hash Table
void insert_hash_entry(RTOS_TMR *timer_obj)
{
	INT8U index = 0;

	index = timer_obj->RTOSTmrMatch % HASH_TABLE_SIZE;
	
	// Lock the Resources
	pthread_mutex_lock(&hash_table_mutex);

	// Incremnet the Counter
	hash_table[index].timer_count++;

	// Add the Entry
	if(hash_table[index].list_ptr == NULL) {
		// List is empty
		// Add the first Element
		hash_table[index].list_ptr = timer_obj;
		timer_obj->RTOSTmrNext = NULL;
		timer_obj->RTOSTmrPrev = NULL;
	}
	else {
		// List pointer contain some entries
		// Add new entry
		timer_obj->RTOSTmrNext = hash_table[index].list_ptr;
		hash_table[index].list_ptr->RTOSTmrPrev = timer_obj;
		hash_table[index].list_ptr = timer_obj;
		timer_obj->RTOSTmrPrev = NULL;
	}

	// Unlock the Resources
	pthread_mutex_unlock(&hash_table_mutex);
}

// Remove the Timer Object entry from the Hash Table
void remove_hash_entry(RTOS_TMR *timer_obj)
{
	INT8U index = 0;

	index = timer_obj->RTOSTmrMatch % HASH_TABLE_SIZE;

	// Lock the Resources
	pthread_mutex_lock(&hash_table_mutex);

	if(timer_obj->RTOSTmrPrev == NULL) {
		// Its First Entry
		hash_table[index].list_ptr = timer_obj->RTOSTmrNext;

		if(hash_table[index].list_ptr != NULL) {
			hash_table[index].list_ptr->RTOSTmrPrev = NULL;
		}

		timer_obj->RTOSTmrNext = NULL;
		timer_obj->RTOSTmrPrev = NULL;
	}
	else {
		timer_obj->RTOSTmrPrev->RTOSTmrNext = timer_obj->RTOSTmrNext;

		if(timer_obj->RTOSTmrNext != NULL) {
			timer_obj->RTOSTmrNext->RTOSTmrPrev = timer_obj->RTOSTmrPrev;
		}

		timer_obj->RTOSTmrNext = NULL;
		timer_obj->RTOSTmrPrev = NULL;
	}

	// Decrement the Counter
	hash_table[index].timer_count--;

	// Unlock the Resources
	pthread_mutex_unlock(&hash_table_mutex);
}

// Timer Task to Manage the Running Timers
void *RTOSTmrTask(void *temp)
{
	INT8U index = 0;
	RTOS_TMR *temp_timer = NULL;
	RTOS_TMR *next_timer = NULL;

	while(1) {
		// Wait for the signal from RTOSTmrSignal()
		sem_wait(&timer_task_sem);

		// Once got the signal, Increment the Counter
		RTOSTmrTickCtr++;

		//fprintf(stdout, "\nGot Sem Signal....%d", RTOSTmrTickCtr);
		// Check the whole List associated with the index of the Hash Table
		index = RTOSTmrTickCtr % HASH_TABLE_SIZE;

		temp_timer = hash_table[index].list_ptr;
		
		if(temp_timer == NULL) {
			// No Entries in the List
			// Nothing to do
		}
		else {
			while(temp_timer != NULL) {
				if(temp_timer->RTOSTmrMatch == RTOSTmrTickCtr) {
					next_timer = temp_timer->RTOSTmrNext;
					
					// call the Callback function
					temp_timer->RTOSTmrCallback(temp_timer->RTOSTmrCallbackArg);

					// First Remove the Entry from Hash Table List
					remove_hash_entry(temp_timer);

					// Check whether timer is periodic
					if(temp_timer->RTOSTmrOpt == RTOS_TMR_PERIODIC) {
						// Again Add the Timer in the Hash Table
						// Fill up the Time = Period
						temp_timer->RTOSTmrMatch = RTOSTmrTickCtr + temp_timer->RTOSTmrPeriod;

						insert_hash_entry(temp_timer);
					}
					else {
						temp_timer->RTOSTmrState = RTOS_TMR_STATE_COMPLETED;
					}

					// Continue Checking the next list
					temp_timer = next_timer;
				}
				else {
					temp_timer = temp_timer->RTOSTmrNext;
				}
			}
		
		}
		index = 0;
		temp_timer = next_timer = NULL;
	}
	return temp;
}

// Timer Initialization Function
void RTOSTmrInit(void)
{
	INT32U timer_count = 0;
	INT8U	retVal;
	pthread_attr_t attr;

	fprintf(stdout,"\n\nHere Enter the number of Timers required in the Pool for the RTOS\n");
	fprintf(stdout,"Timers = ");
	scanf("%d", &timer_count);

	retVal = Create_Timer_Pool(timer_count);

	if(retVal == RTOS_SUCCESS) {
		fprintf(stdout, "\nOkay %d Timers Created Successfully in the Pool for the RTOS", timer_count);
	}
	else {
		fprintf(stdout, "\nTimer pool could not be created due to the Memory Error");
	}

	// Init Hash Table
	init_hash_table();

	fprintf(stdout, "\n\nHash Table Initialized Successfully\n");

	// Initialize Semaphore
	sem_init(&timer_task_sem, 0, 0);

	// Initialize Mutex
	pthread_mutex_init (&hash_table_mutex, NULL);
	pthread_mutex_init (&timer_pool_mutex, NULL);

	// Initialize the pthread Attributes
	pthread_attr_init (&attr);

	// Create Thread
	pthread_create (&thread, &attr, &RTOSTmrTask, NULL);

	fprintf(stdout,"\nRTOS Initialization Done...\n");
}

// Allocate a timer object from free timer pool
RTOS_TMR* alloc_timer_obj(void)
{
	RTOS_TMR *temp_ptr = NULL;

	// Lock the Resources
	pthread_mutex_lock(&timer_pool_mutex);
	
	// Check for Availability of Timers
	if(FreeTmrCount <= 0 || FreeTmrListPtr == NULL) {
		// No timers left
		// Unlock the Resources
		pthread_mutex_unlock(&timer_pool_mutex);

		return NULL;
	}	

	// Assign the Pointer from Top
	temp_ptr = FreeTmrListPtr;

	FreeTmrListPtr = FreeTmrListPtr->RTOSTmrNext;

	if(FreeTmrListPtr != NULL) {
		FreeTmrListPtr->RTOSTmrPrev = NULL;
	}

	temp_ptr->RTOSTmrPrev = NULL;
	temp_ptr->RTOSTmrNext = NULL;

	FreeTmrCount--;

	// Unlock the Resources
	pthread_mutex_unlock(&timer_pool_mutex);
	
	return temp_ptr;
}

// Free the allocated timer object and put it back into free pool
void free_timer_obj(RTOS_TMR *ptmr)
{
	// Lock the Resources
	pthread_mutex_lock(&timer_pool_mutex);

	// Clear the Fields
	ptmr->RTOSTmrCallback = NULL;
	ptmr->RTOSTmrCallbackArg = NULL;
	ptmr->RTOSTmrDelay = 0;
	ptmr->RTOSTmrPeriod = 0;
	ptmr->RTOSTmrName = NULL;
	ptmr->RTOSTmrOpt = 0;
	ptmr->RTOSTmrMatch = 0;

	// Change the State
	ptmr->RTOSTmrState = RTOS_TMR_STATE_UNUSED;

	// Return the Timer to Free Timer Pool
	ptmr->RTOSTmrNext = FreeTmrListPtr;
	ptmr->RTOSTmrPrev = NULL;

	if(FreeTmrListPtr != NULL) {
		FreeTmrListPtr->RTOSTmrPrev = ptmr;
	}

	FreeTmrListPtr = ptmr;

	// Increment the Free Timer Counter
	FreeTmrCount++;

	// Unlock the Resources
	pthread_mutex_unlock(&timer_pool_mutex);
} 

// Function to Setup the Timer of Linux which will provide the Clock Tick Interrupt to the Timer Manager Module
void OSTickInitialize(void) {	
	timer_t timer_id;
	struct itimerspec time_value;

	// Setup the time of the OS Tick as 100 ms after 3 sec of Initial Delay
	time_value.it_interval.tv_sec = 0;
	time_value.it_interval.tv_nsec = RTOS_CFG_TMR_TASK_RATE;

	time_value.it_value.tv_sec = 0;
	time_value.it_value.tv_nsec = RTOS_CFG_TMR_TASK_RATE;

	// Change the Action of SIGALRM to call a function RTOSTmrSignal()
	signal(SIGALRM, &RTOSTmrSignal);

	// Create the Timer Object
	timer_create(CLOCK_REALTIME, NULL, &timer_id);

	// Start the Timer
	timer_settime(timer_id, 0, &time_value, NULL);
}

void print_program_info(void)
{
	fprintf(stdout, "\n\n\n\nTimer Manager Project");
	fprintf(stdout, "\n=====================");

	fprintf(stdout, "\n\nCreated by: Prerak Doshi");

	fprintf(stdout, "\n\n-> This Project will initialize the timers and creates Timer Task as Thread");
	fprintf(stdout, "\n-> It creates 3 Timers");
	fprintf(stdout, "\n-> As per Professor's Doc File");
	fprintf(stdout, "\n\tTimer 1 - Periodic 5 second");
	fprintf(stdout, "\n\tTimer 2 - Periodic 3 second");
	fprintf(stdout, "\n\tTimer 3 - One Shot 10 second");
	
}

