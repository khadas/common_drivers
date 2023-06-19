MODULE_NAME = amvdec_mmpeg12
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_MPEG2_MULTI) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vmpeg12_multi.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
