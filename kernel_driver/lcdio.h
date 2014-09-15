#ifndef __LCDIO_H__
#define __LCDIO_H__

// LCD dsplay interface for 1602/2004 I2C LCD module 

#ifdef __KERNEL__
#include <linux/time.h> /* need struct timeval */
#include <linux/compiler.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/list.h>
#else
#include <sys/time.h>
#include <sys/ioctl.h>
#endif

struct lcd_capability
{
    int max_lines;  // legal values: 2, 4
    int max_rows;   // legal values: 16, 20
};

struct lcd_display_item
{
    //struct list_head list;

    int display_line;
    char str[128+1];

    int repeat_times; // 0 for no-repeat(once)

    enum {
        ALIGN_TO_LEFT,
        ALIGN_TO_RIGHT,
    } align;

    enum {
        NO_SCROLL,
        SCROLL_RIGHT_TO_LEFT,
        SCROLL_LEFT_TO_RIGHT,
    } scroll;

    int start_delay_milisec; // delay before scrolling, only applies if scroll is not NO_SCROLL
    int scroll_delay_milisec; // only applies if scroll is not NO_SCROLL
    int loop_delay_milisec;
};

#define VIDIOC_DISPLAY_QUEUE    _IOW('L', 20, struct lcd_display_item)
#define VIDIOC_DISPLAY_NOW      _IOW('L', 21, struct lcd_display_item)
#define VIDIOC_DISPLAY_FLUSH    _IOW('L', 22, int)
#define VIDIOC_LCD_CLEAR        _IOW('L', 23, int)
//#define VIDIOC_LCD_SETCAP       _IOW('L', 24, struct lcd_capability)

#endif // __LCDIO_H__
