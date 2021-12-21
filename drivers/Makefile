# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

obj-y		+= tty/
obj-y		+= clk/
obj-y		+= pinctrl/
obj-y		+= irqchip/
obj-y		+= secmon/
obj-y		+= power/
obj-y		+= cpu_info/
obj-y		+= pm/
obj-y		+= gpio/
obj-y		+= input/
obj-y		+= i2c/

KBUILD_CFLAGS_MODULE += $(GKI_EXT_MODULE_PREDEFINE)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(GKI_EXT_MODULE_CONFIG)

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
