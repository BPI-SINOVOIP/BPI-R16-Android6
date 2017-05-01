#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x3bc25e7e, "module_layout" },
	{ 0xe125f41a, "input_free_int" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x528c709d, "simple_read_from_buffer" },
	{ 0x15692c87, "param_ops_int" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x97255bdf, "strlen" },
	{ 0x81c795c1, "dev_set_drvdata" },
	{ 0x43a53735, "__alloc_workqueue_key" },
	{ 0xd65a342d, "i2c_del_driver" },
	{ 0x3b05df25, "malloc_sizes" },
	{ 0x76789af4, "hrtimer_cancel" },
	{ 0xfa6a10a3, "i2c_transfer" },
	{ 0x6db82128, "remove_proc_entry" },
	{ 0x33543801, "queue_work" },
	{ 0x432fd7f6, "__gpio_set_value" },
	{ 0x20317da7, "filp_close" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xfbf9cde, "input_free_platform_resource" },
	{ 0x91715312, "sprintf" },
	{ 0xcfe7fc04, "sysfs_remove_group" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xab9df9fb, "input_set_abs_params" },
	{ 0x89d145ed, "pin_config_get" },
	{ 0x27392073, "input_event" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xb5eeb329, "register_early_suspend" },
	{ 0x5f754e5a, "memset" },
	{ 0x74c97f9c, "_raw_spin_unlock_irqrestore" },
	{ 0x27e1a049, "printk" },
	{ 0x20c55ae0, "sscanf" },
	{ 0x3a976d75, "sysfs_create_group" },
	{ 0x71c90087, "memcmp" },
	{ 0xa8f59416, "gpio_direction_output" },
	{ 0xfc50edea, "input_set_int_enable" },
	{ 0x84b183ae, "strncmp" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0xa7a9b9fc, "input_set_capability" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x59bdc133, "input_fetch_sysconfig_para" },
	{ 0xb4cf768e, "i2c_register_driver" },
	{ 0xb78d1eb9, "input_request_int" },
	{ 0xd16733dd, "input_init_platform_resource" },
	{ 0xbc601ad6, "script_get_item" },
	{ 0xcdaa78b3, "input_register_device" },
	{ 0x780f2390, "hrtimer_start" },
	{ 0x996bdb64, "_kstrtoul" },
	{ 0x3d22b4f3, "kmem_cache_alloc_trace" },
	{ 0xbd7083bc, "_raw_spin_lock_irqsave" },
	{ 0x856b3b5f, "input_set_power_enable" },
	{ 0x38459401, "proc_create_data" },
	{ 0x1b68aba6, "pin_config_set" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x7abdd440, "input_unregister_device" },
	{ 0xb190a2b0, "hrtimer_init" },
	{ 0xb227ae83, "unregister_early_suspend" },
	{ 0xe1dc4da4, "dev_get_drvdata" },
	{ 0xe914e41e, "strcpy" },
	{ 0x9329fce1, "filp_open" },
	{ 0x8469594a, "input_allocate_device" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "5C1DDE58922CE8307F46567");
