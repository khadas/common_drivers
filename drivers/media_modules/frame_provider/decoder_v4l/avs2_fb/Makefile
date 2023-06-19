MODULE_NAME = amvdec_avs2_fb_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AVS2) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vavs2_fb_v4l.o avs2_bufmgr.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
