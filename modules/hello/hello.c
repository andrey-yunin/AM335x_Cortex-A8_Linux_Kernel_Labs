#include <linux/init.h>
#include <linux/kstrtox.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static char *username = "world";
module_param(username, charp, 0444);
MODULE_PARM_DESC(username, "Name to print in the hello message");

static bool verbose;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Enable verbose hello module messages");

static int log_level = 1;

static int log_level_set(const char *val, const struct kernel_param *kp)
{
    int new_level;
    int ret;

    ret = kstrtoint(val, 0, &new_level);
    if (ret)
        return ret;

    if (new_level < 0 || new_level > 3) {
        pr_warn("bbb_hello: log_level must be between 0 and 3\n");
        return -EINVAL;
    }

    *((int *)kp->arg) = new_level;
    pr_info("bbb_hello: log_level changed to %d\n", new_level);

    return 0;
}

static int log_level_get(char *buffer, const struct kernel_param *kp)
{
    return param_get_int(buffer, kp);
}

/*
 * kernel_param_ops is the kernel callback table for a custom module
 * parameter. The parameter core calls .set on writes and .get on reads.
 */
static const struct kernel_param_ops log_level_ops = {
    .set = log_level_set,
    .get = log_level_get,
};

/*
 * module_param_cb registers log_level as a module parameter and connects it
 * to log_level_ops instead of the standard int parameter handlers.
 */
module_param_cb(log_level, &log_level_ops, &log_level, 0644);
MODULE_PARM_DESC(log_level, "Logging level from 0 to 3");

static int __init bbb_hello_init(void)
{
    pr_info("bbb_hello: init, username=%s\n", username);
    if (verbose)
        pr_info("bbb_hello: verbose mode is enabled\n");

    if (log_level >= 2)
        pr_info("bbb_hello: log_level=%d\n", log_level);

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
