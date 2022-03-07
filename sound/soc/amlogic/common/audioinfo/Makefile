# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic_audiodata

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/efuse_unifykey/

obj-$(CONFIG_AMLOGIC_AUDIO_INFO) += $(MODULE_NAME).o
$(MODULE_NAME)-y += audio_data.o
