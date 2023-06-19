MODULE_NAME = amvdec_vp9_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_VP9) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vvp9.o

PR_FMT = "vp9"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
