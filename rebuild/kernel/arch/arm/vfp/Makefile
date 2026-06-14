#
# linux/arch/arm/vfp/Makefile
#
# Copyright (C) 2001 ARM Limited
#

# ccflags-y := -DDEBUG
# asflags-y := -DDEBUG

plus_sec := $(call as-instr,.arch_extension sec,+sec)
AFLAGS_vfpmodule.o	:=-Wa,-march=armv7-a$(plus_sec)
CFLAGS_vfpmodule.o	:=-Wa,-march=armv7-a$(plus_sec)

KBUILD_AFLAGS	:=$(KBUILD_AFLAGS:-msoft-float=-Wa,-mfpu=softvfp+vfp -mfloat-abi=soft)
LDFLAGS		+=--no-warn-mismatch

obj-y			+= vfp.o

vfp-$(CONFIG_VFP)	+= vfpmodule.o entry.o vfphw.o vfpsingle.o vfpdouble.o
