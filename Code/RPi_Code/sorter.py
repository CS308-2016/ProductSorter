#Author: Dheerendra Rathor
#Filename: sorter.py
#Functions: initialize_camera, wait_for_signal, capture_image, turn_servo_right, reset_servo, signal_green_chip, signal_red_chip, process_image, main
#Global Variables: NONE

from __future__ import print_function

from threading import Event, Thread

import cv2
import serial
import time

import config
import led
from code import Processor


class Sorter(object):

    def __init__(self):
        # noinspection PyArgumentList
        self.camera = None
        self.uart = serial.Serial(config.UART_PATH)
        self.uart.baudrate = config.UART_BAUDRATE
        self.uart.timeout = 10
        self.initialize_camera()
        led.setup()
        self.led_event = Event()

    def initialize_camera(self):
        self.camera = cv2.VideoCapture(0)
        while not self.camera.isOpened():
            pass
        self.camera.set(cv2.cv.CV_CAP_PROP_FRAME_WIDTH, config.WIDTH)
        self.camera.set(cv2.cv.CV_CAP_PROP_FRAME_HEIGHT, config.HEIGHT)
        print('Camera initialized')

    def wait_for_signal(self):
        print('Waiting for signal')
        while True:
            signal_data = self.uart.read(size=1)
            if signal_data == 'c':
                break
        print('Received signal from UART')

    def capture_image(self):
        _, frame = self.camera.read()
        self.camera.release()
        return frame

    def turn_servo_right(self):
        self.uart.write('\x50')

    def reset_servo(self):
        self.uart.write('\x20')

    def signal_green_chip(self):
        led.glow_led(led.green)
        time.sleep(5)
        led.unglow_led(led.green)

    def signal_red_chip(self):
        led.glow_led(led.red)
        self.turn_servo_right()
        time.sleep(1)
        self.reset_servo()
        time.sleep(4)
        led.unglow_led(led.red)

    def process_image(self, image):
        print("Processing Image")
        self.led_event.clear()
        led_thread = Thread(target=led.blink, args=(led.blue, -1, self.led_event))
        led_thread.start()
        processor = Processor(image)
        good_chip = processor.process_image()
        self.led_event.set()
        if good_chip:
            self.signal_green_chip()
        else:
            self.signal_red_chip()


def main():
    print("Sorter running")
    sorter = Sorter()
    while True:
        sorter.wait_for_signal()

        image = sorter.capture_image()
        cv2.imwrite('image.png', image)
        print('shape', image.shape)
        sorter.process_image(image)
        sorter.initialize_camera()

if __name__ == '__main__':
    try:
        main()
    finally:
        led.gpio.cleanup()
