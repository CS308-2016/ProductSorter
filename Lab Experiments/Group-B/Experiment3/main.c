#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "inc/tm4c123gh6pm.h"

#define PWM_FREQUENCY 55

#define MAX_INTENSITY 254
#define MIN_INTENSITY 1


/**
 * States
 * 0 - idle
 * 1 - pressed
 * 2 - hold
 */

#define AUTO_MODE 0
#define MANUAL_MODE 1
#define MODE_1 11
#define MODE_2 12
#define MODE_3 13

#define MAX_DELAY 100000
#define MIN_DELAY 10000
#define CHANGE_IN_DELAY 10000

volatile uint32_t ui32Load;

int switch_1_state = 0;
int switch_2_state = 0;
int switch_1_state_counter = 0;
int current_mode = AUTO_MODE;
int auto_state_color = 0;
int delay = MAX_DELAY;

bool released = false;

int bothPressed = 0;

uint8_t red_intensity = MAX_INTENSITY, green_intensity = MIN_INTENSITY, blue_intensity = MIN_INTENSITY;

void enablePins(void) {
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

	GPIOPinConfigure(GPIO_PF1_M1PWM5);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);

	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

void increase_intensity(uint8_t* color) {
	if (*color == MAX_INTENSITY) {
		return;
	}
	*color = *color + 1;
}

void decrease_intensity(uint8_t* color) {
	if (*color == MIN_INTENSITY) {
		return;
	}
	*color = *color - 1;
}

void manual_increase_intensity(uint8_t* color) {
	if (*color + 20 > MAX_INTENSITY){
		*color = MAX_INTENSITY;
	}
	else {
		*color = *color + 20;
	}
}

void manual_decrease_intensity(uint8_t* color) {
	if (*color - 20 < MIN_INTENSITY){
		*color = MIN_INTENSITY;
	}
	else {
		*color = *color - 20;
	}
}

void setPWM(){
	PWMGenConfigure(PWM1_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
	PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);

	PWMGenEnable(PWM1_BASE, PWM_GEN_1);
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);
}

void setPWMWidth(uint8_t red_intensity, uint8_t green_intensity, uint8_t blue_intensity){
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, red_intensity * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, blue_intensity * ui32Load / 1000);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, green_intensity * ui32Load / 1000);
}

void check_switch_state(int switch_pin, int* state) {

	int current_state = *state;
	if(GPIOPinRead(GPIO_PORTF_BASE,switch_pin)==0x00)
	{
		if (current_state == 0) {
			*state = 1;
		} else if (current_state == 1) {
			*state = 2;
		}

	} else {
		*state = 0;
	}
}

void timerConfiguration(void){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
}

void setUpTimer(void){
	uint32_t ui32Period;
	ui32Period = (SysCtlClockGet() / 10) /2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);
}


void Timer0IntHandler(void){
	if (current_mode == AUTO_MODE) {
		if (auto_state_color == 0) {
			if (blue_intensity == MIN_INTENSITY) {
				auto_state_color = 1;
			}
			decrease_intensity(&blue_intensity);
			increase_intensity(&red_intensity);
		}
		else if (auto_state_color == 1) {
			if (red_intensity == MIN_INTENSITY) {
				auto_state_color = 2;
			}
			decrease_intensity(&red_intensity);
			increase_intensity(&green_intensity);
		} else if (auto_state_color == 2) {
			if (green_intensity == MIN_INTENSITY) {
				auto_state_color = 0;
			}
			decrease_intensity(&green_intensity);
			increase_intensity(&blue_intensity);
		}
	}

	check_switch_state(GPIO_PIN_4, &switch_1_state);
	check_switch_state(GPIO_PIN_0, &switch_2_state);

	if (current_mode == AUTO_MODE){
		if (switch_1_state == 2 && switch_2_state == 2){
			current_mode = MANUAL_MODE;
		}
		else if (switch_1_state == 1){
			if (delay > MIN_DELAY) {
				delay -= CHANGE_IN_DELAY;
			}

		} else if (switch_2_state == 1){
			if (delay < MAX_DELAY){
				delay += CHANGE_IN_DELAY;
			}
		}
		SysCtlDelay(delay);
	}
	else {
		if (switch_1_state == 2 && switch_2_state == 2){
			if (bothPressed > 50){
				switch_1_state_counter = 0;
			}
			if (released){
				current_mode = MODE_3;
			}
			bothPressed += 1;
		}
		else {
			released = true;
			bothPressed = 0;
			if (switch_1_state == 1 && switch_2_state == 2){

				switch_1_state_counter += 1;
			}
			else if (switch_1_state == 0 && switch_2_state == 0){
				if (switch_1_state_counter == 1){
					current_mode = MODE_1;
				}
				if (switch_1_state_counter > 1){
					current_mode = MODE_2;
				}
				switch_1_state_counter = 0;
			}
			else if (switch_1_state == 1 && switch_2_state == 0){

				if (current_mode == MODE_1){
					manual_increase_intensity(&red_intensity);
				}
				else if (current_mode == MODE_2){
					manual_increase_intensity(&blue_intensity);
				}
				else if (current_mode == MODE_3){
					manual_increase_intensity(&green_intensity);
				}
			}
			else if (switch_2_state == 1 && switch_1_state == 0){
				if (current_mode == MODE_1){
					manual_decrease_intensity(&red_intensity);
				}
				else if (current_mode == MODE_2){
					manual_decrease_intensity(&blue_intensity);
				}
				else if (current_mode == MODE_3){
					manual_decrease_intensity(&green_intensity);
				}
			}
		}
		SysCtlDelay(100000);
	}

	setPWMWidth(red_intensity, green_intensity, blue_intensity);
}

int main(void)
{
	volatile uint32_t ui32PWMClock;


	enablePins();
	timerConfiguration();
	setUpTimer();

	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;

	setPWM();
	setPWMWidth(red_intensity, green_intensity, blue_intensity);

	while(1)
	{

	}

}
