import lcddriver
import time
import datetime
import io
#from time import *

lcd = lcddriver.lcd()

#lcd.lcd_display_string("Hello world", 1)
#lcd.lcd_display_string("picorder", 3)
#lcd.lcd_display_string("I am a Raspberry Pi", 4)

while True:
	lcd.lcd_display_string(datetime.datetime.now().strftime("%Y-%m-%d"), 1)
	lcd.lcd_display_string(datetime.datetime.now().strftime("%H:%M:%S"), 2)
	time.sleep(1)
