/*
 *
 *
*/

#include    <linux/types.h>
#include    <linux/version.h>
#include	<linux/module.h>
#include	<linux/spinlock.h>
#include	<linux/sched.h>
#include    <linux/slab.h>
#include    <linux/i2c.h>
#include    <linux/delay.h>

#define DRV_VERSION "2014090901"

#define LCD_WIDTH (16)
#define LCD_HEIGHT (2)

static void i2c_write_cmd(const struct i2c_client *client, uint8_t data)
{
    int ret = i2c_smbus_write_byte(client, data);
    if(ret<0) {
        printk("i2c_smbus_write_byte failed: %d\n", ret);
    }
}

static inline void mysleep(int mili_second)
{
    msleep(mili_second);
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


static void lcd_strobe(const struct i2c_client *client, uint8_t data)
{
      i2c_write_cmd(client, data | En | LCD_BACKLIGHT);
      mysleep(1); //.0005);
      i2c_write_cmd(client, ((data & ~En) | LCD_BACKLIGHT));
      mysleep(1); //.0001);
}

static void lcd_write_four_bits(const struct i2c_client *client, uint8_t data)
{
      i2c_write_cmd(client, data | LCD_BACKLIGHT);
      lcd_strobe(client, data);
}

static void lcd_write(const struct i2c_client *client, uint8_t cmd, uint8_t mode /*=0*/) 
{
      lcd_write_four_bits(client, mode | (cmd & 0xF0));
      lcd_write_four_bits(client, mode | ((cmd << 4) & 0xF0));
}

static void lcd_display_string(const struct i2c_client *client, char *str, int line)
{
    int i;

    if (line == 1) {
        lcd_write(client, 0x80, 0);
    }
    if (line == 2) {
        lcd_write(client, 0xC0, 0);
    }
    if (line == 3) {
        lcd_write(client, 0x94, 0);
    }
    if (line == 4) {
        lcd_write(client, 0xD4, 0);
    }

    for(i=0; i<LCD_WIDTH /*strlen(str)*/; ++i) {
        if(i<strlen(str))
            lcd_write(client, str[i], Rs);
        else
            lcd_write(client, ' ', Rs); // fill with blank
    }
}

static void lcd_clear(const struct i2c_client *client)
{
    lcd_write(client, LCD_CLEARDISPLAY, 0);
    lcd_write(client, LCD_RETURNHOME, 0);
}

static void lcd_init(const struct i2c_client *client)
{
    lcd_write(client, 0x03, 0);
    lcd_write(client, 0x03, 0);
    lcd_write(client, 0x03, 0);
    lcd_write(client, 0x02, 0);

    lcd_write(client, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE, 0);
//   lcd_write(client, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_8BITMODE, 0);
    lcd_write(client, LCD_DISPLAYCONTROL | LCD_DISPLAYON, 0);
    lcd_write(client, LCD_CLEARDISPLAY, 0);
    lcd_write(client, LCD_ENTRYMODESET | LCD_ENTRYLEFT, 0);
    mysleep(200);
}

static struct i2c_client *lcd_device;
static const struct i2c_board_info lcd_info = {
    I2C_BOARD_INFO("lcd1602", 0x27)
};

static int __init mod_init(void)
{
    struct i2c_adapter *adap;

    printk("LCD 1602 driver %s loaded\n", DRV_VERSION);

    adap = i2c_get_adapter(2);
    if(!adap) {
        printk("unable to get adapter %d\n", 2);
        return -1;
    }

    //TODO: use i2c_new_probed_device
    lcd_device = i2c_new_device(adap, &lcd_info);
    if(lcd_device) {
        printk("lcd installed, client=%p\n", lcd_device);
    }
    else {
        printk("lcd not installed\n");
        return 0;
    }

    //TODO: init LCD 
    lcd_init(lcd_device);
    lcd_clear(lcd_device);
    lcd_display_string(lcd_device, "Hello driver!", 1);

    return 0;
}

static void __exit mod_exit(void)
{
    if(lcd_device) i2c_unregister_device(lcd_device);

    printk("LCD 1602 driver %s removed\n", DRV_VERSION);
}


module_init (mod_init);
module_exit (mod_exit);

MODULE_AUTHOR("Stylon Wang");
MODULE_DESCRIPTION("LCD1602 driver");
MODULE_LICENSE("GPL");
