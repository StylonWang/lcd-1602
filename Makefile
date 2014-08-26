
default:: ip-time

ip-time: ip-time.c lcddriver.c
	$(CROSS_COMPILE)gcc -I/usr/include -g -Wall $^ -o $@
