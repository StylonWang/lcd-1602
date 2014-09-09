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

#define DRV_VERSION "2014083001"

struct lcd1602
{

};

static int lcd_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct lcd1602 *lcd1602;

	printk("%s,line:%d\n",__func__, __LINE__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk("%s ENODEV\n", __func__);
		return -ENODEV;
    }

	lcd1602 = kzalloc(sizeof(struct lcd1602), GFP_KERNEL);
	if (!lcd1602) {
        printk("%s ENOMEM\n", __func__);
		return -ENOMEM;
    }

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");

	//this_client = client;
	//this_client->addr = client->addr;

	i2c_set_clientdata(client, lcd1602);

	return 0;
}

static int lcd_remove(struct i2c_client *client)
{
	struct lcd1602 *lcd1602 = i2c_get_clientdata(client);

	kfree(lcd1602);

	return 0;
}

static int lcd_detect(struct i2c_client *client,
                       struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    int address = client->addr;

    printk("%s, addr=%x\n", __func__, address);

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        printk("%s: functionality not match\n", __func__);
        return -ENODEV;
    }

    return -ENODEV;
}

static const struct i2c_device_id lcd_id[] = {
	{ "lcd1602", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lcd_id);

/* Addresses to scan */
static const unsigned short normal_i2c[] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, I2C_CLIENT_END
};

static struct i2c_driver lcd_driver = {
	.class = I2C_CLASS_HWMON, //TODO: any other class?
	.driver			= {
		.name		= "lcd1602",
	},
	.probe			= lcd_probe,
	.remove			= lcd_remove,
    .detect         = lcd_detect,
	.id_table		= lcd_id,
	.address_list	= normal_i2c,
};

static int __init mod_init(void)
{
    printk("LCD 1602 driver %s loaded\n", DRV_VERSION);
    return i2c_add_driver(&lcd_driver);
}

static void __exit mod_exit(void)
{
    i2c_del_driver(&lcd_driver);
    printk("LCD 1602 driver %s removed\n", DRV_VERSION);
}


module_init (mod_init);
module_exit (mod_exit);
