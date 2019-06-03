/*
 * Kernel data types
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/types.h>

static long base_addr = 0x80000000;
module_param(base_addr, long, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(base_addr, "base address");

/*
 * Peripheral mapping
 */

struct dtypes_s {
	u32 reg0;
	u8  pad0[2];
	u16 reg1;
	u32 pad1[2];
	u8  reg2;
	u8  reg3;
	u16 reg4;
	u32 reg5;
} __attribute__ ((packed));

/*
 * Init & exit stuff
 */

static int __init data_types_init(void)
{
	struct dtypes_s *ptr = (struct dtypes_s *) base_addr;

	pr_info("\tu8\tu16\tu32\tu64\n");
	pr_info("size\t%ld\t%ld\t%ld\t%ld\n",
		sizeof(u8), sizeof(u16), sizeof(u32), sizeof(u64));

	pr_info("name\tptr\n");
	pr_info("reg0\t%px\n", &ptr->reg0);
	pr_info("reg1\t%px\n", &ptr->reg1);
	pr_info("reg2\t%px\n", &ptr->reg2);
	pr_info("reg3\t%px\n", &ptr->reg3);
	pr_info("reg4\t%px\n", &ptr->reg4);
	pr_info("reg5\t%px\n", &ptr->reg5);

	return -EINVAL;
}

static void __exit data_types_exit(void)
{
	pr_info("unloaded\n");
}

module_init(data_types_init);
module_exit(data_types_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel data types");
MODULE_VERSION("0.1");
