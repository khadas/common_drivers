# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_main.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_video.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_audio.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_edid.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_audio.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_hdcp.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_scdc.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_infoframe.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_param.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_HDMITX21) += vout/hdmitx21/hdmi_tx_vrr.o
$(MEDIA_MODULE_NAME)-y += vout/hdmitx21/hdmi_tx_ext.o

ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/vout/hdmitx_common

include $(COMMON_DRIVERS_DIR)/drivers/media/vout/hdmitx21/hw/Makefile
