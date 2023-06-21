
obj-m += amlsens.o

M_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

ifeq ($(DRV_SRC_OBJ),)
	DRV_SRC := $(wildcard $(M_PATH)/*.c $(M_PATH)/imx290/*.c $(M_PATH)/imx335/*.c $(M_PATH)/imx378/*.c $(M_PATH)/imx415/*.c $(M_PATH)/ov08a10/*.c $(M_PATH)/ov5640/*.c \
				$(M_PATH)/ov13b10/*.c $(M_PATH)/imx577/*.c $(M_PATH)/ov16a1q/*c $(M_PATH)/ov13855/*.c $(M_PATH)/lt6911c/*.c)
	DRV_SRC := $(subst $(M_PATH)/,,$(DRV_SRC))
	export DRV_SRC_OBJ := $(DRV_SRC:.c=.o)
endif

amlsens-objs := $(DRV_SRC_OBJ)

LOCAL_INCLUDES := -I$(M_PATH)/ -I$(M_PATH)/include/

EXTRA_CFLAGS += $(LOCAL_INCLUDES)

ifeq ($(O),)
out_dir := .
else
out_dir := $(O)
endif
-include $(out_dir)/include/config/auto.conf

all:
	@$(MAKE) -C $(KERNEL_SRC) M=$(M)  modules

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	$(Q)mkdir -p $(out_dir)/../vendor_lib
	$(Q)if [ -z "$(CONFIG_AMLOGIC_KERNEL_VERSION)" ]; then \
		cp $(out_dir)/$(M)/*.ko ${out_dir}/../vendor_lib/modules/ ; \
	else \
		find $(INSTALL_MOD_PATH)/lib/modules/*/$(INSTALL_MOD_DIR) -name "*.ko" -exec cp {} $(out_dir)/../vendor_lib/modules/ \; ; \
	fi

clean:
	@$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
