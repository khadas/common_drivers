MODULE_NAME = jpegenc
ifeq (${VERSION},5)
ifeq (${PATCHLEVEL},15)
	MODULE_NAME = amlogic-jpegenc
	$(MODULE_NAME)-objs += jpegenc.o
endif
endif
obj-$(CONFIG_AMLOGIC_MEDIA_VENC_JPEG) = $(MODULE_NAME).o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
