MODULE_NAME = vc8000
ifeq (${VERSION},5)
ifeq (${PATCHLEVEL},15)
	MODULE_NAME = amlogic-vc8000
endif
endif
obj-$(CONFIG_AMLOGIC_MEDIA_VENC_VCENC)	+= $(MODULE_NAME).o
$(MODULE_NAME)-objs +=  vc8000_driver.o
$(MODULE_NAME)-objs +=  vc8000_vcmd_driver.o
$(MODULE_NAME)-objs +=  bidirect_list.o
$(MODULE_NAME)-objs +=  vcmdswhwregisters.o
$(MODULE_NAME)-objs +=  vc8000_normal_driver.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
