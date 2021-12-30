# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-mailbox
obj-$(CONFIG_AMLOGIC_MHU_MBOX) += $(MODULE_NAME).o

$(MODULE_NAME)-y += meson_mhu_main.o		\
		    meson_mhu.o meson_mhu_pl.o	\
		    meson_mhu_fifo.o		\
		    meson_mhu_sec.o		\
		    meson_mailbox.o
$(MODULE_NAME)-$(CONFIG_ARM_SCPI_PROTOCOL) += scpi_protocol.o meson_mhu_user.o
