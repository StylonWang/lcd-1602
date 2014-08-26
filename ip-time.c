
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

void i2c_write_cmd(int fd, uint8_t data)
{
    int ret = i2c_smbus_write_byte(fd, data);
    if(ret<0) {
        printf("i2c_smbus_write_byte failed: %d\n", ret);
    }
}

static inline void mysleep(int mili_second)
{
    //TODO: change to select
    usleep(mili_second*1000);
}

// command
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME  0x02
#define LCD_ENTRYMODESET  0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT  0x10
#define LCD_FUNCTIONSET  0x20
#define LCD_SETCGRAMADDR  0x40
#define LCD_SETDDRAMADDR  0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT  0x00
#define LCD_ENTRYLEFT  0x02
#define LCD_ENTRYSHIFTINCREMENT  0x01
#define LCD_ENTRYSHIFTDECREMENT  0x00

// flags for display on/off control
#define LCD_DISPLAYON  0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON  0x02
#define LCD_CURSOROFF  0x00
#define LCD_BLINKON  0x01
#define LCD_BLINKOFF  0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE  0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT  0x04
#define LCD_MOVELEFT  0x00

// flags for function set
#define LCD_8BITMODE  0x10
#define LCD_4BITMODE  0x00
#define LCD_2LINE  0x08
#define LCD_1LINE  0x00
#define LCD_5x10DOTS  0x04
#define LCD_5x8DOTS  0x00

// flags for backlight control
#define LCD_BACKLIGHT  0x08
#define LCD_NOBACKLIGHT  0x00

#define En  0x04 //0b00000100; // Enable bit
#define Rw  0x02 //0b00000010; // Read/Write bit
#define Rs  0x01 //0b00000001; // Register select bit


void lcd_strobe(int fd, uint8_t data)
{
      i2c_write_cmd(fd, data | En | LCD_BACKLIGHT);
      mysleep(1); //.0005);
      i2c_write_cmd(fd, ((data & ~En) | LCD_BACKLIGHT));
      mysleep(1); //.0001);
}

void lcd_write_four_bits(int fd, uint8_t data)
{
      i2c_write_cmd(fd, data | LCD_BACKLIGHT);
      lcd_strobe(fd, data);
}

void lcd_write(int fd, uint8_t cmd, uint8_t mode /*=0*/) 
{
      lcd_write_four_bits(fd, mode | (cmd & 0xF0));
      lcd_write_four_bits(fd, mode | ((cmd << 4) & 0xF0));
}

void lcd_display_string(int fd, char *str, int line)
{
    int i;

    if (line == 1) {
        lcd_write(fd, 0x80, 0);
    }
    if (line == 2) {
        lcd_write(fd, 0xC0, 0);
    }
    if (line == 3) {
        lcd_write(fd, 0x94, 0);
    }
    if (line == 4) {
        lcd_write(fd, 0xD4, 0);
    }

    for(i=0; i<strlen(str); ++i) {
        lcd_write(fd, str[i], Rs);
    }
}

void lcd_clear(int fd)
{
    lcd_write(fd, LCD_CLEARDISPLAY, 0);
    lcd_write(fd, LCD_RETURNHOME, 0);
}

void lcd_init(int fd)
{
    lcd_write(fd, 0x03, 0);
    lcd_write(fd, 0x03, 0);
    lcd_write(fd, 0x03, 0);
    lcd_write(fd, 0x02, 0);

    lcd_write(fd, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE, 0);
//   lcd_write(fd, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_8BITMODE, 0);
    lcd_write(fd, LCD_DISPLAYCONTROL | LCD_DISPLAYON, 0);
    lcd_write(fd, LCD_CLEARDISPLAY, 0);
    lcd_write(fd, LCD_ENTRYMODESET | LCD_ENTRYLEFT, 0);
    mysleep(200);
}

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
        mysleep(1000);
    }

    return 0;
}

