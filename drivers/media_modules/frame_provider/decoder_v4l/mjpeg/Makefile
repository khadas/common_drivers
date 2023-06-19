MODULE_NAME = amvdec_mmjpeg_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_MJPEG_MULTI) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vmjpeg_multi.o

PR_FMT = "mjpeg"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
