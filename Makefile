

GCCBIN = gcc-arm/bin
CC = $(GCCBIN)/arm-none-eabi-gcc
AR = $(GCCBIN)/arm-none-eabi-ar
OBJCOPY = $(GCCBIN)/arm-none-eabi-objcopy
OBJDUMP = $(GCCBIN)/arm-none-eabi-objdump

DEBUG_OPTS = -g3 -gdwarf-2 -gstrict-dwarf
OPTS = -Os
TARGET = -mcpu=cortex-m0
CFLAGS = -ffunction-sections -fdata-sections -Wall -Wa,-adhlns="$@.lst" \
		 -fmessage-length=0 $(TARGET) -mthumb -mfloat-abi=soft \
		 $(DEBUG_OPTS) $(OPTS) -I .

LIBOBJS = _startup.o syscalls.o uart.o delay.o accel.o touch.o usb.o \
		ring.o tests.o

INCLUDES = freedom.h common.h

.PHONY:	clean gcc-arm

# -------------------------------------------------------------------------------

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
	
# -------------------------------------------------------------------------------
# Download and unpack the GCC ARM embedded toolchain (binaries)

ARCH=
ifeq ($(shell uname -s), Darwin)
	ARCH=mac
else
	ARCH=linux
endif
DLPATH=https://launchpad.net/gcc-arm-embedded/4.7/4.7-2013-q2-update/+download/gcc-arm-none-eabi-4_7-2013q2-20130614-$(ARCH).tar.bz2

gcc-arm:
	curl --location $(DLPATH) | tar jx
	ln -s `ls -Artd gcc-arm-none-eabi* | tail -n 1` gcc-arm
