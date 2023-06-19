MODULE_NAME = amvenc_multi
ifeq (${VERSION},5)
ifeq (${PATCHLEVEL},15)
	MODULE_NAME = amlogic-multienc
endif
endif
obj-$(CONFIG_AMLOGIC_MEDIA_VENC_MULTI) = $(MODULE_NAME).o
$(MODULE_NAME)-objs += vpu_multi.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
