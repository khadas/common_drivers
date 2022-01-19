# SPDX-License-Identifier: GPL-2.0-only

MODULE_NAME = amlogic-input-ir
obj-$(CONFIG_AMLOGIC_MESON_IR)  = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson_ir_core.o
$(MODULE_NAME)-y += meson_ir_main.o
$(MODULE_NAME)-y += meson_ir_sysfs.o
$(MODULE_NAME)-y += meson_ir_cdev.o
$(MODULE_NAME)-y += meson_ir_raw.o
$(MODULE_NAME)-y += meson_ir_regmap.o
$(MODULE_NAME)-y += meson_ir_decoder_xmp.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
