#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ftrace.h>
#include <linux/version.h>
#include <linux/random.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/kprobes.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kapusha");
MODULE_DESCRIPTION("Ftrace hook demo for get_random_bytes_user");

/*
 * Workaround to get kallsyms_lookup_name address since it's not always exported.
 * Uses kprobe technique to dynamically find the symbol address.
 */
static unsigned long my_kallsyms_lookup_name(const char *name)
{
    static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
    };

    unsigned long (*real_kallsyms_lookup_name)(const char *name);
    int ret;

    // Register temporary kprobe to get the address
    ret = register_kprobe(&kp);
    if (ret < 0)
        return 0;

    // Save the actual function address
    real_kallsyms_lookup_name = (void *)kp.addr;

    // Clean up the kprobe
    unregister_kprobe(&kp);

    // Call the real function to lookup our target symbol
    return real_kallsyms_lookup_name(name);
}

// Pointer to store the original get_random_bytes function
static void (*real_get_random_bytes)(void *buf, int nbytes);

/*
 * Our replacement function for get_random_bytes.
 * This will be called instead of the original function.
 */
void my_get_random_bytes(void *buf, int nbytes)
{
    // Artificial delay to simulate processing 50ms
    mdelay(50);

    // Log that our function was called
    pr_info("my_get_random_bytes called instead (ftrace)!\n");
}

/*
 * Ftrace callback function that gets executed when hooked function is called.
 * This is where we intercept the call and redirect it to our implementation.
 */
static void notrace ftrace_thunk(unsigned long ip, unsigned long parent_ip,
                                 struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
    // Get the pt_regs structure containing register values
    struct pt_regs *regs = ftrace_get_regs(fregs);

    // Extract function arguments from registers (x86_64 calling convention)
    void *buf = (void *)regs->di;    // First argument (RDI)
    int nbytes = (int)regs->si;       // Second argument (RSI)

    // Call our custom implementation
    my_get_random_bytes(buf, nbytes);
}

/*
 * Helper function to lookup symbol address using our kallsyms workaround.
 */
static unsigned long get_symbol_address(const char *name)
{
    return my_kallsyms_lookup_name(name);
}

/*
 * Ftrace operations structure that defines our hook.
 * The flags configure how ftrace will handle our hook:
 * - SAVE_REGS: Save register state
 * - RECURSION: Allow recursion
 * - IPMODIFY: Allow modifying the instruction pointer
 */
static struct ftrace_ops hook_ops = {
    .func = ftrace_thunk,           // Our callback function
    .flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION | FTRACE_OPS_FL_IPMODIFY,
};

/*
 * Module initialization function.
 * Sets up the ftrace hook for get_random_bytes.
 */
static int __init ftrace_hook_init(void)
{
    // Find address of get_random_bytes using our symbol lookup
    unsigned long addr = get_symbol_address("get_random_bytes_user");
    if (!addr) {
        pr_err("Unable to find symbol: get_random_bytes_user\n");
        return -ENOENT;
    }

    // Save the original function pointer
    real_get_random_bytes = (void *)addr;

    // Set up ftrace filter to only trigger on our target function
    int ret = ftrace_set_filter_ip(&hook_ops, addr, 0, 0);
    if (ret) {
        pr_err("ftrace_set_filter_ip failed: %d\n", ret);
        return ret;
    }

    // Register our ftrace hook
    ret = register_ftrace_function(&hook_ops);
    if (ret) {
        pr_err("register_ftrace_function failed: %d\n", ret);
        // Clean up filter if registration failed
        ftrace_set_filter_ip(&hook_ops, addr, 1, 0);
        return ret;
    }



    pr_info("ftrace_hook_demo loaded and hooked get_random_bytes_user()\n");
    return 0;
}

/*
 * Module cleanup function.
 * Removes the ftrace hook and restores normal operation.
 */
static void __exit ftrace_hook_exit(void)
{
    if (real_get_random_bytes) {
        // Remove the ftrace filter
        ftrace_set_filter_ip(&hook_ops, (unsigned long)real_get_random_bytes, 1, 0);

        // Unregister our hook
        unregister_ftrace_function(&hook_ops);

        pr_info("ftrace_hook_demo unloaded and unhooked get_random_bytes()\n");
    }
}

// Standard module entry and exit points
module_init(ftrace_hook_init);
module_exit(ftrace_hook_exit);
