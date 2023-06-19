MODULE_NAME = memalloc
ifeq (${VERSION},5)
ifeq (${PATCHLEVEL},15)
	MODULE_NAME = amlogic-memalloc
	$(MODULE_NAME)-objs += memalloc.o
endif
endif
obj-$(CONFIG_AMLOGIC_MEDIA_VENC_MEMALLOC) += $(MODULE_NAME).o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
