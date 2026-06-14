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
	{ 0xe120c88a, __VMLINUX_SYMBOL_STR(elv_unregister) },
	{ 0x9e96b6f0, __VMLINUX_SYMBOL_STR(elv_register) },
	{ 0xc8b57c27, __VMLINUX_SYMBOL_STR(autoremove_wake_function) },
	{ 0x7d11c268, __VMLINUX_SYMBOL_STR(jiffies) },
	{ 0xd5f2172f, __VMLINUX_SYMBOL_STR(del_timer_sync) },
	{ 0x1cfb04fa, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x1000e51, __VMLINUX_SYMBOL_STR(schedule) },
	{ 0x499cb58c, __VMLINUX_SYMBOL_STR(prepare_to_wait) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x60d6cff8, __VMLINUX_SYMBOL_STR(blk_run_queue) },
	{ 0x8834396c, __VMLINUX_SYMBOL_STR(mod_timer) },
	{ 0x3bd1b1f6, __VMLINUX_SYMBOL_STR(msecs_to_jiffies) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0xd4848c42, __VMLINUX_SYMBOL_STR(debugfs_create_u32) },
	{ 0x924ea193, __VMLINUX_SYMBOL_STR(debugfs_create_dir) },
	{ 0x593a99b, __VMLINUX_SYMBOL_STR(init_timer_key) },
	{ 0x275ef902, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x6818a5, __VMLINUX_SYMBOL_STR(kobject_put) },
	{ 0xc7d91c9b, __VMLINUX_SYMBOL_STR(elevator_alloc) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0x33b3c9f3, __VMLINUX_SYMBOL_STR(elv_dispatch_sort) },
	{ 0x8f2fa050, __VMLINUX_SYMBOL_STR(debugfs_remove_recursive) },
	{ 0x5fc56a46, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0x9c0bd51f, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xa772c396, __VMLINUX_SYMBOL_STR(blk_rq_map_kern) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x2e0b1fbc, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x998ff5a2, __VMLINUX_SYMBOL_STR(fs_bio_set) },
	{ 0x967bbf71, __VMLINUX_SYMBOL_STR(blk_put_request) },
	{ 0xf0e41dde, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x571eb49d, __VMLINUX_SYMBOL_STR(init_request_from_bio) },
	{ 0xcc62e055, __VMLINUX_SYMBOL_STR(blk_get_request) },
	{ 0x56381773, __VMLINUX_SYMBOL_STR(bio_alloc_bioset) },
	{ 0x1a1431fd, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irq) },
	{ 0x3507a132, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irq) },
	{ 0x52739882, __VMLINUX_SYMBOL_STR(bio_put) },
	{ 0x49ebacbd, __VMLINUX_SYMBOL_STR(_clear_bit) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xd85cd67e, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

