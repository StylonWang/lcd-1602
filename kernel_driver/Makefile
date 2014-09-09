
# for native build
#CROSS_COMPILE=
#KERNELDIR=/lib/modules/`uname -r`/build
#MAKE_OPT=

# cross compile to raspberry pi
CROSS_COMPILE=arm-linux-gnueabihf-
KERNELDIR=/home/stylon/bpi/bananapi-bsp/build/sun7i_defconfig-linux
MAKE_OPT=ARCH=arm

obj-m := lcd1602.o
driver-objs := main.o
lcd1602-objs := lcd-kdriver2.o

default::
	#CROSS_COMPILE=$(CSTOOL_PREFIX) ARCH=arm $(MAKE) -C $(LINUXKERNEL_INSTALL_DIR) SUBDIRS=`pwd` modules
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE_OPT) $(MAKE) -C $(KERNELDIR) SUBDIRS=`pwd` modules
	$(CROSS_COMPILE)strip --strip-debug *.ko

clean::
	$(MAKE) -C $(KERNELDIR) SUBDIRS=`pwd` clean
	
