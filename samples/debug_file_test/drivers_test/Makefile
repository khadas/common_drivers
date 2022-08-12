# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-debug-file-test
obj-$(CONFIG_AMLOGIC_SAMPLES_DEBUG_FILE_TEST) = $(MODULE_NAME).o
$(MODULE_NAME)-y += debug_file_test.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
CFLAGS_pinctrl-debug_file_test.o += $(PR_FMT_DEFINE)