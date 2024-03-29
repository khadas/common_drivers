# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-debug

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

ccflags-y += -I$(srctree)/fs/pstore/
ccflags-y += -I$(srctree)/kernel/sched/
ccflags-y += "-DGKI_CONFIG=\"$(srctree)/$(COMMON_DRIVERS_DIR)/arch/arm64/configs/amlogic_gki.fragment\""

obj-$(CONFIG_AMLOGIC_DEBUG) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o lockup.o sched.o kprobes.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_ISOLCPUS) += isolcpus.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_MHZ) += cpu_mhz.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_ATRACE) += atrace.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_FILE) += debug_file.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_GKI_CONFIG) += gki_config.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_HLD) += watchdog_hld.o
ifeq ($(CONFIG_AMLOGIC_DEBUG_PRINTK),m)
	$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_PRINTK) += printk.o
else
	obj-$(CONFIG_AMLOGIC_DEBUG_PRINTK) += printk.o
endif

MODULE_NAME_IOTRACE = amlogic-debug-iotrace
obj-$(CONFIG_AMLOGIC_DEBUG_IOTRACE) += $(MODULE_NAME_IOTRACE).o
$(MODULE_NAME_IOTRACE)-y = aml_iotrace.o ftrace_ramoops.o

# builtin
obj-$(CONFIG_AMLOGIC_BGKI_DEBUG_MISC) += debug_misc.o

# debug test ko
obj-$(CONFIG_AMLOGIC_DEBUG_TEST) += debug_test.o
AUDIO_UTILS_MODULE_NAME = amlogic-audio-utils

AUDIO_UTILS_PR_FMT = $(subst amlogic-,,$(AUDIO_UTILS_MODULE_NAME))
AUDIO_UTILS_PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(AUDIO_UTILS_PR_FMT)]: \" fmt"
ccflags_audio_utils.o += $(AUDIO_UTILS_PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_AUDIO_UTILS) += $(AUDIO_UTILS_MODULE_NAME).o
$(AUDIO_UTILS_MODULE_NAME)-y = audio_utils.o
