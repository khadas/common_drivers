# SPDX-License-Identifier: GPL-2.0

MODULE_NAME = amlogic-spinand
spinand-objs := core.o gigadevice.o macronix.o micron.o paragon.o toshiba.o winbond.o meson_spinand.o
obj-$(CONFIG_AMLOGIC_MTD_SPI_NAND) += spinand.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
