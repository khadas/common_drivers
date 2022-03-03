# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-freertos

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
CFLAGS_freertos.o += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_FREERTOS) = $(MODULE_NAME).o
$(MODULE_NAME)-y += freertos.o


AUTO_CAPTURE_MODULE_NAME = amlogic-autocapture

PR_FMT_AUTO_CAPTURE = $(subst amlogic-,,$(AUTO_CAPTURE_MODULE_NAME))
PR_FMT_DEFINE_AUTO_CAPTURE="-Dpr_fmt(fmt)= \"[$(PR_FMT_AUTO_CAPTURE)]: \" fmt"
CFLAGS_free_reserved.o += $(PR_FMT_DEFINE_AUTO_CAPTURE)

obj-$(CONFIG_AMLOGIC_AUTO_CAPTURE) += $(AUTO_CAPTURE_MODULE_NAME).o
$(AUTO_CAPTURE_MODULE_NAME)-y += free_reserved.o
