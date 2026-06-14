cmd_arch/arm/crypto/sha512-armv7-neon.o := ../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-gcc -Wp,-MD,arch/arm/crypto/.sha512-armv7-neon.o.d -nostdinc -isystem /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/../lib/gcc/arm-linux-androideabi/4.9.x-google/include -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/drivers/soc/qcom -I/workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/include/uapi -Iinclude/generated/uapi -include /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float       -c -o arch/arm/crypto/sha512-armv7-neon.o arch/arm/crypto/sha512-armv7-neon.S

source_arch/arm/crypto/sha512-armv7-neon.o := arch/arm/crypto/sha512-armv7-neon.S

deps_arch/arm/crypto/sha512-armv7-neon.o := \
  /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /workspaces/rebuild-tplink-neffos-y5-kernel/rebuild/kernel/arch/arm/include/asm/linkage.h \

arch/arm/crypto/sha512-armv7-neon.o: $(deps_arch/arm/crypto/sha512-armv7-neon.o)

$(deps_arch/arm/crypto/sha512-armv7-neon.o):
