MODULE_NAME = amvdec_avs2_fb
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AVS2_FB) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vavs2_fb.o avs2_bufmgr.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
