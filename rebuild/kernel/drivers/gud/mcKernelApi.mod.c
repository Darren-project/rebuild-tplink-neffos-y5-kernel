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
	{ 0xd87a11da, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x2e0b1fbc, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x689844b5, __VMLINUX_SYMBOL_STR(mobicore_open) },
	{ 0x93c992dd, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x4f0eaad, __VMLINUX_SYMBOL_STR(mobicore_unmap_vmem) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0xfeed6aaa, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0xb6ae3e84, __VMLINUX_SYMBOL_STR(mutex_lock_interruptible) },
	{ 0x37379151, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x6b831921, __VMLINUX_SYMBOL_STR(netlink_kernel_release) },
	{ 0x6c4d9db1, __VMLINUX_SYMBOL_STR(mobicore_map_vmem) },
	{ 0x71887e22, __VMLINUX_SYMBOL_STR(mobicore_free_wsm) },
	{ 0x7fff621f, __VMLINUX_SYMBOL_STR(netlink_unicast) },
	{ 0xb7635f3f, __VMLINUX_SYMBOL_STR(init_net) },
	{ 0xc34dd86e, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0xa8e962e4, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0xcfb3fdf, __VMLINUX_SYMBOL_STR(mobicore_allocate_wsm) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x3bd1b1f6, __VMLINUX_SYMBOL_STR(msecs_to_jiffies) },
	{ 0xd76df796, __VMLINUX_SYMBOL_STR(kfree_skb) },
	{ 0x8f368ea9, __VMLINUX_SYMBOL_STR(netlink_ack) },
	{ 0xf0e41dde, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xc4eff14e, __VMLINUX_SYMBOL_STR(__netlink_kernel_create) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x9d669763, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x4be7fb63, __VMLINUX_SYMBOL_STR(up) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x8f678b07, __VMLINUX_SYMBOL_STR(__stack_chk_guard) },
	{ 0x71e88a25, __VMLINUX_SYMBOL_STR(dev_set_name) },
	{ 0x96f8d8c1, __VMLINUX_SYMBOL_STR(__nlmsg_put) },
	{ 0x837d0f0a, __VMLINUX_SYMBOL_STR(down_timeout) },
	{ 0x96e2a925, __VMLINUX_SYMBOL_STR(mobicore_release) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=mcDrvModule";

