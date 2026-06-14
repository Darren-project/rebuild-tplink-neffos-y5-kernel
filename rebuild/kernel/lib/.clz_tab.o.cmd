cmd_lib/clz_tab.o := ../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-gcc -Wp,-MD,lib/.clz_tab.o.d -nostdinc -isystem /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9.x-google/include -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/drivers/soc/qcom -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/include/uapi -Iinclude/generated/uapi -include /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -Os -Wno-maybe-uninitialized -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fstack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack    -D"KBUILD_STR(s)=#s" -D"KBUILD_BASENAME=KBUILD_STR(clz_tab)"  -D"KBUILD_MODNAME=KBUILD_STR(clz_tab)" -c -o lib/.tmp_clz_tab.o lib/clz_tab.c

source_lib/clz_tab.o := lib/clz_tab.c

deps_lib/clz_tab.o := \

lib/clz_tab.o: $(deps_lib/clz_tab.o)

$(deps_lib/clz_tab.o):
