# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-power
obj-$(CONFIG_AMLOGIC_POWER)		+= $(MODULE_NAME).o
$(MODULE_NAME)-y += main.o	\
			power_domain_sysfs.o	\
			sec_pwr_ctrl.o	\
			sec_power_domain.o

$(MODULE_NAME)-$(CONFIG_AMLOGIC_POWER_EE) += power_ee_domain.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

include $(COMMON_DRIVERS_DIR)/drivers/power/meson_pmic6b/Makefile
