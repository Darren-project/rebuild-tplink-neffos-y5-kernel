cmd_drivers/platform/msm/sps/sps_dma.o := ../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-gcc -Wp,-MD,drivers/platform/msm/sps/.sps_dma.o.d -nostdinc -isystem /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9.x-google/include -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/drivers/soc/qcom -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/include/uapi -Iinclude/generated/uapi -include /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -Os -Wno-maybe-uninitialized -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fstack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack    -D"KBUILD_STR(s)=#s" -D"KBUILD_BASENAME=KBUILD_STR(sps_dma)"  -D"KBUILD_MODNAME=KBUILD_STR(sps_dma)" -c -o drivers/platform/msm/sps/.tmp_sps_dma.o drivers/platform/msm/sps/sps_dma.c

source_drivers/platform/msm/sps/sps_dma.o := drivers/platform/msm/sps/sps_dma.c

deps_drivers/platform/msm/sps/sps_dma.o := \
    $(wildcard include/config/sps/support/bamdma.h) \
    $(wildcard include/config/sps/support/ndp/bam.h) \

drivers/platform/msm/sps/sps_dma.o: $(deps_drivers/platform/msm/sps/sps_dma.o)

$(deps_drivers/platform/msm/sps/sps_dma.o):
