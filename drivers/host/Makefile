# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

# Makefile for amlogic mcu host driver

MODULE_NAME = amlogic-host
obj-$(CONFIG_AMLOGIC_HOST_DRIVER)	= $(MODULE_NAME).o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_HOST_DRIVER)		+= host.o sysfs.o host_poll.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
