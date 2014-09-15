#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "lcdio.h"

int main(int argc, char **argv)
{
    int fd;
    int ret;
    struct lcd_display_item item;

    if(argc<4) {
        printf("usage: %s text line repeat-times\n", argv[0]);
        exit(1);
    }

    memset(&item, 0, sizeof(item));
    snprintf(item.str, sizeof(item.str), "%s", argv[1]);
    item.display_line = atoi(argv[2]);
    item.repeat_times = atoi(argv[3]);

    fd = open("/proc/lcd1602", O_RDWR);
    if(fd<0) {
        printf("open failed: %s\n", strerror(errno));
        exit(errno);
    }

    ret = ioctl(fd, VIDIOC_DISPLAY_QUEUE, &item);
    if(ret<0) {
        printf("ioctl failed: %s\n", strerror(errno));
    }

    close(fd);
    return 0;
}

