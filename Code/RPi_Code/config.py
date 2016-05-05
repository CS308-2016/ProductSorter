import json

#The width of the image
WIDTH = 160

#The height of the image
HEIGHT = 120

UART_PATH = '/dev/ttyACM0'
UART_BAUDRATE = 115200

_f = open('refhist', 'r')
GOOD_HIST = json.load(_f)
