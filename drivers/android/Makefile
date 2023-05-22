# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

#MODULE_NAME = amlogic-logger

#PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
#PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
#ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_ANDROID_LOGGER)	+= logger.o
obj-$(CONFIG_ANDROID_LOW_MEMORY_KILLER)	+= lowmemorykiller.o
