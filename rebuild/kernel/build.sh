export ARCH=arm
export CROSS_COMPILE=../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
export KBUILD_CFLAGS="-march=armv7-a -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard"
export KBUILD_AFLAGS="-march=armv7-a"
export CC=../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-gcc
export LD=../prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-ld
export HOSTCFLAGS="-fcommon"

make  -j$(nproc) CC=$CC LD=$LD HOSTCFLAGS=$HOSTCFLAGS
