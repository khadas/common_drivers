MODULE_NAME = amvdec_av1
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AV1) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vav1.o av1_bufmgr.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
