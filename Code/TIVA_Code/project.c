

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


volatile int color = 0;
volatile int count = 0;
volatile int state = 0;
volatile char c1;
volatile uint32_t load;
volatile uint32_t PWMClock;
volatile uint8_t adjust = 200;
void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
}


void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);  // Pin-1 of PORT F set as output. Modifiy this to use other 2 LEDs.
}

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

void UARTConfig(void) {
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

void PWMConfig(void) {
	GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
	GPIOPinConfigure(GPIO_PD0_M1PWM0);
	PWMClock = SysCtlClockGet() / 64;
	load = (PWMClock / PWM_FREQUENCY) - 1;
	PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, load);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, adjust * load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
	PWMGenEnable(PWM1_BASE, PWM_GEN_0);
}

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


void Timer0IntHandler(void) {

	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, color);
	if(count <= 0)
		color = 0;
	else
		count--;
	if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) {
		if(state == 0)
			state += 1;
		else if(state == 1) {
			state += 1;
			UARTCharPutNonBlocking(UART0_BASE, 'c');
		}
	}
	else
		state = 0;
}

void UARTIntHandler(void)
{
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true);
	UARTIntClear(UART0_BASE, ui32Status);

	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 2);
	while(UARTCharsAvail(UART0_BASE)) {
		char c = UARTCharGetNonBlocking(UART0_BASE);
		//UARTCharPutNonBlocking(UART0_BASE, c);
		//if(c == '\r')
		//	UARTCharPutNonBlocking(UART0_BASE, '\n');
		if(c == 'g') {
			color = 8;
			count = 10;
		}
		else if(c == 'b') {
			color = 2;
			count = 10;
		}
		adjust = c;
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, adjust*load/1000);
	}
	//SysCtlDelay(100);
	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
}

int main(void)
{

    uint32_t ui32Period;
	setup();
	ledPinConfig();
	switchPinConfig();
	UARTConfig();

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

    PWMConfig();
    while(1) {
    }
}
