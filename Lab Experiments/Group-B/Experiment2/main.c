/*

 * Author: Texas Instruments

 * Editted by: Saurav Shandilya, Vishwanathan Iyer
	      ERTS Lab, CSE Department, IIT Bombay

 * Description: This code will familiarize you with using GPIO on TMS4C123GXL microcontroller.

 * Filename: lab-1.c

 * Functions: setup(), ledPinConfig(), switchPinConfig(), main()

 * Global Variables: none

 */
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include <stdio.h>

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

/*
 ------ Global Variable Declaration
 */
uint16_t sw2Status = 0;
bool lastSw2Up = 1;
bool lastSw1Up = 1;

uint8_t ui8Led = 2;
bool sw1Pressed = false;
bool sw2Pressed = false;

/*

 * Function Name: setup()

 * Input: none

 * Output: none

 * Description: Set crystal frequency and enable GPIO Peripherals

 * Example Call: setup();

 */
void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

/*
 * Timer configuration
 */
void timerConfiguration(void){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
}

/*

 * Function Name: ledPinConfig()

 * Input: none

 * Output: none

 * Description: Set PORTF Pin 1, Pin 2, Pin 3 as output.

 * Example Call: ledPinConfig();

 */

void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);  // Pin-1 of PORT F set as output. Modifiy this to use other 2 LEDs.
}

/*

 * Function Name: switchPinConfig()

 * Input: none

 * Output: none

 * Description: Set PORTF Pin 0 and Pin 4 as input. Note that Pin 0 is locked.

 * Example Call: switchPinConfig();

 */
void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0|GPIO_PIN_4;

	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

/**
 * Interrupt Handler
 */
void Timer0IntHandler(void)
{
	uint8_t swButtonValue;


	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	swButtonValue = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);

	if (swButtonValue == 0x10 || swButtonValue == 0x00){

		if (lastSw2Up == 0){
			if (!sw2Pressed){
				sw2Status += 1;
			}
			sw2Pressed = true;
		}
		lastSw2Up = 0;

	} else {
		if (lastSw2Up == 1){
			sw2Pressed = false;
		}
		lastSw2Up = 1;
	}


	if (swButtonValue == 0x00 || swButtonValue == 0x01){

		if (lastSw1Up == 0){
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8Led);
			sw1Pressed = true;
		}
		lastSw1Up = 0;
	}
	else {

		//lastSw1Up = 1;
		if (lastSw1Up == 1){

			if (sw1Pressed){
				if (ui8Led == 2) ui8Led = 8;
				else {
					ui8Led /= 2;
				}

				sw1Pressed = false;
			}

			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
		}
		lastSw1Up = 1;
	}

}

/**
 * Setting up timer
 */
void setUpTimer(void){
	uint32_t ui32Period;
	ui32Period = (SysCtlClockGet() / 100) /2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);
}

int main(void)
{

	setup();
	timerConfiguration();
	ledPinConfig();
	switchPinConfig();
	setUpTimer();

	/*---------------------

	 * Write your code here
	 * You can create additional functions

	---------------------*/

	while(1){

	}

}
