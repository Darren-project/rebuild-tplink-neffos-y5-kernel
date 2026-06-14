#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xd87a11da, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xfe990052, __VMLINUX_SYMBOL_STR(gpio_free) },
	{ 0xed06b916, __VMLINUX_SYMBOL_STR(wakeup_source_drop) },
	{ 0xff5cda7c, __VMLINUX_SYMBOL_STR(wakeup_source_remove) },
	{ 0xedce55d0, __VMLINUX_SYMBOL_STR(hrtimer_cancel) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0x3ce4ca6f, __VMLINUX_SYMBOL_STR(disable_irq) },
	{ 0xce2840e7, __VMLINUX_SYMBOL_STR(irq_set_irq_wake) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0xee4f3181, __VMLINUX_SYMBOL_STR(wakeup_source_add) },
	{ 0xfcdb4034, __VMLINUX_SYMBOL_STR(wakeup_source_prepare) },
	{ 0x3fffd469, __VMLINUX_SYMBOL_STR(hrtimer_init) },
	{ 0x8226642f, __VMLINUX_SYMBOL_STR(__gpio_cansleep) },
	{ 0x47229b5c, __VMLINUX_SYMBOL_STR(gpio_request) },
	{ 0x9fbcfaa4, __VMLINUX_SYMBOL_STR(input_set_capability) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xf2fc7f6e, __VMLINUX_SYMBOL_STR(__pm_relax) },
	{ 0xfcec0987, __VMLINUX_SYMBOL_STR(enable_irq) },
	{ 0x9ca9538f, __VMLINUX_SYMBOL_STR(input_event) },
	{ 0xa8f59416, __VMLINUX_SYMBOL_STR(gpio_direction_output) },
	{ 0x6c8d5ae8, __VMLINUX_SYMBOL_STR(__gpio_get_value) },
	{ 0x68bf0a7c, __VMLINUX_SYMBOL_STR(hrtimer_start) },
	{ 0x1525f36d, __VMLINUX_SYMBOL_STR(__pm_stay_awake) },
	{ 0x65d6d0f0, __VMLINUX_SYMBOL_STR(gpio_direction_input) },
	{ 0x432fd7f6, __VMLINUX_SYMBOL_STR(__gpio_set_value) },
	{ 0x11f447ce, __VMLINUX_SYMBOL_STR(__gpio_to_irq) },
	{ 0x27bbf221, __VMLINUX_SYMBOL_STR(disable_irq_nosync) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

