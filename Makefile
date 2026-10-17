#---------------------------------------------------------------------------------
# Super Mayo Boss - Legend of the Lost Kingdom
# 3DS homebrew build (devkitARM + libctru + citro2d)
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET  : output name
# BUILD   : object/intermediate directory
# SOURCES : source directories
# INCLUDES: header directories
# ROMFS   : romfs directory (relative to this Makefile)
#---------------------------------------------------------------------------------
TARGET   := mayro
BUILD    := build
SOURCES  := source
INCLUDES := include
ROMFS    := romfs

#---------------------------------------------------------------------------------
# SMDH metadata shown by the 3DS HOME menu.
# Built by devkitARM's 3ds_rules into $(TARGET).smdh, then embedded in the .3dsx.
#   APP_TITLE       -> "Short Description" field
#   APP_DESCRIPTION -> "Long Description" field
#   APP_AUTHOR      -> "Publisher" field
#   APP_ICON        -> 144x144 icon.png (auto-detected below, override with ICON=)
#---------------------------------------------------------------------------------
export APP_TITLE       := Supra Mayro Bross
export APP_DESCRIPTION := Tiem deafet Bigger Gumba
export APP_AUTHOR      := MisakiP_ / Misakii-P

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
	-ffunction-sections -fdata-sections \
	$(ARCH)

CFLAGS += $(INCLUDE) -D__3DS__

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS := -g $(ARCH)
LDFLAGS  = -specs=3dsx.specs -g $(ARCH) -Wl,--gc-sections -Wl,-Map,$(notdir $*.map)

# Order matters: citro2d sits on top of citro3d, which sits on top of ctru.
LIBS := -lcitro2d -lcitro3d -lctru -lvorbisidec -logg -lm

LIBDIRS := $(CTRULIB)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
	$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD := $(CC)

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
	-I$(DEVKITPRO)/portlibs/3ds/include \
	-I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(DEVKITPRO)/portlibs/3ds/lib

export _3DSXDEPS := $(if $(NO_SMDH),,$(OUTPUT).smdh)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean

#---------------------------------------------------------------------------------
# Every .png in assets/ becomes romfs/gfx/<name>.t3x (a citro2d sprite sheet)
#---------------------------------------------------------------------------------
ROMFS_STAMP := .romfs_stamp
GFX_PNGS    := $(sort $(wildcard assets/*.png))
GFX_T3X     := $(patsubst assets/%.png,$(ROMFS)/gfx/%.t3x,$(GFX_PNGS))

# Fonts (.bcfnt) are copied verbatim into the romfs.
FONT_FILES  := $(sort $(wildcard fonts/*.bcfnt))
FONT_ROMFS  := $(patsubst fonts/%,$(ROMFS)/fonts/%,$(FONT_FILES))

AUDIO_FILES := $(sort $(wildcard audio/*.ogg))
AUDIO_ROMFS := $(patsubst audio/%,$(ROMFS)/audio/%,$(AUDIO_FILES))

#---------------------------------------------------------------------------------
all: $(BUILD) $(GFX_T3X) $(FONT_ROMFS) $(AUDIO_ROMFS)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

#---------------------------------------------------------------------------------
# PNG -> .t3x (citro2d sprite sheets). rgba keeps alpha for later sprite work.
#---------------------------------------------------------------------------------
$(ROMFS)/gfx/%.t3x : assets/%.png
	@mkdir -p $(dir $@)
	@echo tex3ds $(notdir $<)
	@tex3ds -f rgba -z auto -o $@ $<

#---------------------------------------------------------------------------------
# .bcfnt fonts are copied verbatim into the romfs.
#---------------------------------------------------------------------------------
$(ROMFS)/fonts/%.bcfnt : fonts/%.bcfnt
	@mkdir -p $(dir $@)
	@cp $< $@

#---------------------------------------------------------------------------------
# .ogg audio files are copied verbatim into the romfs.
#---------------------------------------------------------------------------------
$(ROMFS)/audio/%.ogg : audio/%.ogg
	@mkdir -p $(dir $@)
	@cp $< $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(ROMFS)
	@rm -f .romfs_stamp

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx : $(OUTPUT).elf $(_3DSXDEPS)

$(OUTPUT).elf  : $(OFILES)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
