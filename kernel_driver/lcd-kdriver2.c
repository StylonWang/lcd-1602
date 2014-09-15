/* Driver to control 1602 LCD module on I2C bus via PCF8574.
 * The driver exposes an interface to user-space application via lcdio.h.
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
#include    <linux/proc_fs.h>
#include    <linux/mutex.h>
#include    <linux/kthread.h>
#include    <linux/list.h>
#include    <linux/slab.h>

#include <asm/uaccess.h>

#include    "lcdio.h"

#define DRV_VERSION "2014091201"

#define DEF_LCD_WIDTH (16)
#define DEF_LCD_HEIGHT (2)

static int g_lcd_width = DEF_LCD_WIDTH;
static int g_lcd_height = DEF_LCD_HEIGHT;

static void i2c_write_cmd(const struct i2c_client *client, uint8_t data)
{
    int ret = i2c_smbus_write_byte(client, data);
    if(ret<0) {
        printk("i2c_smbus_write_byte failed: %d\n", ret);
    }
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

DEFINE_MUTEX(lcd_lock);

static void lcd_strobe(const struct i2c_client *client, uint8_t data)
{
      i2c_write_cmd(client, data | En | LCD_BACKLIGHT);
      //mysleep(1); //.0005);
      udelay(500); //TODO: check spec and make sure this delay is long enough
      i2c_write_cmd(client, ((data & ~En) | LCD_BACKLIGHT));
      //mysleep(1); //.0001);
      udelay(100); //TODO: check spec and make sure this delay is long enough
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

    mutex_lock(&lcd_lock);

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

    for(i=0; i<g_lcd_width /*strlen(str)*/; ++i) {
        if(i<strlen(str))
            lcd_write(client, str[i], Rs);
        else
            lcd_write(client, ' ', Rs); // fill with blank
    }

    mutex_unlock(&lcd_lock);
}

static void lcd_clear(const struct i2c_client *client)
{
    mutex_lock(&lcd_lock);

    lcd_write(client, LCD_CLEARDISPLAY, 0);
    lcd_write(client, LCD_RETURNHOME, 0);

    mutex_unlock(&lcd_lock);
}

static void lcd_init(const struct i2c_client *client)
{
    mutex_lock(&lcd_lock);

    lcd_write(client, 0x03, 0);
    lcd_write(client, 0x03, 0);
    lcd_write(client, 0x03, 0);
    lcd_write(client, 0x02, 0);

    lcd_write(client, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE, 0);
//   lcd_write(client, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_8BITMODE, 0);
    lcd_write(client, LCD_DISPLAYCONTROL | LCD_DISPLAYON, 0);
    lcd_write(client, LCD_CLEARDISPLAY, 0);
    lcd_write(client, LCD_ENTRYMODESET | LCD_ENTRYLEFT, 0);
    msleep(200); //TODO: check spec and make sure this delay is long enough

    mutex_unlock(&lcd_lock);
}

struct lcd_service_item
{
    struct lcd_display_item ditem;
    struct list_head list;
};

struct lcd_service 
{
    int line;

    struct mutex lock;
    wait_queue_head_t waitq;
    struct list_head head; // linked list of lcd_service_item
    struct task_struct *thread;
};

static struct i2c_client *lcd_device = NULL;
static struct lcd_service **lcd_services = NULL;

#define LOOP_DELAY_MAX (1000*20)

static struct proc_dir_entry *lcd_proc_entry = NULL;

static int service_thread_func(void *data)
{
    struct lcd_service *service = (struct lcd_service *)data;

    printk("lcd1602 thread %d started\n", service->line);

    while(1)  {
        struct lcd_service_item *item=NULL, ritem;
        struct list_head *pos, *q;

        wait_event_interruptible(service->waitq, kthread_should_stop() ||
                                    !list_empty(&service->head));

        printk("lcd1602 %d wake up\n", service->line);
        if(kthread_should_stop()) break;

        // retrieve one item from the list
        printk("lock before get item\n"); 
        mutex_lock(&service->lock);
#if 0
        item = list_entry(&service->head, struct lcd_service_item, list);
#else
        list_for_each_safe(pos, q, &service->head) {
            item = list_entry(pos, struct lcd_service_item, list);
            break;
        }
#endif
        if(!item) {
            printk("item is null\n");
            continue;
        }

        // remove item from list if not repeating
        if(item->ditem.repeat_times==0) {
            list_del(&item->list);
        }
        // get a copy of the item so we can continue without locking the list
        else {
            item->ditem.repeat_times--; // decrease repeat count
            ritem = *item; // copy
            item = &ritem;
        }
        mutex_unlock(&service->lock);
        printk("unlock after get item\n"); 

        // now service this item
        // TODO: scroll, align
        // display text
        printk("lcd service %d: %s\n", service->line, item->ditem.str);
        lcd_display_string(lcd_device, item->ditem.str, service->line+1);

        // do loop_delay_milisec
        if(item->ditem.loop_delay_milisec>0 &&
           item->ditem.loop_delay_milisec<LOOP_DELAY_MAX) {
            printk("loop delay %d\n", item->ditem.loop_delay_milisec);
            msleep(item->ditem.loop_delay_milisec); // do loop delay
        }

        if(item != &ritem) {
            kfree(item);
        }
    }

    printk("lcd1602 thread %d stopped\n", service->line);
    return 0;
}

static int start_service_thread(int line)
{
    struct lcd_service *service = (struct lcd_service *)kmalloc(sizeof(struct lcd_service), GFP_KERNEL);

    if(!service) {
        return -ENOMEM;
    }

    // initialize context
    mutex_init(&service->lock);
    INIT_LIST_HEAD(&service->head);
    init_waitqueue_head(&service->waitq);
    service->line = line;

    // create service thread
    service->thread = kthread_run(service_thread_func, service, "lcdservice%d", line);
    if(IS_ERR(service->thread)) {
        int err = PTR_ERR(service->thread);
        printk("unable to create lcd service%d\n", line);
        kfree(service);
        return err;
    }

    lcd_services[line] = service;

    return 0;
}

void stop_service_thread(int line)
{
    struct lcd_service *service = lcd_services[line];

    if(!service) return;
    kthread_stop(service->thread);
    kfree(service);
}

static int lcd_proc_open (struct inode *i, struct file *f)
{
    return 0;
}

static int lcd_proc_release (struct inode *i, struct file *f)
{
    return 0;
}

static int lcd_display_queue(struct lcd_service_item *item)
{
    int line = item->ditem.display_line;
    struct lcd_service *service = lcd_services[line];
    
    //TODO: remove this if proc_write
    item->ditem.str[sizeof(item->ditem.str)-1] = 0; // make sure null-terminated

    mutex_lock(&service->lock);
    list_add_tail(&item->list, &service->head);
    mutex_unlock(&service->lock);

    wake_up(&service->waitq);

    return 0;
}

static int lcd_display_now(struct lcd_service_item *item)
{
    int line = item->ditem.display_line;
    struct lcd_service *service = lcd_services[line];
    
    item->ditem.str[sizeof(item->ditem.str)-1] = 0; // make sure null-terminated

    mutex_lock(&service->lock);
    list_add(&item->list, &service->head);
    mutex_unlock(&service->lock);

    wake_up(&service->waitq);

    return 0;
}

static int lcd_display_flush(int line)
{
    struct lcd_service *service;
    struct list_head *pos, *q;
    struct lcd_service_item *item;

    if(line>=g_lcd_height) {
        return -EINVAL;
    }

    service = lcd_services[line];
    mutex_lock(&service->lock);

    // remove all items on this service line
    list_for_each_safe(pos, q, &service->head) {
        item = list_entry(pos, struct lcd_service_item, list);
        list_del(pos);
        kfree(item);
    }

    mutex_unlock(&service->lock);

    return 0;
}

static long lcd_proc_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
    long ret = -EINVAL;

    switch(cmd) {

        case VIDIOC_DISPLAY_QUEUE:
            printk("%s VIDIOC_DISPLAY_QUEUE\n", __func__);
            {
                struct lcd_service_item *item = kmalloc(sizeof(struct lcd_service_item), GFP_KERNEL);
                int missing = copy_from_user(&item->ditem, (void *)arg, sizeof(item->ditem));
                if(0==missing) {
                    ret = lcd_display_queue(item);
                }
                else {
                    kfree(item);
                    ret = -EFAULT;
                }
            }
            break;

        case VIDIOC_DISPLAY_NOW:
            printk("%s VIDIOC_DISPLAY_NOW\n", __func__);
            {
                struct lcd_service_item *item = kmalloc(sizeof(struct lcd_service_item), GFP_KERNEL);
                int missing = copy_from_user(&item->ditem, (void *)arg, sizeof(item->ditem));
                if(0==missing) {
                    ret = lcd_display_now(item);
                }
                else {
                    kfree(item);
                    ret = -EFAULT;
                }
            }
            break;

        case VIDIOC_DISPLAY_FLUSH:
            printk("%s VIDIOC_DISPLAY_FLUSH\n", __func__);
            {
                int line = (int)arg;
                ret = lcd_display_flush(line);
            }
            break;

        case VIDIOC_LCD_CLEAR:
            printk("%s VIDIOC_LCD_CLEAR\n", __func__);
            lcd_clear(lcd_device);
            ret = 0;
            break;

        default:
            ret = -EINVAL;
            break;
    }

    printk("%s ret=%d\n", __func__, (int)ret);
    return ret;
}

static int parse_str_into_item(struct lcd_service_item *item, char *str)
{
   // string format: "line repeat text"
    char *p = str, *n;
    int err;
    long val;

    n = strsep(&p, " \n");
    if(n) {
        err = kstrtol(n, 10, &val);
        if(err) return err;
        item->ditem.display_line = val-1;
    }
    else return -EINVAL;

    n = strsep(&p, " \n");
    if(n) {
        err = kstrtol(n, 10, &val);
        if(err) return err;
        item->ditem.repeat_times = val;
    }
    else return -EINVAL;

    if(p) {
        strncpy(item->ditem.str, p, sizeof(item->ditem.str));
    }
    else return -EINVAL;

    item->ditem.scroll = SCROLL_RIGHT_TO_LEFT;
    item->ditem.loop_delay_milisec = 1000;

    return 0;
}

static ssize_t lcd_proc_write (struct file *f, const char __user *u, size_t s, loff_t *off)
{
    char buf[128];
    char *c;
    int missing;
    int len = s>sizeof(buf)? sizeof(buf) : s;
    int err;
    struct lcd_service_item *item;

    missing = copy_from_user(buf, u, len);
    if(missing) {
        return -EFAULT;
    }

    buf[len] = 0; // null terminate
    // cut off anything behind the first newline
    c = strnchr(buf, len, '\n');
    if(c) *c = 0;

    printk("%s got %s\n", __func__, buf);
    
    item = kmalloc(sizeof(*item), GFP_KERNEL);
    memset(item, 0, sizeof(*item));

    err = parse_str_into_item(item, buf);
    if(err) return err;

    //item->ditem.display_line = 0;
    //strncpy(item->ditem.str, buf, sizeof(item->ditem.str));

    err = lcd_display_queue(item);
    
    if(err) return err;
    else return s;
}

static struct file_operations lcd_proc_fops = {
    .open = lcd_proc_open,
    .release = lcd_proc_release,
    .compat_ioctl = lcd_proc_ioctl, //TODO: add support for CONFIG_COMPAT for 64bit kernel
    .write = lcd_proc_write,
};

static const struct i2c_board_info lcd_info = {
    I2C_BOARD_INFO("lcd1602", 0x27)
};

static int __init mod_init(void)
{
    struct i2c_adapter *adap;
    int i;

    printk("LCD 1602 driver %s loaded\n", DRV_VERSION);

    //TODO: remove assumption that LCD1602 is always on i2c bus 2
    adap = i2c_get_adapter(2);
    if(!adap) {
        printk("unable to get adapter %d\n", 2);
        return -1;
    }

    //TODO: use i2c_new_probed_device, since LCD1602 can live on other addresses.
    lcd_device = i2c_new_device(adap, &lcd_info);
    if(lcd_device) {
        printk("lcd installed, client=%p\n", lcd_device);
    }
    else {
        printk("lcd not installed\n");
        return 0;
    }

    // init LCD
    lcd_init(lcd_device);
    lcd_clear(lcd_device);
    lcd_display_string(lcd_device, "Hello driver!", 1);

    lcd_services = (struct lcd_service **)kmalloc(sizeof(struct lcd_service *)*g_lcd_height, GFP_KERNEL);
    if(!lcd_services) {
        printk("unable to create lcd service\n");
        //TODO: error handling
    }
    
    for(i=0; i<g_lcd_height; ++i) {
        start_service_thread(i);
    }

    // create /proc/lcd1602 file
    lcd_proc_entry = create_proc_entry("lcd1602", S_IFREG|S_IRUGO|S_IWUGO, NULL);
    if(lcd_proc_entry) {
        lcd_proc_entry->proc_fops = &lcd_proc_fops;
        printk("%s created /proc/lcd1602\n", __func__);
    }
    else {
        printk("%s failed to create proc entry\n", __func__);
    }

    return 0;
}

static void __exit mod_exit(void)
{
    int i;

    printk("%s +\n", __func__);

    remove_proc_entry("lcd1602", NULL); 

    printk("%s stop threads\n", __func__);

    for(i=0; i<g_lcd_height; ++i) {
        stop_service_thread(i);
    }

    kfree(lcd_services);

    printk("%s lcd clear\n", __func__);
    if(lcd_device) lcd_clear(lcd_device);

    if(lcd_device) i2c_unregister_device(lcd_device);
    printk("LCD 1602 driver %s removed\n", DRV_VERSION);
}


module_init (mod_init);
module_exit (mod_exit);

MODULE_AUTHOR("Stylon Wang");
MODULE_DESCRIPTION("LCD1602 driver");
MODULE_LICENSE("GPL");
