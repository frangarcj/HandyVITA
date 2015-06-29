#
# Copyright (c) 2015 Sergi Granell (xerpi)
# based on Cirne's vita-toolchain test Makefile
#

TARGET		:= HANDY
HANDY := lynx
ZLIB := $(HANDY)/zlib-113
#SOURCES		:= src $(ZLIB)
SOURCES		:= src

DATA		:= data
INCLUDES	:= src


#BUILD_ZLIB=$(ZLIB)/unzip.o

BUILD_APP=$(HANDY)/lynxdec.o $(HANDY)/Cart.o $(HANDY)/Memmap.o $(HANDY)/Mikie.o $(HANDY)/Ram.o $(HANDY)/Rom.o $(HANDY)/Susie.o $(HANDY)/System.o



CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CXXFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(BUILD_APP) $(CXXFILES:.cpp=.o)

LIBS = -lc_stub -lstdc++_stub -lSceKernel_stub -lSceDisplay_stub -lSceGxm_stub 	\
	-lSceCtrl_stub -lSceTouch_stub

DEFINES	=	-DPSP -DHANDY_AUDIO_BUFFER_SIZE=4096 -DGZIP_STATE -DLSB_FIRST -DWANT_CRC32


PREFIX  = arm-none-eabi
AS	= $(PREFIX)-as
CC      = $(PREFIX)-gcc
CXX			=$(PREFIX)-g++
READELF = $(PREFIX)-readelf
OBJDUMP = $(PREFIX)-objdump
CFLAGS  = -Wall -specs=$(PSP2SDK)/psp2.specs -I$(DATA)  $(DEFINES)
CXXFLAGS = $(CFLAGS) -O2 -mword-relocations -fomit-frame-pointer -fno-unwind-tables -fno-rtti -fno-exceptions -Wno-deprecated
ASFLAGS = $(CFLAGS)



all: $(TARGET).velf

$(TARGET).velf: $(TARGET).elf
	psp2-fixup -q -S $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@


.SUFFIXES: .bin

%.bin.o: %.bin
	$(bin2o)

define bin2o
	C:\\devkitPro\\devkitARM\\bin\\bin2s $< | $(AS) -o $(@)
	echo "extern const unsigned char" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > $(DATA)/`(echo $(<F) | tr . _)`.h
	echo "extern const unsigned char" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> $(DATA)/`(echo $(<F) | tr . _)`.h
	echo "extern const unsigned int" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> $(DATA)/`(echo $(<F) | tr . _)`.h
endef

clean:
	@rm -rf $(TARGET).elf $(TARGET).velf $(OBJS) $(DATA)/*.h

copy: $(TARGET).velf
	@cp $(TARGET).velf ~/shared/$(TARGET).elf
