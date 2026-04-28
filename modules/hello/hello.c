#include <linux/init.h>
#include <linux/module.h>

static char *username = "world";
module_param(username, charp, 0444);
MODULE_PARM_DESC(username, "Name to print in the hello message");

static bool verbose;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable verbose hello module messages");

static int __init bbb_hello_init(void)
{
    pr_info("bbb_hello: init, username=%s\n", username);
    if (verbose)
        pr_info("bbb_hello: verbose mode is enabled\n");

    return 0;
}

static void __exit bbb_hello_exit(void)
{
    pr_info("bbb_hello: exit, username=%s\n", username);
    if (verbose)
        pr_info("bbb_hello: verbose mode was enabled\n");
}

module_init(bbb_hello_init);
module_exit(bbb_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey");
MODULE_DESCRIPTION("Minimal BeagleBone Black hello kernel module");
