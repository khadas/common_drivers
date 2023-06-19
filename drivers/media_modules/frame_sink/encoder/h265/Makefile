MODULE_NAME = vpu
ifeq (${VERSION},5)
ifeq (${PATCHLEVEL},15)
	MODULE_NAME = amlogic-vpu
	$(MODULE_NAME)-objs += vpu.o
endif
endif
obj-$(CONFIG_AMLOGIC_MEDIA_VENC_H265) = $(MODULE_NAME).o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
