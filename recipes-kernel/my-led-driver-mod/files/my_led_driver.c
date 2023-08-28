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


static struct proc_dir_entry *proc_folder;
static struct proc_dir_entry *proc_file;

static struct gpio_desc *my_led = NULL;
static int led_on = 0;

static ssize_t driver_read(struct file *file, char *user_buffer, size_t count, loff_t *offs) {

	char led_on_char = led_on + '0';

	int status = copy_to_user(user_buffer, &led_on_char, 1);
	printk("LED status: %s\n", led_on ? "on" : "off");

	return status;
}

static ssize_t driver_write(struct file *file, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta = 0;
	char input_char;

	to_copy = min(count, 2);
	not_copied = copy_from_user(&input_char, user_buffer, 1);
	delta = to_copy - not_copied;

	int input = input_char - '0';

	printk("Input received: %d\n", input);

	led_on = input;

	gpiod_set_value(my_led, input);

	return delta;
}

static struct proc_ops fops = {
	.proc_read = driver_read,
	.proc_write = driver_write,
};


static int my_device_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;

	device_for_each_child_node(dev, child) {
		const char *label;
		int gpios[3];
		const char *default_trigger;

		if(fwnode_property_read_string(child, "label", &label)) {
			printk("MLD: Could not read 'label'\n");
			return -1;
		}

		printk("MLD: label: %s\n", label);
		
		if(fwnode_property_read_u32_array(child, "gpios", gpios, 3)) {
			printk("MLD: Could not read 'gpios'\n");
			return -1;
		}
		printk("MLD: gpios: %d %d %d\n", gpios[0], gpios[1], gpios[2]);


		my_led = devm_fwnode_gpiod_get(dev, child, NULL, GPIOD_OUT_HIGH, label);
		if(IS_ERR(my_led)) {
			printk("Error, could not set up GPIO\n");
			return -1;
		}

		// LED starts HIGH
		led_on = 1;

		proc_file = proc_create(label, 0666, proc_folder, &fops);
		if(proc_file == NULL) {
			printk("Error creating file /proc/my-leds/%s\n", label);
			proc_remove(proc_folder);
			gpiod_set_value(my_led, 0);
			gpiod_put(my_led);
			return -ENOMEM;
		}

	}

	return 0;
}

static int my_device_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id my_gpio_led_of_match[] = {
	{ .compatible = "my-gpio-led", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_gpio_led_of_match);

static struct platform_driver my_led_device_driver = {
	.probe = my_device_probe,
	.remove = my_device_remove,
	.driver = {
		.name = "my_gpio_led",
		.of_match_table = my_gpio_led_of_match,
	}
};




static int __init my_module_init(void)
{
	printk("Creating directory\n");
	proc_folder = proc_mkdir("my-leds", NULL);
	if(proc_folder == NULL) {
		printk("Error creating directory /proc/my-leds/\n");
		return -ENOMEM;
	}

	printk("Running probe function\n");
	platform_driver_register(&my_led_device_driver);

	return 0;
}

static void __exit my_module_exit(void)
{
	printk("Running remove function\n");
	gpiod_set_value(my_led, 0);
	gpiod_put(my_led);
	platform_driver_unregister(&my_led_device_driver);

	printk("Removing procfs\n");
	proc_remove(proc_file);
	proc_remove(proc_folder);

}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");


