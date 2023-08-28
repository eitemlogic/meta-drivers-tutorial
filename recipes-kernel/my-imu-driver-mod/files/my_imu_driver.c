/******************************************************************************
 *
 *   Copyright (C) 2011  Intel Corporation. All rights reserved.
 *
 *   SPDX-License-Identifier: GPL-2.0-only
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>

// REGISTRIES
#define ADXL345_REG_DEVID			0x00
#define ADXL345_REG_POWER_CTL		0x2D
#define ADXL345_REG_DATA_FORMAT		0x31
#define ADXL345_REG_DATAX0			0x32
#define ADXL345_REG_DATAY0			0x34
#define ADXL345_REG_DATAZ0			0x36
// COMMANDS
#define ADXL345_POWER_CTL_MEASURE	BIT(3)
#define ADXL345_POWER_CTL_STANDBY	0x00
// DEVICE ID
#define ADXL345_DEVID				0xE5

static struct proc_dir_entry *proc_folder;
static struct proc_dir_entry *proc_file;

static struct i2c_client *imu_client;

static struct gpio_desc *my_led = NULL;
static int led_on = 0;

static ssize_t driver_read(struct file *file, char *user_buffer, size_t count, loff_t *offs) {
	
	char return_string[50] = "";
	u8 raw_data[6];
	i2c_smbus_read_i2c_block_data(imu_client, ADXL345_REG_DATAX0, 6, raw_data);

	u16 x16 = (raw_data[1] << 8) | raw_data[0];
	u16 y16 = (raw_data[3] << 8) | raw_data[2];
	u16 z16 = (raw_data[5] << 8) | raw_data[4];

	int x = sign_extend32(le16_to_cpu(x16), 15);
	int y = sign_extend32(le16_to_cpu(y16), 15);
	int z = sign_extend32(le16_to_cpu(z16), 15);

	// printk("IMU - Values [x y z]: %5d %5d %5d\n", x, y, z);
	sprintf(return_string, "[x y z]: %5d %5d %5d\n", x, y, z);
	copy_to_user(user_buffer, return_string, sizeof(return_string));

	return count;
}

static ssize_t driver_write(struct file *file, const char *user_buffer, size_t count, loff_t *offs) {

	printk("Doing write (NOT IMPLEMENTED)\n");

	return count;
}

static struct proc_ops fops = {
	.proc_read = driver_read,
	.proc_write = driver_write,
};


static int my_imu_probe(struct i2c_client *client)
{
	// alternative: dev_notice, dev_err etc.
	printk("IMU - Running probe function\n");

	if(client->addr != 0X53) {
		printk("IMU - wrong I2C address\n");
		return -1;
	}

	const char *label = "my-imu";
	const char *directory = "my-imus";
	imu_client = client;

	printk("Creating directory\n");
	proc_folder = proc_mkdir(directory, NULL);
	if(proc_folder == NULL) {
		printk("Error creating directory /proc/%s/\n", directory);
		return -ENOMEM;
	}

	proc_file = proc_create(label, 0666, proc_folder, &fops);
	if(proc_file == NULL) {
		printk("Error creating file /proc/%s/%s\n", directory, label);
		proc_remove(proc_folder);
		return -ENOMEM;
	}

	// Enable IMU measurements
	i2c_smbus_write_byte_data(imu_client, 
							  ADXL345_REG_POWER_CTL,
							  ADXL345_POWER_CTL_MEASURE);

	return 0;
}

static void my_imu_remove(struct i2c_client *client)
{
	printk("IMU - Running remove function\n");
	proc_remove(proc_folder);
}

static struct i2c_device_id my_imu_id[] = {
	{"my_imu", 0},
	{ },
};

static struct of_device_id my_imu_of_match[] = {
	{ .compatible = "adi,adxl345", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_imu_of_match);

static struct i2c_driver my_imu_driver = {
	.probe_new = my_imu_probe,
	.remove = my_imu_remove,
	.id_table = my_imu_id,
	.driver = {
		.name = "my_imu_driver",
		.of_match_table = my_imu_of_match,
	}
};


module_i2c_driver(my_imu_driver);
MODULE_LICENSE("GPL");


