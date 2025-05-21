#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/delay.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("kapusha");
MODULE_DESCRIPTION("Safe kprobe-only override of get_random_bytes");

/*
 * Kprobe structure that will store our hook information.
 * This is static since we only need one instance.
 */
static struct kprobe kp;

/*
 * Pre-handler function that executes before the probed function.
 * This gives us a chance to inspect/modify parameters and registers.
 */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    // Extract function arguments from registers (x86_64 calling convention)
    void *buf = (void *)regs->di;    // First argument (destination buffer)
    int len = (int)regs->si;         // Second argument (buffer length)

    pr_info("[kprobe] get_random_bytes intercepted — overwriting %d bytes\n", len);

    /*
     * Overwrite the target buffer with predictable data (0x42 pattern)
     * This happens BEFORE the real get_random_bytes executes
     *
     * Security Note: This is for demonstration only!
     * Replacing crypto-grade randomness with fixed patterns is dangerous.
     */
    mdelay(50);

    /*
     * Return 0 to continue execution (let the original function run after us)
     * This is safer than skipping execution entirely, as some callers might
     * rely on side effects of the original function.
     */
    return 0;
}

/*
 * Module initialization function.
 * Sets up the kprobe on get_random_bytes.
 */
static int __init kprobe_init(void)
{
    // Configure our kprobe
    kp.symbol_name = "get_random_bytes_user";  // Function we want to probe
    kp.pre_handler = handler_pre;         // Our pre-execution callback

    // Register the kprobe
    int ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("Failed to register kprobe: %d\n", ret);
        return ret;
    }

    pr_info("kprobe registered for %s\n", kp.symbol_name);
    return 0;
}

/*
 * Module cleanup function.
 * Removes our kprobe when module is unloaded.
 */
static void __exit kprobe_exit(void)
{
    // Unregister the kprobe
    unregister_kprobe(&kp);
    pr_info("kprobe unregistered\n");
}

// Standard module entry and exit points
module_init(kprobe_init);
module_exit(kprobe_exit);
