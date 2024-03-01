#
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2011-2018 ARM or its affiliates
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2.
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

obj-m += iv009_isp_g12b.o

M_PATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))
LOCAL_PATH := $(shell pwd)

FW_SRC := $(wildcard $(M_PATH)/src/*.c  $(M_PATH)/src/*/*.c \
		$(M_PATH)/src/*/*/*.c  $(M_PATH)/app/*.c  $(M_PATH)/app/*/*.c)
M_FW_SRC := $(FW_SRC:$(M_PATH)/%=%)
FW_SRC_OBJ := $(M_FW_SRC:.c=.o)

iv009_isp_g12b-objs += $(FW_SRC_OBJ)

EXTRA_CFLAGS := -I$(M_PATH)/app -I$(M_PATH)/inc -I$(M_PATH)/app/control \
		-I$(M_PATH)/inc/api -I$(M_PATH)/inc/isp -I$(M_PATH)/inc/sys \
		-I$(M_PATH)/src/platform -I$(M_PATH)/src/fw \
		-I$(M_PATH)/src/fw_lib -I$(M_PATH)/src/calibration \
		-I$(M_PATH)/src/driver/sensor -I$(M_PATH)/src/driver/lens

EXTRA_CFLAGS += -Wno-declaration-after-statement -Idrivers/staging/android/

ccflags-y += -DCONFIG_LINUX_OS

ccflags-y += -DCONFIG_G12B

ccflags-y += -DCONFIG_SEAMLESS_N
.PHONY:clean

all:
	@echo "$(MAKE) jss M=$(M) -C $(KERNEL_SRC) modules"
	@$(MAKE) -C $(KERNEL_SRC) M=$(M)  modules
modules_install:
	@echo "$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install"
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib
	cp $(OUT_DIR)/$(M)/*.ko ${OUT_DIR}/../vendor_lib/modules/

clean:
	@$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
