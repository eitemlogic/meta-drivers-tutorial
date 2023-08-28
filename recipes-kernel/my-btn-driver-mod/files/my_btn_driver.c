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
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/pm_wakeirq.h>

static struct work_struct my_work;
static int setup_irq = 0;

static int my_device_suspend(struct device *dev);
// static int my_device_resume(struct device *dev);

static void my_work_handler(struct work_struct *work) {
	printk(KERN_INFO "Doing a work task\n");
	pr_debug("This is a dynamic debug from work\n");
}

static unsigned int irq_number;
static struct gpio_desc *my_btn = NULL;

static irq_handler_t btn_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
	printk("Btn interrupt triggered!\n");
	pr_debug("This is an example dynamic debug message\n");
	pr_debug("from the button handler\n");
	schedule_work(&my_work);

	return (irq_handler_t) IRQ_HANDLED;
}


static int my_device_probe(struct platform_device *pdev)
{
	pr_debug("This is a probe dynamic debug message\n");

	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;

	device_for_each_child_node(dev, child) {
		const char *label;
		int gpios[3];
		int debounce_interval;

		if (fwnode_property_read_string(child, "label", &label)) {
			printk("MBD: Could not read 'label'\n");
			return -1;
		}
		printk("MBD: label: %s\n", label);
		
		if (fwnode_property_read_u32_array(child, "gpios", gpios, 3)) {
			printk("MBD: Could not read 'gpios'\n");
			return -1;
		}
		printk("MBD: gpios: %d %d %d\n", gpios[0], gpios[1], gpios[2]);

		if (fwnode_property_read_u32(child, "debounce-interval", &debounce_interval)) {
			printk("MBD: Could not read 'debounce-interval'\n");
			return -1;
		}
		printk("MBD: debounce-interval: %d\n", debounce_interval);




		my_btn = devm_fwnode_gpiod_get(dev, child, NULL, GPIOD_IN, label);
		if(IS_ERR(my_btn)) {
			printk("Error, could not set up GPIO\n");
			return -1;
		}



		irq_number = gpiod_to_irq(my_btn);

		gpiod_set_debounce(my_btn, debounce_interval * 1000);

		if(request_irq(irq_number, 
					   (irq_handler_t) btn_irq_handler, 
					   IRQF_TRIGGER_FALLING, 
					   "my_btn_irq", NULL) != 0) {
			printk("Cannot request interrupt nr: %d\n", irq_number);
			free_irq(irq_number, NULL);
			gpiod_put(my_btn);
			return -1;
		}

		INIT_WORK(&my_work, my_work_handler);

		printk("IRQ number: %d\n", irq_number);

		device_init_wakeup(dev, true);
	}

	return 0;
}

static int my_device_remove(struct platform_device *pdev)
{
	pr_debug("This is a remove dynamic debug message\n");
	return 0;
}

static int my_device_suspend(struct device *dev)
{
	int error;
	printk("Configuring btn as wakeup source\n");
	printk("IRQ number: %d\n", irq_number);
	
	if (!device_may_wakeup(dev)) {
		printk("Device may not wake up?\n");
		return -1;
	}

	error = enable_irq_wake(irq_number);
	if (error) {
		printk(KERN_ERR "Failed to configure irq as wakeup source\n");
		return error;
	}

	printk("Finished configuration\n");

	return 0;
}

static int my_device_resume(struct device *dev)
{
	int error;
	printk("Disabling btn as wakeup source\n");

	error = disable_irq_wake(irq_number);
	if (error) {
		printk("Failed to disable irq as wakeup source\n");
		return error;
	}

	return 0;
}

static struct dev_pm_ops btn_pm_ops = {
	.prepare = my_device_suspend,
	// .suspend = my_device_suspend,	// seems to be called too late?
	.resume = my_device_resume,
};

static struct of_device_id my_gpio_led_of_match[] = {
	{ .compatible = "my-gpio-button", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_gpio_led_of_match);

static struct platform_driver my_btn_device_driver = {
	.probe = my_device_probe,
	.remove = my_device_remove,
	.driver = {
		.name = "my_gpio_button",
		.pm = &btn_pm_ops,
		.of_match_table = my_gpio_led_of_match,
	}
};


static int __init my_module_init(void)
{
	printk("Running probe function\n");
	platform_driver_register(&my_btn_device_driver);

	return 0;
}

static void __exit my_module_exit(void)
{
	printk("Running remove function\n");
	flush_scheduled_work();
	// disable_irq_wake(irq_number);
	free_irq(irq_number, NULL);
	gpiod_set_value(my_btn, 0);
	gpiod_put(my_btn);
	platform_driver_unregister(&my_btn_device_driver);
}

late_initcall(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");


