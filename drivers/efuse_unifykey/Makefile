# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-efuse-unifykey
obj-$(CONFIG_AMLOGIC_EFUSE_UNIFYKEY) = $(MODULE_NAME).o
$(MODULE_NAME)-y += efuse_unifykey.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_EFUSE) += efuse.o efuse_api.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_EFUSE_BURN) +=  efuse_burn.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_UNIFYKEY) += security_key.o unifykey.o unifykey_dts.o storage_key.o normal_key.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
