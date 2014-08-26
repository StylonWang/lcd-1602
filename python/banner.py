#!/usr/bin/env python

import lcddriver
import time
import datetime
import io
import os
import signal
import sys
import getip

scroll_delay=0.2

# signal handler
def signal_handler(signal, frame):
        print('quit')
	lcd.lcd_clear()
        sys.exit(0)

def get_fortune():
	f = os.popen("/usr/games/fortune")
	try:
		l = f.readline()
		f.close()
	except:
		f.close()
		return ''
	return l.rstrip('\n')

lcd = lcddriver.lcd()
fortune = get_fortune()
loop_time = 0
ip = getip.get_ip()

# register signal handler to clear screen on quit
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

while True:
	lcd.lcd_display_string(ip, 2)

	str = fortune

	# scroll the string with 16-byte capacity
	for i in range(0, len(str)-1):
		substr = str[i:16+i]
		substr = substr + (" "*(16-len(substr)))
		lcd.lcd_display_string(substr, 1)
		time.sleep(scroll_delay)
		
		loop_time += scroll_delay

		if loop_time>5:
			print("time to get ip and new fortune\n")
			loop_time = 0

			ip = getip.get_ip()
			ip = ip + (" "*(16-len(ip)))
			lcd.lcd_display_string(ip, 2)
			fortune = get_fortune()
