# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-seckey

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_SECKEY) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SECKEY_KT) += aml_kt.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SECKEY_KL) += aml_mkl.o