MODULE_NAME = stream_input
obj-m	+=	 ${MODULE_NAME}.o

${MODULE_NAME}-objs	+=	amports/amstream.o
${MODULE_NAME}-objs	+=	amports/adec.o
${MODULE_NAME}-objs	+=	amports/thread_rw.o
${MODULE_NAME}-objs	+=	amports/streambuf.o
${MODULE_NAME}-objs	+=	amports/stream_buffer_base.o
${MODULE_NAME}-objs	+=	amports/stream_buffer_interface.o

ifndef CONFIG_AMLOGIC_MEDIA_NO_PARSER
${MODULE_NAME}-objs	+=	parser/esparser.o
${MODULE_NAME}-objs	+=	parser/tsdemux.o
endif
${MODULE_NAME}-objs	+=	parser/dvb_common.o
${MODULE_NAME}-objs	+=	subtitle/subtitle.o

obj-$(CONFIG_AMLOGIC_HW_DEMUX)   += parser/hw_demux/
obj-$(CONFIG_AMLOGIC_DVB)   += parser/dvb_ci/

ccflags-y += -I.
ccflags-y += -I$(srctree)/include/media

#obj-y 	+= 	tv_frontend/
# obj-y	+=	box-frontend/avl6211/
# obj-y	+=	box-frontend/atbm8881/
# obj-y	+=	box-frontend/avl68xx/
