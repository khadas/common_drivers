MODULE_NAME = amvdec_avs3_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AVS2) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vavs3.o avs3_bufmgr.o dec_eco.o com_picman.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
