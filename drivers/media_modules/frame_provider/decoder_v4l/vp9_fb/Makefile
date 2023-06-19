MODULE_NAME = amvdec_vp9_fb_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_VP9_FB) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vvp9_fb_v4l.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
