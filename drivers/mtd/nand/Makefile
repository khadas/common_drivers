# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic_mtd_nfc
obj-$(CONFIG_AMLOGIC_MTD_NAND) = $(MODULE_NAME).o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_MTD_NAND) += meson_nand.o meson_parse_part.o nand_flash_id.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
