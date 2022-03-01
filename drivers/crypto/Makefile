# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-crypto-dma

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_CRYPTO_DMA) += $(MODULE_NAME).o
$(MODULE_NAME)-objs = aml-dma.o aml-crypto-dma.o aml-aes-dma.o aml-tdes-dma.o aml-sha-dma.o aml-crypto-device.o
