# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

# use this func, the host build not found asm/types.h
# userprogs-always-y += debug_file_app
# userccflags += -pthread

#source file
SOURCE  := $(wildcard *.c)
OBJS    := $(patsubst %.c, %.o, $(SOURCE))

#target you can change test to what you want
TARGET  := debug_file_app

#compile and lib parameter
CC	:= ../../../../../prebuilts/gcc/linux-x86/host/x86_64-arm-10.3-2021.07/bin/arm-none-linux-gnueabihf-gcc
LD	:= ../../../../../prebuilts/gcc/linux-x86/host/x86_64-arm-10.3-2021.07/bin/arm-none-linux-gnueabihf-ld
LIBS	:= -lpthread
LDFLAGS	:= -static
DEFINES	:=
INCLUDE	:= -I.
CFLAGS	:= -g -Wall -O2 $(DEFINES) $(INCLUDE)

.PHONY : all clean
all : $(TARGET)

clean :
	rm -f $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

