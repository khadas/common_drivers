MODULE_NAME = amvdec_av1_fb_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AV1_FB) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vav1_fb_v4l.o av1_bufmgr.o av1_film_grain.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
