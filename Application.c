// Application file to Demonstrate the Usage of Timer Manager
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "TimerAPI.h"

extern pthread_t thread;

// Function to Print the Time
void print_time(void)
{
	struct tm *Time;
	time_t t;

	t = time(NULL);
	Time = localtime(&t);
	fprintf(stdout, "UTC time and date: %s\n", asctime(Time));
}

// Function to Print the Time and Msg in Callback Function
void print_time_msg(int num)
{
	struct tm *Time;
	time_t t;

	t = time(NULL);
	Time = localtime(&t);
	fprintf(stdout, "This is Function %d and UTC time and date: %s\n", num, asctime(Time));
}

void function1(void *arg)
{
	print_time_msg(1);
}

void function2(void *arg)
{
	print_time_msg(2);
}

void function3(void *arg)
{
	print_time_msg(3);
}

int main(void) 
{
	INT8U err_val;

	RTOS_TMR *timer_obj1 = NULL;
	RTOS_TMR *timer_obj2 = NULL;
	RTOS_TMR *timer_obj3 = NULL;

	INT8 *timer_name[3] = {"Timer 1", "Timer 2", "Timer 3"};

	// Display the Program Info
	print_program_info();

	// Initialize the OS Tick
	OSTickInitialize();

	fprintf(stdout, "\nRTOS Tick Initialization completed successfully");

	// Initialize the RTOS Timer
	RTOSTmrInit();

	fprintf(stdout, "\nRTOS Timer Initialized now.\n");
	
	// Create Timer1
	timer_obj1 = RTOSTmrCreate(0, 50, RTOS_TMR_PERIODIC, &function1, NULL, timer_name[0], &err_val);

	if(err_val == RTOS_SUCCESS) {
		fprintf(stdout, "\nTimer 1 Created Successfully at ");
		print_time();
	}
	else {
		fprintf(stderr, "\nTimer 1 failed with error = %d", err_val);
	}

	// Create Timer2
	timer_obj2 = RTOSTmrCreate(0, 30, RTOS_TMR_PERIODIC, &function2, NULL, timer_name[1], &err_val);

	if(err_val == RTOS_SUCCESS) {
		fprintf(stdout, "\nTimer 2 Created Successfully at ");
		print_time();
	}
	else {
		fprintf(stderr, "\nTimer 2 failed with error = %d", err_val);
	}

	// Create Timer3
	timer_obj3 = RTOSTmrCreate(100, 0, RTOS_TMR_ONE_SHOT, &function3, NULL, timer_name[2], &err_val);


	if(err_val == RTOS_SUCCESS) {
		fprintf(stdout, "\nTimer 3 Created Successfully at ");
		print_time();
	}
	else {
		fprintf(stderr, "\nTimer 3 failed with error = %d", err_val);
	}

	// Start Timer1
	RTOSTmrStart(timer_obj1, &err_val);

	if(err_val == RTOS_SUCCESS) {
		fprintf(stdout, "\nTimer 1 Started Successfully at ");
		print_time();
	}
	else {
		fprintf(stderr, "\nTimer 1 failed with error = %d", err_val);
	}

	// Start Timer2
	RTOSTmrStart(timer_obj2, &err_val);

	if(err_val == RTOS_SUCCESS) {
		fprintf(stdout, "\nTimer 2 Started Successfully at ");
		print_time();
	}
	else {
		fprintf(stderr, "\nTimer 2 failed with error = %d", err_val);
	}

	// Start Timer3
	RTOSTmrStart(timer_obj3, &err_val);

	if(err_val == RTOS_SUCCESS) {
		fprintf(stdout, "\nTimer 3 Started Successfully at ");
		print_time();
		fprintf(stdout, "\n");
	}
	else {
		fprintf(stderr, "\nTimer 3 failed with error = %d", err_val);
	}
	
	// Wait for the created Timer Thread (We can also use pthread_exit())
	pthread_join(thread, NULL);

	return 0;
}

