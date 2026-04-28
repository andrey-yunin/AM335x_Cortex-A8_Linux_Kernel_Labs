#include <linux/init.h>
#include <linux/module.h>

static int __init bbb_hello_init(void)
{
    pr_info("bbb_hello: init\n");
    return 0;
}

static void __exit bbb_hello_exit(void)
{
    pr_info("bbb_hello: exit\n");
}

module_init(bbb_hello_init);
module_exit(bbb_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey");
MODULE_DESCRIPTION("Minimal BeagleBone Black hello kernel module");
