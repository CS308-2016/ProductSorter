#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "utils/uartstdio.h"


#define QUESTION_MODE 1

uint32_t ui32ADC0Value[4];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;

volatile int32_t MonitoringTemp = 25;
uint8_t mode = 1;
int32_t value = 0;
uint8_t somethingEntered = 0;
uint8_t negative = 0;
uint8_t char_len = 0;

void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}


void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}

// Enables the Timer
void enableTimer(void) {
	uint32_t ui32Period;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	ui32Period = (SysCtlClockGet() / 1) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);
}

// UART Interrupt handler
void UARTIntHandler(void) {
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
	while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
	{
		char input = UARTCharGetNonBlocking(UART0_BASE);
		if (QUESTION_MODE == 2) {
			if (mode == 1) {
				if (input == 'S') {
					mode = 2;
					value = 0;
					UARTprintf("Enter The Temperature : ");
				}
			} else {
				if (input == 13){
					//Enter character

					MonitoringTemp = value;
					if (negative == 1){
						MonitoringTemp *= -1;
						negative = 0;
					}
					mode = 1;
					somethingEntered = 0;
					char signChar = '\0';
					if (MonitoringTemp < 0){
						signChar = '-';
					}
					char_len = 0;
					UARTprintf("\nSet Temperature updated to %c%d %c%cC\n", signChar, value, 194, 176);
				}
				else if (input == '-' && somethingEntered == 0){
					somethingEntered = 1;
					negative = 1;
					char_len += 1;
					UARTCharPutNonBlocking(UART0_BASE, '-');
				}
				else if (input == 127){
					if (char_len > 0){
						value /= 10;
						char_len -= 1;
						UARTCharPutNonBlocking(UART0_BASE, '\b');
						UARTCharPutNonBlocking(UART0_BASE, ' ');
						UARTCharPutNonBlocking(UART0_BASE, '\b');
					}
					if (char_len == 0){
						negative = 0;
					}
				}
				else {
					int a = input - '0';
					if (a >= 0 && a<= 9) {
						value *= 10;
						value += a;
						somethingEntered = 1;
						char_len += 1;
						UARTCharPutNonBlocking(UART0_BASE, input); //echo character
					} else {
						//Ignore other characters
					}
				}
			}
		}

		SysCtlDelay(SysCtlClockGet() / (50 * 3)); //delay ~1 msec
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
	}
}

// Enables the UART
void enableUART(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	IntMasterEnable();
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}

// Enables the ADC
void enableADC(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);

	ADCSequenceEnable(ADC0_BASE, 1);
}

// Gets the Temperature value and sets the global variables
void getADCReading(void) {
	ADCIntClear(ADC0_BASE, 1);

	ADCProcessorTrigger(ADC0_BASE, 1);

	while(!ADCIntStatus(ADC0_BASE, 1, false)) {
		// Busy Wait
	}
	ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
	ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
	ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
}


void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	if (QUESTION_MODE == 1) {
		getADCReading();
		UARTprintf("Current Temperature %d %c%cC\n", ui32TempValueC, 194, 176);
	} else {
		getADCReading();
		if (mode == 1) {
			UARTprintf("Current Temp %d %c%cC, Set Temp %d %c%cC\n", ui32TempValueC, 194, 176, MonitoringTemp, 194, 176);
			if (MonitoringTemp > ui32TempValueC) {
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 8);
			} else {
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 2);
			}
		}
	}
}

int main(void)
y{
	// Setup the various pins
	setup();
	ledPinConfig();
	enableTimer();
	enableADC();
	enableUART();

	UARTStdioConfig(0, 115200, SysCtlClockGet());

	while(1){
		// Delay between cycles
	}

}
