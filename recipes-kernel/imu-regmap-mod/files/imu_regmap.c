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
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/regmap.h>

// REGISTRIES
#define ADXL345_REG_DEVID			0x00
#define ADXL345_REG_POWER_CTL		0x2D
#define ADXL345_REG_DATA_FORMAT		0x31
#define ADXL345_REG_DATAX0			0x32
#define ADXL345_REG_DATAY0			0x34
#define ADXL345_REG_DATAZ0			0x36
#define ADXL345_REG_DATA_AXIS(index)	\
	(ADXL345_REG_DATAX0 + (index) * sizeof(__le16))
// COMMANDS
#define ADXL345_POWER_CTL_MEASURE	BIT(3)
#define ADXL345_POWER_CTL_STANDBY	0x00
// DEVICE ID
#define ADXL345_DEVID				0xE5

static const struct regmap_config imu_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

struct imu_data {
	struct i2c_client *client;
	struct regmap *regmap;
};

#define IMU_CHANNEL(index, axis) {						\
	.type = IIO_ACCEL,									\
	.modified = 1,										\
	.channel2 = IIO_MOD_##axis,							\
	.address = index,									\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
}

static const struct iio_chan_spec imu_channels[] = {
	IMU_CHANNEL(0, X),
	IMU_CHANNEL(1, Y),
	IMU_CHANNEL(2, Z),
};

static int imu_read_raw(struct iio_dev *indio_dev, 
					struct iio_chan_spec const *chan,
					int *val, int *val2, long mask)
{
	struct imu_data *data = iio_priv(indio_dev);
	u16 measurement;
	int ret;

	if(mask == IIO_CHAN_INFO_RAW) {
		ret = regmap_bulk_read(data->regmap, 
							   ADXL345_REG_DATA_AXIS(chan->address), 
							   &measurement, sizeof(measurement));

		if(ret < 0)
			return ret;

		*val = sign_extend32(le16_to_cpu(measurement), 15);
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static const struct iio_info imu_info = {
	.read_raw = imu_read_raw,
};

static int my_imu_probe(struct i2c_client *client)
{
	struct iio_dev *indio_dev;
	struct imu_data *data;
	struct regmap *regmap;
	const char *name = "my_imu";
	int ret;
	printk("IMU - Running probe function\n");


	regmap = devm_regmap_init_i2c(client, &imu_regmap_config);
	if(IS_ERR(regmap)) {
		printk("IMU - error initializing regmap\n");
		return -1;
	}

	if(client->addr != 0X53) {
		printk("IMU - wrong I2C address\n");
		return -1;
	}

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(data));
	if(!indio_dev) {
		printk("IMU - error, out of memory\n");
		return -ENOMEM;
	}
	
	data = iio_priv(indio_dev);
	data->client = client;
	data->regmap = regmap;

	indio_dev->name = name;
	indio_dev->info = &imu_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = imu_channels;
	indio_dev->num_channels = ARRAY_SIZE(imu_channels);

	// Enable IMU measurements
	regmap_write(data->regmap, ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_MEASURE);

	return devm_iio_device_register(&client->dev, indio_dev);
}

static void my_imu_remove(struct i2c_client *client)
{
	printk("IMU - Running remove function\n");
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
		.name = "imu_regmap_driver",
		.of_match_table = my_imu_of_match,
	}
};


module_i2c_driver(my_imu_driver);
MODULE_LICENSE("GPL");


