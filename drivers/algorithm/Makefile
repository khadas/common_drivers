# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-algorithm

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

#ccflags-y += -I$(srctree)/drivers/algorithm

obj-$(CONFIG_AMLOGIC_ALGORITHM) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o

#include $(COMMON_DRIVERS_DIR)/drivers/algorithm/dnlp/Makefile
#include $(COMMON_DRIVERS_DIR)/drivers/algorithm/common/Makefile

#obj-$(CONFIG_AMLOGIC_MEDIA_ALGORITHM) = ldim/
#obj-$(CONFIG_AMLOGIC_MEDIA_ALGORITHM) += dnlp/
#obj-$(CONFIG_AMLOGIC_MEDIA_ALGORITHM) += common/
#obj-$(CONFIG_AMLOGIC_MEDIA_ALGORITHM) += frc/
