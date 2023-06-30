# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-trusted-keys
obj-$(CONFIG_AMLOGIC_TRUSTED_KEYS) = $(MODULE_NAME).o
$(MODULE_NAME)-y += trusted_core.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_TRUSTED_KEYS_TEE) += trusted_tee.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
BUILD_TIME := $(shell date +%Y.%m.%d-%H.%M.%S)
ccflags-y += "-DBUILD_TIME=\"$(BUILD_TIME)\""

COMMON_DRIVER_RELEASE=$(subst -g,, $(shell $(srctree)/scripts/setlocalversion $(srctree)/$(COMMON_DRIVERS_DIR)))
COMMON_DRIVER_RELEASE_DEFINE="-DCOMMON_DRIVER_RELEASE=\"$(COMMON_DRIVER_RELEASE)\""
ccflags-y += $(COMMON_DRIVER_RELEASE_DEFINE)
