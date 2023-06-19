MODULE_NAME = amvdec_h265_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_H265) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vh265.o

PR_FMT = "h265"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
