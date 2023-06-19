MODULE_NAME = media_clock
obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y += ../chips/chips.o
$(MODULE_NAME)-y += clk/clkg12.o
$(MODULE_NAME)-y += clk/clk.o
$(MODULE_NAME)-y += switch/amports_gate.o
$(MODULE_NAME)-y += ../chips/decoder_cpu_ver_info.o
$(MODULE_NAME)-y += ../register/register.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
