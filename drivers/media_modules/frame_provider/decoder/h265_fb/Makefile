MODULE_NAME = amvdec_h265_fb
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_H265_FB) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vh265_fb.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
