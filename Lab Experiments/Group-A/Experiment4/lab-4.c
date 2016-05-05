/*

* Author: Abhinav Gupta, Anant Gupta

* Description: Debouncing using timer interrupts

* Filename: lab-3. 

* Functions: setup(), ledPinConfig(), switchPinConfig(), main(), UARTConfig(), UARTIntHandler(), ADCConfig(), measureTemp()

* Global Variables:  ui32ADC0Value[4], ui32TempAvg, ui32TempC, test = 1, mode = 1, state = 0, setTemp = 25,char c1

*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
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

uint32_t ui32ADC0Value[4];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempC;
volatile int test = 1;
volatile int mode = 1;
volatile int state = 0;
uint32_t setTemp = 25;
volatile char c1;

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
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	//SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
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

* Function Name: UARTConfig()

* Input: none

* Output: none

* Description: Configure UART pins, baud rate, stop bit, parity bit

* Example Call: UARTConfig();

*/
void UARTConfig(void) {
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

/*

* Function Name: ADCConfig()

* Input: none

* Output: none

* Description: Configure ADC Sequencer

* Example Call: ADCConfig();

*/
void ADCConfig(void) {
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 3, ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 1);
}

/*

* Function Name: UARTPrint()

* Input: char * msg

* Output: none

* Description: Print the message given to UART Output

* Example Call: UARTPrint("hello");

*/
void UARTPrint(char * msg) {
	int len = strlen(msg);
	int i;
	for(i = 0; i < len; i++) {
		UARTCharPut(UART0_BASE, msg[i]);
	}
}

void UARTPrintNonBlocking(char * msg) {
	int len = strlen(msg);
	int i;
	for(i = 0; i < len; i++) {
		UARTCharPutNonBlocking(UART0_BASE, msg[i]);
	}
}


/*

* Function Name: measureTemp()

* Input: none

* Output: none

* Description: Trigger ADC Sampler and measure average temperature

* Example Call: measureTemp();

*/
void measureTemp(void) {
	ADCIntClear(ADC0_BASE, 1);
	ADCProcessorTrigger(ADC0_BASE, 1);
	while(!ADCIntStatus(ADC0_BASE, 1, false));
	ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
	ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
	ui32TempC = (1475 - ((2475 * ui32TempAvg)) / 4096) / 10;
}

void Timer0IntHandler(void) {

	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

}

void UARTIntHandler(void)
{
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true);
	UARTIntClear(UART0_BASE, ui32Status);
	if(test == 1)
		return;
	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 2);
	while(UARTCharsAvail(UART0_BASE)) {
		char c = UARTCharGetNonBlocking(UART0_BASE);
		UARTCharPutNonBlocking(UART0_BASE, c);
		if(c == 's') {
			mode = 2;
			UARTPrint("\r\nEnter the temperature : ");
			state = 1;
		}
		else if(mode == 2) {
			if(c >= '0' && c <= '9') {
				state++;
				if(state == 2) {
					c1 = c;
				}
				if(state == 3) {
					setTemp = ((c1 - '0')*10)+(c-'0');
					UARTPrint("\r\nSet Temperature updated to ");
					UARTCharPut(UART0_BASE, c1);
					UARTCharPut(UART0_BASE, c);
					UARTCharPut(UART0_BASE, ' ');
					UARTCharPut(UART0_BASE, 194);
					UARTCharPut(UART0_BASE, 176);
					UARTPrint("C\r\n");
					mode = 1;
					state = 0;
				}

			}
			else {
				UARTPrint("\r\nEnter Numbers Only! Returning to Monitor Mode\r\n");
				mode = 1;
			}
		}
		if(c == '\r')
			UARTCharPutNonBlocking(UART0_BASE, '\n');
	}
	//SysCtlDelay(100);
	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
}

int main(void)
{

    uint32_t ui32Period;
	setup();
	ledPinConfig();
	UARTConfig();
	ADCConfig();
	/*---------------------

		* Write your code here
		* You can create additional functions

	---------------------*/


    ui32Period = (SysCtlClockGet() / FREQUENCY);
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    TimerEnable(TIMER0_BASE, TIMER_A);
    char msg[] = "Welcome to Lab 4\r\n";
    UARTPrint(msg);
    //GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 2);
    //SysCtlDelay(100);
    //GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
    while(1) {
    		if(test == 1) {
    			measureTemp();
			UARTPrint("Current Temperature ");
			UARTCharPut(UART0_BASE, (char)(((int)'0') + (ui32TempC/10)));
			UARTCharPut(UART0_BASE, (char)(((int)'0') + (ui32TempC%10)));
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, 194);
			UARTCharPut(UART0_BASE, 176);
			UARTPrint(" C\r\n");
			SysCtlDelay(SysCtlClockGet() / 3);
    		}
    		else if(test == 2) {
    			if(mode == 1) {
    				measureTemp();
    				UARTPrint("Current Temp = ");
    				UARTCharPut(UART0_BASE, (char)(((int)'0') + (ui32TempC/10)));
    				UARTCharPut(UART0_BASE, (char)(((int)'0') + (ui32TempC%10)));
    				UARTCharPut(UART0_BASE, ' ');
    				UARTCharPut(UART0_BASE, 194);
    				UARTCharPut(UART0_BASE, 176);
   				UARTPrint(" C , Set Temp = ");
   				UARTCharPut(UART0_BASE, (char)(((int)'0') + (setTemp/10)));
   				UARTCharPut(UART0_BASE, (char)(((int)'0') + (setTemp%10)));
   				UARTCharPut(UART0_BASE, ' ');
   				UARTCharPut(UART0_BASE, 194);
   				UARTCharPut(UART0_BASE, 176);
   				UARTPrint(" C\r\n");
   				if(ui32TempC < setTemp) {
   					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_3);
   				}
   				else {
   					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_1);
   				}
				SysCtlDelay(SysCtlClockGet() / 3);
    			}
    		}
    }
}
