MODULE_NAME = amvdec_vc1_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_VC1) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vvc1.o

PR_FMT = "vc1"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
