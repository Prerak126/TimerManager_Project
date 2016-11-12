Timer Manager By Prerak
=============

This is the Timer Manager created for the Real Time Operating System being developed in-house. 
Sample Application is provided along with code to test the Timer Manager Code.

File Structure
==============
TimerAPI.c 			-> Contains Timer Manager Public and Private functions
Application.c			-> Contains sample Application code to test the Timer Manager

TimerAPI.h			-> Header file containing Basic Type Definations, Related States, Structures,Hash Table,Functions, Error 					Codes,Timer API declarations

Platform
========
Linux with code developed in C Language

Instruction to Compile and Run
==============================

run below commands in the extracted folder
-> cd TimerManager Project By Prerak
-> gcc Application.c TimerAPI.c -o Prerak -lrt -lpthread
-> ./Prerak
(You need to provide the input for the number of Timers required in the pool for the OS)
=======================================

In this Project Timer 1 & 2 are periodic with 5 and 3 seconds respectively, and Timer 3 is one shot after 10  second timer.
We can make this global periodic with below changes;
timer_obji = RTOSTmrCreate(0, n_timer[i], RTOS_TMR_PERIODIC, &function1, NULL, timer_name[i], &err_val);

And then 
	fprintf(stdout, "\nPlease Enter the Time period of Timer...\n");

	scanf("%d",&n_timer[i]);
Now we can add period that has to be multiple of 10. Example for 10s= 10*10=100, so for 10s input should be 100.

Same, we can make this global one shot Timer
timer_obji = RTOSTmrCreate(n_timer[i], 0, RTOS_TMR_ONE_SHOT, &function3, NULL, timer_name[i], &err_val);

and then
	fprintf(stdout, "\nPlease Enter the delay period of Timer...\n");

	scanf("%d",&n_timer[i]);

