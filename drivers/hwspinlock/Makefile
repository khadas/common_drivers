MODULE_NAME = amlogic-hwspinlock
obj-$(CONFIG_AMLOGIC_HWSPINLOCK) += $(MODULE_NAME).o

$(MODULE_NAME)-y += hwspinlock_main.o hwspinlock_bakery.o hwspinlock.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
