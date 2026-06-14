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
	{ 0x5611810a, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0xbebf4a0d, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0xc5baf0a3, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x133ac2d, __VMLINUX_SYMBOL_STR(mmc_unregister_driver) },
	{ 0x2e27c7d9, __VMLINUX_SYMBOL_STR(mmc_register_driver) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x27fa66e1, __VMLINUX_SYMBOL_STR(nr_free_buffer_pages) },
	{ 0xf7802486, __VMLINUX_SYMBOL_STR(__aeabi_uidivmod) },
	{ 0x954b19ae, __VMLINUX_SYMBOL_STR(mmc_can_trim) },
	{ 0x1424f59, __VMLINUX_SYMBOL_STR(sg_copy_to_buffer) },
	{ 0x8371daff, __VMLINUX_SYMBOL_STR(sg_copy_from_buffer) },
	{ 0xefdd2345, __VMLINUX_SYMBOL_STR(sg_init_one) },
	{ 0x85d82102, __VMLINUX_SYMBOL_STR(mmc_set_blocklen) },
	{ 0xaed891b6, __VMLINUX_SYMBOL_STR(mem_map) },
	{ 0x6ccf7bd7, __VMLINUX_SYMBOL_STR(__pv_phys_offset) },
	{ 0x46608fa0, __VMLINUX_SYMBOL_STR(getnstimeofday) },
	{ 0xd5152710, __VMLINUX_SYMBOL_STR(sg_next) },
	{ 0x6ec1cf0a, __VMLINUX_SYMBOL_STR(page_address) },
	{ 0xf88c3301, __VMLINUX_SYMBOL_STR(sg_init_table) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0xee9878ab, __VMLINUX_SYMBOL_STR(mmc_wait_for_req) },
	{ 0xea19167e, __VMLINUX_SYMBOL_STR(mmc_wait_for_cmd) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x6b6bf16b, __VMLINUX_SYMBOL_STR(mmc_start_req) },
	{ 0xca17189f, __VMLINUX_SYMBOL_STR(mmc_set_data_timeout) },
	{ 0x4dfcfa2d, __VMLINUX_SYMBOL_STR(mmc_erase) },
	{ 0xe28a0c9, __VMLINUX_SYMBOL_STR(mmc_can_erase) },
	{ 0xe6da44a, __VMLINUX_SYMBOL_STR(set_normalized_timespec) },
	{ 0xc34dd86e, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0xfeed6aaa, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x819022c0, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0x2e0b1fbc, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x98a0a8b2, __VMLINUX_SYMBOL_STR(contig_page_data) },
	{ 0x8f678b07, __VMLINUX_SYMBOL_STR(__stack_chk_guard) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x5b8af32b, __VMLINUX_SYMBOL_STR(__free_pages) },
	{ 0x5112b87a, __VMLINUX_SYMBOL_STR(mmc_rpm_release) },
	{ 0xd098d627, __VMLINUX_SYMBOL_STR(mmc_release_host) },
	{ 0xf0e41dde, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x90ea553a, __VMLINUX_SYMBOL_STR(__mmc_claim_host) },
	{ 0x228d64fe, __VMLINUX_SYMBOL_STR(mmc_rpm_hold) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x7dbed389, __VMLINUX_SYMBOL_STR(__alloc_pages_nodemask) },
	{ 0x86a4889a, __VMLINUX_SYMBOL_STR(kmalloc_order_trace) },
	{ 0x11a13e31, __VMLINUX_SYMBOL_STR(_kstrtol) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0x59e5070d, __VMLINUX_SYMBOL_STR(__do_div64) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0xfb449aeb, __VMLINUX_SYMBOL_STR(mmc_can_reset) },
	{ 0xacddfb7d, __VMLINUX_SYMBOL_STR(mmc_hw_reset_check) },
	{ 0x676c9915, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0x2c72ab94, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0x890d5728, __VMLINUX_SYMBOL_STR(debugfs_remove) },
	{ 0x93c992dd, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xdc96b929, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

