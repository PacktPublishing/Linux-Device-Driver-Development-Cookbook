/*
 * Kernel timer
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/reboot.h>

/*
 * Module data
 */

static struct notifier_data {
	struct notifier_block netdevice_nb;
	struct notifier_block reboot_nb;
        unsigned int data;
} ninfo;


/*
 * The notifier handlers
 */

static int netdevice_notifier(struct notifier_block *nb,
                                     unsigned long code, void *unused)
{
	struct notifier_data *ninfo = container_of(nb, struct notifier_data,
                                                            netdevice_nb);

	pr_info("netdevice: event #%d with code 0x%lx caught!\n",
					ninfo->data++, code);

	return NOTIFY_DONE;
}

static int reboot_notifier(struct notifier_block *nb,
                                     unsigned long code, void *unused)
{
	struct notifier_data *ninfo = container_of(nb, struct notifier_data,
                                                            reboot_nb);

	pr_info("reboot: event #%d with code 0x%lx caught!\n",
					ninfo->data++, code);

	return NOTIFY_DONE;
}

/*
 * Probe/remove functions
 */

static int __init notifier_init(void)
{
	int ret;

	ninfo.netdevice_nb.notifier_call = netdevice_notifier;
	ninfo.netdevice_nb.priority = 10;

	ret = register_netdevice_notifier(&ninfo.netdevice_nb);
        if (ret) {
		pr_err("unable to register netdevice notifier\n");
		return ret;
	}

	ninfo.reboot_nb.notifier_call = reboot_notifier;
	ninfo.reboot_nb.priority = 10;

	ret = register_reboot_notifier(&ninfo.reboot_nb);
        if (ret) {
		pr_err("unable to register reboot notifier\n");
		goto unregister_netdevice;
	}

	pr_info("notifier module loaded\n");

	return 0;

unregister_netdevice:
	unregister_netdevice_notifier(&ninfo.netdevice_nb);
	return ret;
}

static void __exit notifier_exit(void)
{
	unregister_netdevice_notifier(&ninfo.netdevice_nb);
	unregister_reboot_notifier(&ninfo.reboot_nb);

	pr_info("notifier module unloaded\n");
}

module_init(notifier_init);
module_exit(notifier_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel timer");
MODULE_LICENSE("GPL");
