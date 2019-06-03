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
#include <linux/platform_device.h>
#include <linux/firmware.h>

#include "chrdev.h"

#define FIRMWARE_VER	"1.0.0"
#define FIRMWARE_NLEN	128

static inline char printable(char c)
{
	return (c < ' ') || (c > '~') ? '-' : c;
}

static void dump_data(const u8 *buf, size_t len)
{
	int n, i;

	for (n = 0; n < len; n += i) {
		pr_info("");
		for (i = 0; (i < 8) && (n + i < len); i++)
			pr_cont("%02x[%c] ", buf[n + i], printable(buf[n + i]));
		pr_cont("\n");
	}
}

/*
 * Firmware loading callback
 */

static void chrdev_fw_cb(const struct firmware *fw, void *context)
{
	struct device *dev = context;

	dev_info(dev, "firmware callback executed!\n");
	if (!fw) {
		dev_err(dev, "unable to load firmware\n");
		return;
	}

	dump_data(fw->data, fw->size);

	/* Firmware data has been read, now we can release it */
	release_firmware(fw);
}

/*
 * Firmware loading functions
 */

static int chrdev_load_fw_wait(struct device *dev, const char *file)
{
	char fw_name[FIRMWARE_NLEN];
	const struct firmware *fw;
	int ret;

	/* Compose firmware filename */
	if (strlen(file) > (128 - 6 - sizeof(FIRMWARE_VER)))
		return -EINVAL;
	sprintf(fw_name, "%s-%s.bin", file, FIRMWARE_VER);

	/* Do the firmware request */
	ret = request_firmware(&fw, fw_name, dev);
	if (ret) {
		dev_err(dev, "unable to load firmware\n");
		return ret;
	}

	dump_data(fw->data, fw->size);

	/* Firmware data has been read, now we can release it */
	release_firmware(fw);

	return 0;
}

static int chrdev_load_fw_nowait(struct device *dev, const char *file)
{
	char fw_name[FIRMWARE_NLEN];
	int ret;

	/* Compose firmware filename */
	if (strlen(file) > (128 - 6 - sizeof(FIRMWARE_VER)))
		return -EINVAL;
	sprintf(fw_name, "%s-%s.bin", file, FIRMWARE_VER);

	/* Do the firmware request */
	ret = request_firmware_nowait(THIS_MODULE, false, fw_name, dev,
			GFP_KERNEL, dev, chrdev_fw_cb);
	if (ret) {
		dev_err(dev,
			"unable to register call back for firmware loading\n");
		return ret;
	}

	return 0;
}

/*
 * Platform driver stuff
 */

static int chrdev_req_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct fwnode_handle *fwh = of_fwnode_handle(np);
	struct module *owner = THIS_MODULE;
	const char *file;
	int ret = 0;

	/* Read device properties */
	if (fwnode_property_read_string(fwh, "firmware", &file)) {
		dev_err(dev, "unable to get property \"firmware\"!");
		return -EINVAL;
	}

	/* Load device firmware */
	if (of_device_is_compatible(np, "ldddc,chrdev-fw_wait"))
		ret = chrdev_load_fw_wait(dev, file);
	else if (of_device_is_compatible(np, "ldddc,chrdev-fw_nowait"))
		ret = chrdev_load_fw_nowait(dev, file);
	if (ret)
		return ret;

	/* Register the new chr device */
	ret = chrdev_device_register("chrdev-fw", 0, 0, owner, dev);
	if (ret) {
		dev_err(dev, "unable to register");
		return ret;
	}

	return 0;
}

static int chrdev_req_remove(struct platform_device *pdev)
{
	return chrdev_device_unregister("chrdev-fw", 0);
}

static const struct of_device_id of_chrdev_req_match[] = {
	{
		.compatible	= "ldddc,chrdev-fw_wait",
	},
	{
		.compatible	= "ldddc,chrdev-fw_nowait",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_chrdev_req_match);

static struct platform_driver chrdev_req_driver = {
	.probe	= chrdev_req_probe,
	.remove	= chrdev_req_remove,
	.driver	= {
		.name		= "chrdev-fw",
		.of_match_table	= of_chrdev_req_match,
	},
};
module_platform_driver(chrdev_req_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("chrdev firmware");
