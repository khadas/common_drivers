
obj-m += amlcam.o

M_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

ifeq ($(DRV_SRC_OBJ),)
	DRV_SRC := $(wildcard $(M_PATH)/*.c $(M_PATH)/cam_common/*.c $(M_PATH)/cam_isp/*.c $(M_PATH)/cam_isp/*/*.c $(M_PATH)/cam_adapter/*.c $(M_PATH)/cam_adapter/*/*.c $(M_PATH)/cam_csiphy/*.c $(M_PATH)/cam_csiphy/*/*.c $(M_PATH)/cam_pattern/*.c)
	DRV_SRC := $(subst $(M_PATH)/,,$(DRV_SRC))
	export DRV_SRC_OBJ := $(DRV_SRC:.c=.o)
endif

amlcam-objs += $(DRV_SRC_OBJ)

LOCAL_INCLUDES := -I$(M_PATH)/ -I$(M_PATH)/cam_common \
		-I$(M_PATH)/cam_isp -I$(M_PATH)/cam_isp/hw/ \
		-I$(M_PATH)/cam_adapter -I$(M_PATH)/cam_adapter/hw/ \
		-I$(M_PATH)/cam_csiphy -I$(M_PATH)/cam_csiphy/hw/ \
		-I$(M_PATH)/cam_pattern

ccflags-y += -DT7C_CHIP -DANDROID_OS

ccflags-y += -Wno-unused-function
ccflags-y += -Wno-unused-variable

ccflags-y += $(LOCAL_INCLUDES)
ccflags-y += -Wno-declaration-after-statement -Idrivers/staging/android/ \
		-Wno-error
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
	-include $(OUT_DIR)/include/config/auto.conf
	$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	$(Q)mkdir -p $(out_dir)/../vendor_lib
	$(Q)if [ -z "$(CONFIG_AMLOGIC_KERNEL_VERSION)" ]; then \
		cp $(out_dir)/$(M)/*.ko ${out_dir}/../vendor_lib/modules/ ; \
	else \
		find $(INSTALL_MOD_PATH)/lib/modules/*/$(INSTALL_MOD_DIR) -name "*.ko" -exec cp {} $(out_dir)/../vendor_lib/modules/ \; ; \
	fi

clean:
	@$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
