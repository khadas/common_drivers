MODULE_NAME = amvdec_h264mvc
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_H264_MVC) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vh264_mvc.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)

