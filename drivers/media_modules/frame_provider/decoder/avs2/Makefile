MODULE_NAME = amvdec_avs2
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AVS2) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vavs2.o avs2_bufmgr.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
