# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-mtd-reserve
obj-$(CONFIG_AMLOGIC_MTD_RESV) = $(MODULE_NAME).o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_MTD_RESV) += rsv-manage.o rsv-dev.o rsv-key.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
