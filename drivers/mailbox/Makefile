# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-mailbox
obj-$(CONFIG_AMLOGIC_MHU_MBOX) += $(MODULE_NAME).o

$(MODULE_NAME)-y += meson_mhu_main.o		\
		    meson_mhu.o meson_mhu_pl.o	\
		    meson_mhu_fifo.o		\
		    meson_mhu_sec.o		\
		    meson_mailbox.o
$(MODULE_NAME)-$(CONFIG_ARM_SCPI_PROTOCOL) += scpi_protocol.o meson_mhu_user.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
