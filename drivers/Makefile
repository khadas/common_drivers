# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

obj-y		+= uart/
obj-y		+= clk/
obj-y		+= pinctrl/
obj-y		+= irqchip/
obj-y		+= secmon/
obj-y		+= power/

KBUILD_CFLAGS_MODULE += $(GKI_EXT_MODULE_PREDEFINE)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(GKI_EXT_MODULE_CONFIG)

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
