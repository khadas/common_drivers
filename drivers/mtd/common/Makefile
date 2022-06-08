# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-mtd-common
obj-$(CONFIG_AMLOGIC_MTD_COMMON) = $(MODULE_NAME).o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_MTD_RESV) += reserve/rsv-manage.o reserve/rsv-dev.o reserve/rsv-key.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_MTD_COMMON) += nand_bootpart.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
