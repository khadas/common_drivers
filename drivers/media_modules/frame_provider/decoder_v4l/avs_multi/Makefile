MODULE_NAME = amvdec_mavs_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AVS_MULTI) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += avs_multi.o

PR_FMT = "avs"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
