#!/usr/bin/env python

import lcddriver
import time
import datetime
import io
import os
import signal
import sys
import getip

# signal handler
def signal_handler(signal, frame):
        print('quit')
	lcd.lcd_clear()
        sys.exit(0)

lcd = lcddriver.lcd()

# register signal handler to clear screen on quit
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# change working directory to where this script resides, so
# getip.sh can be located and executed.
#script_path = sys.argv[0]
#script_path_dir = os.path.dirname(script_path)
#print "cd into " + script_path_dir
#os.chdir(script_path_dir)

while True:
	#lcd.lcd_display_string(datetime.datetime.now().strftime("%Y-%m-%d"), 1)
	lcd.lcd_display_string(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), 1)
	
	#f = os.popen("sh getip.sh")
	#str = f.read(32)

	lcd.lcd_display_string("" + getip.get_ip(), 2)

	time.sleep(5)
