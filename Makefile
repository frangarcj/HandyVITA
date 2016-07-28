#
# Copyright (c) 2015 Sergi Granell (xerpi)
# based on Cirne's vita-toolchain test Makefile
#

TARGET		:= HANDY
HANDY := lynx
ZLIB := $(HANDY)/zlib-113
#SOURCES		:= src $(ZLIB)
SOURCES		:= src

INCLUDES	:= -Isrc


#BUILD_ZLIB=$(ZLIB)/unzip.o

BUILD_APP=$(HANDY)/c65c02.o $(HANDY)/lynxdec.o $(HANDY)/Cart.o $(HANDY)/Memmap.o $(HANDY)/Mikie.o $(HANDY)/Ram.o $(HANDY)/Rom.o $(HANDY)/Susie.o $(HANDY)/System.o



CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CXXFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
OBJS     := $(CFILES:.c=.o) $(BUILD_APP) $(CXXFILES:.cpp=.o)

LIBS= -lvita2d -lm -lstdc++ -lSceSysmodule_stub -lSceCommonDialog_stub -lSceDisplay_stub -lSceGxm_stub -lScePower_stub 	\
	-lSceCtrl_stub -lSceAudio_stub

DEFINES	=	-DPSP -DLSB_FIRST -DWANT_CRC32 -DLINUX_PATCH



PREFIX  = arm-vita-eabi
AS	= $(PREFIX)-as
CC      = $(PREFIX)-gcc
CXX			=$(PREFIX)-g++
READELF = $(PREFIX)-readelf
OBJDUMP = $(PREFIX)-objdump
CFLAGS  = -Wl,-q -O3 $(INCLUDES) $(DEFINES) -fno-exceptions \
					-fno-unwind-tables -fno-asynchronous-unwind-tables -O3 -ftree-vectorize \
					-mfloat-abi=hard -ffast-math -fsingle-precision-constant -ftree-vectorizer-verbose=2 -fopt-info-vec-optimized -funroll-loops
CXXFLAGS = $(CFLAGS) -fno-rtti -Wno-deprecated -Wno-comment -Wno-sequence-point -std=c++11
ASFLAGS = $(CFLAGS)



all: eboot.bin

eboot.bin: $(TARGET).velf
	vita-make-fself $(TARGET).velf $@
	vita-mksfoex -s TITLE_ID=FRAN00002 "HandyVITA" param.sfo
$(TARGET).velf: $(TARGET).elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@ > /dev/null

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).elf $(TARGET).velf $(OBJS) $(DATA)/*.h

copy: eboot.bin
	@cp eboot.bin param.sfo /mnt/shared/HandyVITA

