MODULE_NAME = amvdec_av1_t5d_v4l
obj-$(CONFIG_AMLOGIC_MEDIA_VDEC_AV1_T5D) += ${MODULE_NAME}.o
${MODULE_NAME}-objs += vav1_t5d.o av1_bufmgr_t5d.o av1_film_grain_t5d.o

PR_FMT = $(subst amvdec_,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
