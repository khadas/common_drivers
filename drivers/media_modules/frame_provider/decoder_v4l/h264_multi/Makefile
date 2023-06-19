MODULE_NAME = amvdec_mh264_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_H264_MULTI) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vmh264.o h264_dpb.o

PR_FMT = "h264"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
