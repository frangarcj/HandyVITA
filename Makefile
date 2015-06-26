#
# Copyright (c) 2015 Sergi Granell (xerpi)
# based on Cirne's vita-toolchain test Makefile
#

TARGET		:= HANDY
HANDY := handy-0.95
ZLIB := $(HANDY)/zlib-113
SOURCES		:= src $(ZLIB)
DATA		:= data
INCLUDES	:= src


#BUILD_ZLIB=$(ZLIB)/unzip.o

BUILD_APP=$(HANDY)/Cart.o $(HANDY)/Susie.o $(HANDY)/Mikie.o $(HANDY)/Memmap.o $(HANDY)/Ram.o $(HANDY)/Rom.o $(HANDY)/System.o $(HANDY)/C65c02.o



CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CXXFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CXXFILES:.cpp=.o)  $(BUILD_APP)

LIBS = -lc -lrdimon -lSceDisplay_stub -lSceGxm_stub 	\
	-lSceCtrl_stub -lSceTouch_stub

DEFINES	=	-DPSP -DHANDY_AUDIO_BUFFER_SIZE=4096 -DGZIP_STATE


PREFIX  = arm-none-eabi
AS	= $(PREFIX)-as
CC      = $(PREFIX)-gcc
CXX			=$(PREFIX)-g++
READELF = $(PREFIX)-readelf
OBJDUMP = $(PREFIX)-objdump
CFLAGS  = -Wall -specs=$(PSP2SDK)/psp2.specs -I$(DATA)  $(DEFINES)
CXXFLAGS = $(CFLAGS) -O2 -mword-relocations -fomit-frame-pointer -fno-unwind-tables -fno-rtti -fno-exceptions -Wno-deprecated -std=gnu++11
ASFLAGS = $(CFLAGS)



all: $(TARGET).velf

$(TARGET).velf: $(TARGET).elf
	psp2-fixup -q -S $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CFLAGS) $^ $(LIBS) -o $@


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
