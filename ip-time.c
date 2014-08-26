
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "lcddriver.h"

int main(int argc, char **argv)
{
    char *dev = "/dev/i2c-2";
    int fd = open(dev, O_RDWR);
    int ret;
    uint8_t address = 0x27;

    if(fd<0) {
        printf("open: %s\n", strerror(errno));
        exit(1);
    }

    ret = ioctl(fd, I2C_SLAVE, address);
    if(ret<0) {
        printf("set i2c address: %s\n", strerror(errno));
        exit(1);
    }

    lcd_init(fd);

    while(1) {
        time_t tt;
        struct tm ttm;
        char buf[64];

        tt = time(NULL); // get current time
        localtime_r(&tt, &ttm); // convert to localtime

        strftime(buf, sizeof(buf), "%Y-%m-%d", &ttm);
        lcd_display_string(fd, buf, 1);
        strftime(buf, sizeof(buf), "%H:%M:%S", &ttm);
        lcd_display_string(fd, buf, 2);
        sleep(1);
    }

    return 0;
}

