/*
 * chrdev req
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/property.h>

#include "chrdev.h"

/*
 * Platform driver stuff
 */

static int chrdev_req_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	struct module *owner = THIS_MODULE;
	int count, ret;

	count = device_get_child_node_count(dev);
	if (count == 0)
		return -ENODEV;
	if (count > MAX_DEVICES)
		return -ENOMEM;

	device_for_each_child_node(dev, child) {
		const char *label;
		unsigned int id, ro;

		/*
		 * Get device's properties
		 */

		if (fwnode_property_present(child, "reg")) {
			fwnode_property_read_u32(child, "reg", &id);
		} else {
			dev_err(dev, "property \"reg\" not present! Skipped");
			continue;
		}
		if (fwnode_property_present(child, "label")) {
			fwnode_property_read_string(child, "label", &label);
		} else {
			dev_err(dev, "property \"label\" not present! Skipped");
			continue;
		}
		ro = fwnode_property_present(child, "read-only");

		/* Register the new chr device */
		ret = chrdev_device_register(label, id, ro, owner, dev);
		if (ret) {
			dev_err(dev, "unable to register");
		}
	}

	return 0;
}

static int chrdev_req_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	int ret;

	device_for_each_child_node(dev, child) {
		const char *label;
		int id;

		/*
		 * Get device's properties
		 */
			
		if (fwnode_property_present(child, "reg"))
			fwnode_property_read_u32(child, "reg", &id);
		else
			BUG();
		if (fwnode_property_present(child, "label"))
			fwnode_property_read_string(child, "label", &label);
		else
			BUG();

		/* Register the new chr device */
		ret = chrdev_device_unregister(label, id);
		if (ret)
			dev_err(dev, "unable to unregister");
	}

	return 0;
}

static const struct of_device_id of_chrdev_req_match[] = {
	{
		.compatible	= "ldddc,chrdev",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_chrdev_req_match);

static struct platform_driver chrdev_req_driver = {
	.probe	= chrdev_req_probe,
	.remove	= chrdev_req_remove,
	.driver	= {
		.name		= "chrdev-req",
		.of_match_table	= of_chrdev_req_match,
	},
};
module_platform_driver(chrdev_req_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("chrdev request");
