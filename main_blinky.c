/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

 /******************************************************************************
  * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
  * do not expect to get real time behaviour from the FreeRTOS Windows port, or
  * this demo application.  Also, the timing information in the FreeRTOS+Trace
  * logs have no meaningful units.  See the documentation page for the Windows
  * port for further information:
  * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
  *
  * NOTE 2:  This project provides two demo applications.  A simple blinky style
  * project, and a more comprehensive test and demo application.  The
  * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
  * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
  * in main.c.  This file implements the simply blinky version.  Console output
  * is used in place of the normal LED toggling.
  *
  * NOTE 3:  This file only contains the source code that is specific to the
  * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
  * in main.c.
  ******************************************************************************/


  /* Standard includes. */
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "time.h"
#include "timers.h"
#include "semphr.h"
#include <Windows.h>
#include <sys/timeb.h>

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )


/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 200UL )
#define mainTIMER_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 2000UL )


/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH					( 2 )

/* The values sent to the queue receive task from the queue send task and the
queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK			( 100UL )
#define mainVALUE_SENT_FROM_TIMER			( 200UL )

/* Task parameters to be generated randomly */
/*Task is ready once every FREQ ticks*/
unsigned short T1FREQ = 0;
unsigned short T2FREQ = 0;

/*Time the task takes to complete*/
unsigned int T1TIME = 0;
unsigned int T2TIME = 0;

/*Priority, to be filled out based on your frequency for RMA*/
unsigned short T1PRIO = 0;
unsigned short T2PRIO = 0;

//static SemaphoreHandle_t safePrint;
//static void safePrintFunc(char* string1) {
//	xSemaphoreTake(safePrint, portMAX_DELAY);
//	printf("%s\n", string1);
//	xSemaphoreGive(safePrint);
//}



/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask(void* pvParameters);
static void prvQueueSendTask(void* pvParameters);
static void T1Task(void* pvParameters);
static void T2Task(void* pvParameters);


/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback(TimerHandle_t xTimerHandle);

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_blinky(void)
{
	//TODO: randomly generate frequency and time for your tasks, and set priority.
	//you can use srand() and rand()
	//safePrintFunc("top of main_blinky");
	printf("top of main_blinky\n");
	srand(time(0));
	T1FREQ = 1 + (rand() % 25);
	T1TIME = 1 + (rand() % T1FREQ);
	T2FREQ = 1 + (rand() % 25);
	T2TIME = 1 + (rand() % T1FREQ);

	if (T1FREQ > T2FREQ) {
		T1PRIO = tskIDLE_PRIORITY + 1;
		T2PRIO = tskIDLE_PRIORITY + 2;

	}
	else {
		T1PRIO = tskIDLE_PRIORITY + 2;
		T2PRIO = tskIDLE_PRIORITY + 1;
	}
	printf("Task, Freq, Time, Prio \r\n");
	printf("T1, %hu, %hu, %hu\r\n", T1FREQ, T1TIME, T1PRIO);
	printf("T2, %hu, %hu, %hu\r\n", T2FREQ, T2TIME, T2PRIO);

	const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

	/* Create the queue. */
	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(uint32_t));
	if (xQueue != NULL)
	{
		printf("xqueue is not null\n");
		/*Create your tasks*/
		xTaskCreate(T1Task, "T1", configMINIMAL_STACK_SIZE * 4, NULL, T1PRIO, NULL);
		xTaskCreate(T2Task, "T2", configMINIMAL_STACK_SIZE * 4, NULL, T2PRIO, NULL);
		/* Create the software timer, but don't start it yet. */
		xTimer = xTimerCreate("Timer",				/* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
			xTimerPeriod,		/* The period of the software timer in ticks. */
			pdFALSE,			/* xAutoReload is set to pdFALSE, so this is a one-shot timer. */
			NULL,				/* The timer's ID is not used. */
			prvQueueSendTimerCallback);/* The function executed when the timer expires. */

		xTimerStart(xTimer, 0); /* The scheduler has not started so use a block time of 0. */

		/* Start the tasks and timer running. */
		vTaskStartScheduler();
	}

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	for (;; );
}
/*-----------------------------------------------------------*/

static void T1Task(void* pvParameters) {
	TickType_t xNextWakeTime;
	TickType_t xBlockTime = T1FREQ;
	(void)pvParameters;
	xNextWakeTime = xTaskGetTickCount();
	//TODO: any other variables to keep track of things.
	int i = 0;
	int count = 0;
	TickType_t initialTick = xTaskGetTickCount();
	for (;; )
	{
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		volatile TickType_t beginTicks = xTaskGetTickCount();
		count++;
		if (count == 1) {
			initialTick = beginTicks;
		}
		//TODO: write your tasks to take TIME number of ticks to finish.
		TickType_t ticksOutLoop = beginTicks;
		while (i <= T2TIME) {
			TickType_t ticks;
			ticks = xTaskGetTickCount();
			if ((ticks - ticksOutLoop) >= 1) {
				i++;
				ticksOutLoop = ticks;
			}
		}
		//TODO: write code to check deadline miss for T1task
		printf("Task 1 completed\n");
		if (ticksOutLoop > (initialTick + (T1FREQ * count))) {
			printf("Task 1 missed deadline!\n");
		}
	}
}

static void T2Task(void* pvParameters) {
	TickType_t xNextWakeTime;
	TickType_t xBlockTime = T2FREQ;
	(void)pvParameters;
	xNextWakeTime = xTaskGetTickCount();
	//TODO: any other variables to keep track of things.
	int j = 0;
	int count2 = 0;
	TickType_t initialTick = xTaskGetTickCount();
	for (;; )
	{
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);
		volatile TickType_t beginTicks = xTaskGetTickCount();
		count2++;
		if (count2 == 1) {
			initialTick = beginTicks;
		}
		//TODO: write your tasks to take TIME number of ticks to finish.
		TickType_t ticksOutLoop = beginTicks;
		while (j <= T2TIME) { 
			TickType_t ticks;
			ticks = xTaskGetTickCount();
			if ((ticks - ticksOutLoop) >= 1) {
				j++;
				ticksOutLoop = ticks;
			}
			//safePrintFunc("Hi"); 
		}
		//TODO: write code to check deadline miss for T1task
		printf("Task 2 completed\n");
		if (ticksOutLoop > initialTick + T2FREQ*count2) {
			printf("Task 2 missed deadline!\n");
		}
	}
}


/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback(TimerHandle_t xTimerHandle)
{
	const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	(void)xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend(xQueue, &ulValueToSend, 0U);
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask(void* pvParameters)
{
	uint32_t ulReceivedValue;

	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	for (;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
		Blocked state. */
		xQueueReceive(xQueue, &ulReceivedValue, portMAX_DELAY);

		/*  To get here something must have been received from the queue, but
		is it an expected value?  Normally calling printf() from a task is not
		a good idea.  Here there is lots of stack space and only one task is
		using console IO so it is ok.  However, note the comments at the top of
		this file about the risks of making Windows system calls (such as
		console output) from a FreeRTOS task. */
		if (ulReceivedValue == mainVALUE_SENT_FROM_TASK)
		{
			printf("Message received from task\r\n");
		}
		else if (ulReceivedValue == mainVALUE_SENT_FROM_TIMER)
		{
			printf("Message received from software timer\r\n");
		}
		else
		{
			printf("Unexpected message\r\n");
		}

		/* Reset the timer if a key has been pressed.  The timer will write
		mainVALUE_SENT_FROM_TIMER to the queue when it expires. */
		if (_kbhit() != 0)
		{
			/* Remove the key from the input buffer. */
			(void)_getch();

			/* Reset the software timer. */
			xTimerReset(xTimer, portMAX_DELAY);
		}
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTask(void* pvParameters)
{
	TickType_t xNextWakeTime;
	const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
	const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for (;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil(&xNextWakeTime, xBlockTime);

		/* Send to the queue - causing the queue receive task to unblock and
		write to the console.  0 is used as the block time so the send operation
		will not block - it shouldn't need to block as the queue should always
		have at least one space at this point in the code. */
		xQueueSend(xQueue, &ulValueToSend, 0U);
	}
}