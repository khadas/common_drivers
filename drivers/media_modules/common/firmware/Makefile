MODULE_NAME = firmware
obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y += firmware_drv.o
$(MODULE_NAME)-y += firmware_type.o
ifeq (${VERSION},5)
ifeq (${PATCHLEVEL},15)
	$(MODULE_NAME)-y += ../media_utils/media_utils_dummy.o
else
	$(MODULE_NAME)-y += ../media_utils/media_utils.o
endif
endif
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
