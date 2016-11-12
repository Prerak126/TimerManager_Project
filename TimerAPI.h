// Header File for Timer APIs
#ifndef TIMER_API_H
#define TIMER_API_H


// Typedefines
typedef unsigned char INT8U;
typedef unsigned short int INT16U;
typedef unsigned int INT32U;

typedef char INT8;
typedef short int INT16;
typedef int INT32;


// OS Tick Time in ns
#define RTOS_CFG_TMR_TASK_RATE	100000000

// Lets assume RTOS Timer Type = 20
#define RTOS_TMR_TYPE	20

// RTOS SUCCESS/FAILURE
#define RTOS_FALSE	0
#define RTOS_TRUE	1

// RTOS Timer States
#define	RTOS_TMR_STATE_UNUSED		1
#define	RTOS_TMR_STATE_STOPPED		2
#define	RTOS_TMR_STATE_RUNNING		3
#define	RTOS_TMR_STATE_COMPLETED	4

// RTOS Timer Options
#define RTOS_TMR_ONE_SHOT	1
#define RTOS_TMR_PERIODIC	2

// Error Code
#define RTOS_ERR_NONE			0
#define RTOS_SUCCESS			0
#define RTOS_MALLOC_ERR			1
#define RTOS_ERR_TMR_INVALID_DLY	2
#define RTOS_ERR_TMR_INVALID_PERIOD	3
#define RTOS_ERR_TMR_INVALID_OPT	4
#define RTOS_ERR_TMR_NON_AVAIL		5
#define RTOS_ERR_TMR_INVALID_TYPE	6
#define RTOS_ERR_TMR_INACTIVE		7
#define RTOS_ERR_TMR_INVALID_STATE	8
#define RTOS_ERR_TMR_INVALID		9
#define RTOS_ERR_TMR_STOPPED		10
#define RTOS_ERR_TMR_NO_CALLBACK	11

// RTOS Stop Options
#define RTOS_TMR_OPT_NONE		1
#define RTOS_TMR_OPT_CALLBACK		2
#define RTOS_TMR_OPT_CALLBACK_ARG	3

#define HASH_TABLE_SIZE		10

// Timer Callback
typedef void (*RTOS_TMR_CALLBACK)(void *p_arg);

// OS Timer Object Structure
typedef struct os_timer {
	INT8U	RTOSTmrType;	/* Should Always be set to RTOS_TMR_TYPE for Timers*/

	RTOS_TMR_CALLBACK	RTOSTmrCallback;	/* Function to call when Timer Expires */

	void	*RTOSTmrCallbackArg;	/* Callback Function Arguments */

	struct os_timer	*RTOSTmrNext;	/* Double Link List Pointers */
	struct os_timer	*RTOSTmrPrev;

	INT32U	RTOSTmrMatch;	/* Timer Expires when RTOSTmrTickCtr = RTOSTmrMatch */

	INT32U	RTOSTmrDelay;	/* One Shot Timer - Time for one shot, Periodic Timer - Delay before periodic update starts */

	INT32U	RTOSTmrPeriod;	/* Period to repeat Timer*/

	INT8	*RTOSTmrName;	/* Name to give to the Timer */

	INT8U	RTOSTmrOpt;	/* Timer Options */

	INT8U	RTOSTmrState;	/* State of the Timer
				   RTOS_TMR_STATE_UNUSED
				   RTOS_TMR_STATE_STOPPED
				   RTOS_TMR_STATE_RUNNING
				   RTOS_TMR_STATE_COMPLETED	*/
} RTOS_TMR;

// Hash Table Entry Structure
typedef struct hash_obj {
	INT8U	timer_count;
	RTOS_TMR *list_ptr;
} HASH_OBJ;


// TIMER MANAGER APIs

extern void RTOSTmrInit(void);

extern RTOS_TMR* RTOSTmrCreate(INT32U delay, INT32U period, INT8U option, RTOS_TMR_CALLBACK callback, void *callback_arg, INT8	*name, INT8U *err);

extern INT8U RTOSTmrDel(RTOS_TMR *ptmr, INT8U *perr);

extern INT8* RTOSTmrNameGet(RTOS_TMR *ptmr, INT8U *perr);

extern INT32U RTOSTmrRemainGet(RTOS_TMR *ptmr, INT8U *perr);

extern INT8U RTOSTmrStateGet(RTOS_TMR *ptmr, INT8U *perr);

extern INT8U RTOSTmrStart(RTOS_TMR *ptmr, INT8U *perr);

extern INT8U RTOSTmrStop(RTOS_TMR *ptmr, INT8U opt, void *callback_arg, INT8U *perr);

extern void RTOSTmrSignal(int signum);

// Internal Functions
INT8U Create_Timer_Pool(INT32U timer_count);

void init_hash_table(void);

void insert_hash_entry(RTOS_TMR *timer_obj);

void remove_hash_entry(RTOS_TMR *timer_obj);

void* RTOSTmrTask(void *temp);

RTOS_TMR* alloc_timer_obj(void);

void free_timer_obj(RTOS_TMR *ptmr);

#endif
