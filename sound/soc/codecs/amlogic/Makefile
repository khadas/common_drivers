# SPDX-License-Identifier: GPL-2.0

KBUILD_CFLAGS += -Wlarger-than=32768
#KBUILD_CFLAGS += -Wstack-usage=1856

# Amlogic
MODULE_NAME_DUMMY = amlogic-snd-codec-dummy
PR_FMT_DUMMY = $(subst amlogic-,,$(MODULE_NAME_DUMMY))
PR_FMT_DEFINE_DUMMY = "-Dpr_fmt(fmt)= \"[$(PR_FMT_DUMMY)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_CODEC_DUMMY_CODEC) += $(MODULE_NAME_DUMMY).o
CFLAGS_dummy_codec.o += $(PR_FMT_DEFINE_DUMMY)
$(MODULE_NAME_DUMMY)-y += dummy_codec.o

MODULE_NAME_A1 = amlogic-snd-codec-a1
PR_FMT_A1 = $(subst amlogic-,,$(MODULE_NAME_A1))
PR_FMT_DEFINE_A1 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_A1)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_CODEC_A1_ACODEC) += $(MODULE_NAME_A1).o
CFLAGS_aml_codec_a1_acodec.o += $(PR_FMT_DEFINE_A1)
$(MODULE_NAME_A1)-y += aml_codec_a1_acodec.o

MODULE_NAME_T9015 = amlogic-snd-codec-t9015
PR_FMT_T9015 = $(subst amlogic-,,$(MODULE_NAME_T9015))
PR_FMT_DEFINE_T9015 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_T9015)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_CODEC_AMLT9015) += $(MODULE_NAME_T9015).o
CFLAGS_aml_codec_t9015.o += $(PR_FMT_DEFINE_T9015)
$(MODULE_NAME_T9015)-y += aml_codec_t9015.o

MODULE_NAME_TL1 = amlogic-snd-codec-tl1
PR_FMT_TL1 = $(subst amlogic-,,$(MODULE_NAME_TL1))
PR_FMT_DEFINE_TL1 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_TL1)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_CODEC_TL1_ACODEC) += $(MODULE_NAME_TL1).o
CFLAGS_aml_codec_tl1_acodec.o += $(PR_FMT_DEFINE_TL1)
$(MODULE_NAME_TL1)-y += aml_codec_tl1_acodec.o


#Third part codecs
MODULE_NAME_AD82584F = amlogic-snd-codec-ad82584f
PR_FMT_AD82584F = $(subst amlogic-,,$(MODULE_NAME_AD82584F))
PR_FMT_DEFINE_AD82584F = "-Dpr_fmt(fmt)= \"[$(PR_FMT_AD82584F)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_SOC_AD82584F) += $(MODULE_NAME_AD82584F).o
CFLAGS_ad82584f.o += $(PR_FMT_DEFINE_AD82584F)
$(MODULE_NAME_AD82584F)-y += ad82584f.o

MODULE_NAME_TLV320ADC3101 = amlogic-snd-codec-tlv320adc3101
PR_FMT_TLV320ADC3101 = $(subst amlogic-,,$(MODULE_NAME_TLV320ADC3101))
PR_FMT_DEFINE_TLV320ADC3101 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_TLV320ADC3101)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_SOC_TLV320ADC3101) += $(MODULE_NAME_TLV320ADC3101).o
CFLAGS_tlv320adc3101.o += $(PR_FMT_DEFINE_TLV320ADC3101)
$(MODULE_NAME_TLV320ADC3101)-y += tlv320adc3101.o

MODULE_NAME_TAS5707 = amlogic-snd-codec-tas5707
PR_FMT_TAS5707 = $(subst amlogic-,,$(MODULE_NAME_TAS5707))
PR_FMT_DEFINE_TAS5707 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_TAS5707)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_SOC_TAS5707) += $(MODULE_NAME_TAS5707).o
CFLAGS_tas5707.o += $(PR_FMT_DEFINE_TAS5707)
$(MODULE_NAME_TAS5707)-y += tas5707.o

MODULE_NAME_TAS5805 = amlogic-snd-codec-tas5805
PR_FMT_TAS5805 = $(subst amlogic-,,$(MODULE_NAME_TAS5805))
PR_FMT_DEFINE_TAS5805 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_TAS5805)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_SOC_TAS5805) += $(MODULE_NAME_TAS5805).o
CFLAGS_tas5805.o += $(PR_FMT_DEFINE_TAS5805)
$(MODULE_NAME_TAS5805)-y += tas5805.o

MODULE_NAME_TAS5805M = amlogic-snd-codec-tas5805m
PR_FMT_TAS5805M = $(subst amlogic-,,$(MODULE_NAME_TAS5805M))
PR_FMT_DEFINE_TAS5805M = "-Dpr_fmt(fmt)= \"[$(PR_FMT_TAS5805M)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_SOC_TAS5805m) += $(MODULE_NAME_TAS5805M).o
CFLAGS_tas5805m.o += $(PR_FMT_DEFINE_TAS5805M)
$(MODULE_NAME_TAS5805M)-y += tas5805m.o

MODULE_NAME_AD82128 = amlogic-snd-codec-ad82128
PR_FMT_AD82128 = $(subst amlogic-,,$(MODULE_NAME_AD82128))
PR_FMT_DEFINE_AD82128 = "-Dpr_fmt(fmt)= \"[$(PR_FMT_AD82128)]: \" fmt"
obj-$(CONFIG_AMLOGIC_SND_SOC_AD82128) += $(MODULE_NAME_AD82128).o
CFLAGS_ad82128.o += $(PR_FMT_DEFINE_AD82128)
$(MODULE_NAME_AD82128)-y += ad82128.o
