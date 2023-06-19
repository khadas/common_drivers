MODULE_NAME = amvdec_mmpeg4_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_MPEG4_MULTI) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vmpeg4_multi.o

PR_FMT = "mpeg4"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
