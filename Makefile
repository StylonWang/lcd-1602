
CROSS_COMPILE=arm-linux-gnueabihf-

default:: ip-time banner

clean::
	rm -f ip-time

ip-time: ip-time.c lcddriver.c
	$(CROSS_COMPILE)gcc -I/usr/include -g -Wall $^ -o $@

banner: banner.c lcddriver.c
	$(CROSS_COMPILE)gcc -I/usr/include -g -Wall $^ -o $@
