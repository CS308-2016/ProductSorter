#Author: Siddharth Patel
#Filename: led.py
#Functions: setup, print, glow_led, unglow_led, main
#Global Variables: red, green, blue, wait

"""
Circuit:
    +Ve leg, longest one Anode: 3V power with resistors in series
    
    GPIO 2 (PIN 3) => RED
    GPIO 3 (PIN 5) => GREEN
    GPIO 4 (PIN 7) => BLUE
"""

import RPi.GPIO as gpio
import time

red = 2
green = 3
blue = 4
wait = 5


def setup():
    gpio.setmode(gpio.BCM)
    gpio.setup(red, gpio.OUT)
    gpio.setup(green, gpio.OUT)
    gpio.setup(blue, gpio.OUT)
    gpio.output(red, gpio.HIGH)
    gpio.output(green, gpio.HIGH)
    gpio.output(blue, gpio.HIGH)


def blink(color, count, abort, cleanup=False):
    counter = 0 if count >= 0 else -100
    while not abort.isSet() and counter < count:
        if count >= 0:
            counter += 1
        glow_led(color)
        time.sleep(.25)
        unglow_led(color)
        time.sleep(.25)
    if cleanup:
        gpio.cleanup()
        


def glow_led(color):
    gpio.output(color, gpio.LOW)


def unglow_led(color):
    gpio.output(color, gpio.HIGH)


def main():
    setup()
    while True:
        for color in [red, green, blue]:
            #print "Glowing color is {}".format(color)
            glow_led(color)
            time.sleep(wait)
            unglow_led(color)

if __name__ == '__main__':
    try:
        main()
    except:
        gpio.cleanup()


