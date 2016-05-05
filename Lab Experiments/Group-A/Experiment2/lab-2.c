/*

* Author: Abhinav Gupta, Anant Gupta

* Description: Debouncing using timer interrupts

* Filename: lab-2.c 

* Functions: setup(), ledPinConfig(), switchPinConfig(), main()  

* Global Variables: sw2Status, sw1state, sw2state, ui8LED

*/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

#define DEBOUNCE_DELAY 100000
#define FREQUENCY 1
/*
 ------ Global Variable Declaration
*/
	
uint8_t sw2Status = 0;
uint8_t sw1state = 0;
uint8_t sw2state = 0;
uint8_t ui8LED = 2;

/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set Clock, Enable Peripherals

* Example Call: setup();

*/


void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
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


/*

* Function Name: Timer0IntHandler()

* Input: none

* Output: none

* Description: Interrupt Handler for Timer 0

* Example Call: Not to be called directly

*/

void Timer0IntHandler(void) {

	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) { // Switch is pressed
		if(sw1state == 0) { // Temporary debouncing state
			sw1state = 1;
		}
		else if(sw1state == 1) {
			sw1state = 2;
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8LED); // Turn on LED
			if(ui8LED == 2) // Change next LED color
				ui8LED = 8;
			else
			    ui8LED = ui8LED / 2;
		}
	}
	else {
		sw1state = 0;
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0); // Turn off LED
	}
	if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0) { // Switch is pressed
			if(sw2state == 0) { // Temporary debouncing state
				sw2state = 1;
			} 
			else if(sw2state == 1) {
				sw2state = 2;
				sw2Status++;
			}
	}
	else {
		sw2state = 0;
	}
}

int main(void)
{

    uint32_t ui32Period;
	setup();
	ledPinConfig();
    switchPinConfig();
	/*---------------------

		* Write your code here
		* You can create additional functions

	---------------------*/
    // Timer Configuration
    ui32Period = (SysCtlClockGet() / FREQUENCY);
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);
    while(1) {
    }
}
