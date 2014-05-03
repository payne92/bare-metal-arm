

# If the GCC ARM tools are already on the path, use them. Otherwise, use 
# the local version in gcc-arm/bin
GCC_ARM_VERSION := $(shell arm-none-eabi-gcc --version 2>/dev/null)
ifdef GCC_ARM_VERSION
	GCCDIR=
else
	GCCDIR=gcc-arm/bin/
endif

CC = $(GCCDIR)arm-none-eabi-gcc
AR = $(GCCDIR)arm-none-eabi-ar
OBJCOPY = $(GCCDIR)arm-none-eabi-objcopy
OBJDUMP = $(GCCDIR)arm-none-eabi-objdump

DEBUG_OPTS = -g3 -gdwarf-2 -gstrict-dwarf
OPTS = -Os
TARGET = -mcpu=cortex-m0
CFLAGS = -ffunction-sections -fdata-sections -Wall -Wa,-adhlns="$@.lst" \
		 -fmessage-length=0 $(TARGET) -mthumb -mfloat-abi=soft \
		 $(DEBUG_OPTS) $(OPTS) -I .

LIBOBJS = _startup.o syscalls.o uart.o delay.o accel.o touch.o usb.o \
		ring.o tests.o

INCLUDES = freedom.h common.h

.PHONY:	clean gcc-arm deploy

# -----------------------------------------------------------------------------

all: demo.srec demo.dump

libbare.a: $(LIBOBJS)
	$(AR) -rv libbare.a $(LIBOBJS)

clean:
	rm -f *.o *.lst *.out libbare.a *.srec *.dump

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.dump: %.out
	$(OBJDUMP) --disassemble $< >$@

%.srec: %.out
	$(OBJCOPY) -O srec $< $@

%.out: %.o mkl25z4.ld libbare.a
	$(CC) $(CFLAGS) -T mkl25z4.ld -o $@ $< libbare.a

# -----------------------------------------------------------------------------
# Burn/deploy by copying to the development board filesystem
#  Hack:  we identify the board by the filesystem size (128mb)
DEPLOY_VOLUME = $(shell df -h 2>/dev/null | fgrep " 128M" | awk '{print $$6}')
deploy: demo.srec
	dd conv=fsync bs=64k if=$< of=$(DEPLOY_VOLUME)/$<
	
# -----------------------------------------------------------------------------
# Download and unpack the GCC ARM embedded toolchain (binaries)

DLPATH=https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q1-update/+download/gcc-arm-none-eabi-4_8-2014q1-20140314

ifeq ($(shell uname -s), Darwin)
	DL_CMD=curl --location $(DLPATH)-mac.tar.bz2
else
	DL_CMD=wget --verbose $(DLPATH)-linux.tar.bz2 -O -
endif

gcc-arm:
	$(DL_CMD) | tar jx
	ln -s `ls -Artd gcc-arm-none-eabi* | tail -n 1` gcc-arm
