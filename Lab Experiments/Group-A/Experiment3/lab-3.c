/*

* Author: Abhinav Gupta, Anant Gupta

* Description: Debouncing using timer interrupts

* Filename: lab-3. 

* Functions: setup(), ledPinConfig(), switchPinConfig(), main(), PWMConfig, changeColor(), updateMode()

* Global Variables: ui8LED, ui32Load, ui32PWMClock,  ui8RedAdjust = 251,  ui8BlueAdjust = 1,  ui8GreenAdjust = 1,  speed = 10,  count = 10, colorState, sw1State = 0, sw2State = 0, manualsw1State = 0, manualsw2State = 0, mode = 0, sw1Duration = 0, sw2Duration = 0, sw1Count = 0

*/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "inc/hw_gpio.h"

#define PWM_FREQUENCY 55
// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

#define FREQUENCY 100
#define COLOR_DELTA 5
/*
 ------ Global Variable Declaration
*/
volatile uint32_t ui32Load;
volatile uint32_t ui32PWMClock;
volatile uint8_t ui8RedAdjust = 251;
volatile uint8_t ui8BlueAdjust = 1;
volatile uint8_t ui8GreenAdjust = 1;
volatile uint8_t speed = 10;
volatile uint8_t count = 10;
int colorState;
int sw1State = 0;
int sw2State = 0;
int manualsw1State = 0;
int manualsw2State = 0;
int mode = 0;
int sw1Duration = 0;
int sw2Duration = 0;
int sw1Count = 0;

/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set Clock, Enable Peripherals

* Example Call: setup();

*/

void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
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

* Function Name: PWMConfig()

* Input: none

* Output: none

* Description: Configure PWM pins, generators, and width

* Example Call: PWMConfig();

*/

void PWMConfig(void) {
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);
	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);
	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT | PWM_OUT_6_BIT | PWM_OUT_7_BIT, true);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8RedAdjust * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8BlueAdjust * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8GreenAdjust * ui32Load / 1000);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);
}



/*

* Function Name: changeColor()

* Input: none

* Output: none

* Description: Change the color of the next LED

* Example Call: changeColor();

*/

void changeColor(void) {
	if(colorState == 0) {
		ui8RedAdjust -= COLOR_DELTA;
		ui8GreenAdjust += COLOR_DELTA;
		if(ui8RedAdjust == 1)
			colorState = 1;
	}
	else if(colorState == 1) {
		ui8GreenAdjust -= COLOR_DELTA;
		ui8BlueAdjust += COLOR_DELTA;
		if(ui8GreenAdjust == 1)
			colorState = 2;
	}
	else if(colorState == 2) {
		ui8BlueAdjust -= COLOR_DELTA;
		ui8RedAdjust += COLOR_DELTA;
		if(ui8BlueAdjust == 1)
			colorState = 0;
	}
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8RedAdjust * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8BlueAdjust * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8GreenAdjust * ui32Load / 1000);
}

/*

* Function Name: updateMode()

* Input: none

* Output: none

* Description: Update the mode of the program

* Example Call: updateMode();

*/

void updateMode(void) {

	if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0) {
		if(manualsw2State == 0 || manualsw2State == 1) {
			manualsw2State++;
		}
		else if(manualsw2State == 2)
			sw2Duration = sw2Duration == 200 ? 200 : sw2Duration + 1;

	}
	else {
		manualsw2State = 0;
		if(sw2Duration >= 200) {
			if(sw1Duration >= 200) {
				mode = 3;
			}
			else if(sw1Count == 2) {
				mode = 2;
			}
			else if(sw1Count == 1) {
				mode = 1;
			}
		}
		sw2Duration = 0;
	}

	if(manualsw2State == 2) {
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) {
			if(manualsw1State == 0 || manualsw1State == 1) {
				manualsw1State++;
			}
			else if(manualsw1State == 2)
				sw1Duration = sw1Duration == 200 ? 200 : sw1Duration + 1;
		}
		else {
			if(manualsw1State == 2) {
				if(sw2Duration >= 200 && sw1Duration >= 200) {
					mode = 3;
					sw1Count = 0;
				}
				else
					sw1Count++;
				sw1Duration = 0;
			}
			manualsw1State = 0;
		}
	}
	else {
		manualsw1State = 0;
		sw1Duration = 0;
		sw1Count = 0;
	}
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
	updateMode();
	if((manualsw2State == 2 && manualsw1State == 2) || sw2Duration >= 200)
		return;
	if(mode == 0) {
		if(count == 0) {
			changeColor();
			count = speed;
		}
		else
			count--;
		// Switch 1
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) {
			if(sw1State == 0)
				sw1State = 1;
			else if(sw1State == 1) {
				sw1State = 2;
				speed = speed == 1 ? 1 : speed-1;
			}
		}
		else {
			sw1State = 0;
		}
		// Switch 2
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0) {
			if(sw2State == 0)
				sw2State = 1;
			else if(sw2State == 1) {
				sw2State = 2;
				speed = speed == 20 ? 20 : speed+1;
			}
		}
		else {
			sw2State = 0;
		}
	}
	else {
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) {
			if(sw1State == 0)
				sw1State = 1;
			else if(sw1State == 1) {
				sw1State = 2;
				 switch(mode) {
				 case 1:
					 ui8RedAdjust = ui8RedAdjust == 251 ? 251 : ui8RedAdjust + 5;
					 PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8RedAdjust * ui32Load / 1000);
					 break;
				 case 2:
					 ui8BlueAdjust = ui8BlueAdjust == 251 ? 251 : ui8BlueAdjust + 5;
					 PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8BlueAdjust * ui32Load / 1000);
					 break;
				 case 3:
					 ui8GreenAdjust = ui8GreenAdjust == 251 ? 251 : ui8GreenAdjust + 5;
					 PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8GreenAdjust * ui32Load / 1000);
					 break;
				 }
			}
		}
		else {
			sw1State = 0;
		}
		// Switch 2
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0) {
			if(sw2State == 0)
				sw2State = 1;
			else if(sw2State == 1) {
				sw2State = 2;
				switch(mode) {
				case 1:
					 ui8RedAdjust = ui8RedAdjust == 1 ? 1 : ui8RedAdjust - 5;
					 PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8RedAdjust * ui32Load / 1000);
					 break;
				case 2:
					 ui8BlueAdjust = ui8BlueAdjust == 1 ? 1 : ui8BlueAdjust - 5;
					 PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8BlueAdjust * ui32Load / 1000);
					 break;
				case 3:
					 ui8GreenAdjust = ui8GreenAdjust == 1 ? 1 : ui8GreenAdjust - 5;
					 PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8GreenAdjust * ui32Load / 1000);
					 break;
				}
			}
		}
		else {
			sw2State = 0;
		}
	}
}

int main(void)
{

    uint32_t ui32Period;
	setup();
	//ledPinConfig();
	/*---------------------

		* Write your code here
		* You can create additional functions

	---------------------*/
    switchPinConfig();
    PWMConfig();
    ui32Period = (SysCtlClockGet() / FREQUENCY);
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);

    while(1) {
    }
}
