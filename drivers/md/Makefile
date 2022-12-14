# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-dm-bow

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
ccflags-y += -I$(srctree)/drivers/md

obj-$(CONFIG_AMLOGIC_DM_BOW) = $(MODULE_NAME).o
$(MODULE_NAME)-y += dm-bow.o
