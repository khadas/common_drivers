# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-mailbox
obj-$(CONFIG_AMLOGIC_MHU_MBOX) += $(MODULE_NAME).o

$(MODULE_NAME)-y += meson_mbox_main.o \
		    meson_mbox_fifo.o \
		    meson_mbox_pl_v0.o \
		    meson_mbox_pl_v1.o \
		    meson_mbox_pl_v2.o \
		    meson_mbox_devfs.o \
		    meson_mbox_sec.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
