# Makefile to build and launch screenshots
# The generated screenshots can be used to validate NBGL Application Use Cases
GCC 				?= gcc
PRODUCT_NAME ?=$(word 1, $(subst _golden, , $(subst _clean, , $(subst _screenshots, ,$(MAKECMDGOALS)))))

TARGET_PRODUCT_NAME ?= $(PRODUCT_NAME)
# possible products
PRODUCT_NAMES := stax nanox flex nanosp apex_p

ifeq (, $(filter $(PRODUCT_NAMES), $(PRODUCT_NAME)))
$(error "Product $(PRODUCT_NAME) not found")
endif

SRC_DIR			:= src
PRODUCT_DIR	    := build/$(PRODUCT_NAME)
BUILD_DIR		:= $(PRODUCT_DIR)
GEN_DIR			:= $(BUILD_DIR)/generated
OBJ_DIR			:= $(BUILD_DIR)/obj
BIN_DIR			:= $(BUILD_DIR)/bin
PUBLIC_SDK_DIR	:= ../..
APP_PATH  := $(SRC_DIR)/apps
GOLDEN_PATH  := golden_samples

SCREENSHOTS_SRC_DIR := .


WARNINGS 			:= -Wall -Wextra \
				   -Wshadow -Wundef -Wmaybe-uninitialized -Wmissing-prototypes -Wno-discarded-qualifiers \
				   -Wunused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized \
				   -Wunused-parameter -Wno-missing-field-initializers -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default  \
				   -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wsign-compare -Wdouble-promotion -Wclobbered -Wdeprecated  \
				   -Wempty-body -Wshift-negative-value -Wstack-usage=2048 \
           -Wtype-limits -Wsizeof-pointer-memaccess -Wpointer-arith \
           -Werror

CFLAGS 			:= -O0 -g $(WARNINGS)

# Add simulator define to allow modification of source
DEFINES+=              \
                       LINUX_SIMU=1 \
                       VERSION=\"$(VERSION)\" \
                       WITH_STDIO \
                       HAVE_NBGL \
                       HAVE_SEED_COOKIE \
                       HAVE_ECC \
                       HAVE_SHA256 \
                       HAVE_SHA512 \
                       HAVE_PBKDF2 \
                       HAVE_RNG \
                       HAVE_HASH \
                       HAVE_INDEXED_STRINGS \
                       NBGL_KEYBOARD \
                       NBGL_KEYPAD \
                       HAVE_RECOVER \
                       OS_IO_SEPROXYHAL \
                       USB_SEGMENT_SIZE=64 \
                       IO_HID_EP_LENGTH=64 \
                       OS_IO_SEPH_BUFFER_SIZE=300 \
                       BUILD_SCREENSHOTS

ifeq ($(PRODUCT_NAME),stax)
TARGET_DEFINES += \
                       TARGET_STAX \
                       HAVE_SIDE_SCREEN \
                       HAVE_BAGL_FONT_INTER_REGULAR_24PX \
                       HAVE_BAGL_FONT_INTER_SEMIBOLD_24PX \
                       HAVE_BAGL_FONT_INTER_MEDIUM_32PX \
                       HAVE_BAGL_FONT_INTER_SEMIBOLD_24PX_1BPP \
                       SCREEN_SIZE_WALLET

TARGET_ID           := 0x33200004
SMALL_ICON_SIZE := 32px
LARGE_ICON_SIZE := 64px
APP_ICON_SIZE   := 32px
endif

ifeq ($(PRODUCT_NAME),flex)
TARGET_DEFINES += \
                       TARGET_FLEX \
                       HAVE_BAGL_FONT_INTER_REGULAR_28PX \
                       HAVE_BAGL_FONT_INTER_SEMIBOLD_28PX \
                       HAVE_BAGL_FONT_INTER_MEDIUM_36PX \
                       SCREEN_SIZE_WALLET

TARGET_ID           := 0x33300004
SMALL_ICON_SIZE := 40px
LARGE_ICON_SIZE := 64px
APP_ICON_SIZE   := 40px
endif

ifeq ($(PRODUCT_NAME),nanox)
TARGET_DEFINES += \
                       TARGET_NANOX \
                       HAVE_BATTERY \
                       HAVE_HARDWARE_VERSIONS \
                       SCREEN_SIZE_NANO \
                       HAVE_BLE

TARGET_ID           := 0x33000004
endif

ifeq ($(PRODUCT_NAME),nanosp)
TARGET_DEFINES += \
                       TARGET_NANOSP \
                       SCREEN_SIZE_NANO

TARGET_ID           := 0x33100004
TARGET_PRODUCT_NAME := nanos2
endif

ifeq ($(PRODUCT_NAME),apex_p)
TARGET_DEFINES += \
                       TARGET_APEX \
                       TARGET_APEX_P \
                       HAVE_BAGL_FONT_NANOTEXT_MEDIUM_18PX \
                       HAVE_BAGL_FONT_NANOTEXT_BOLD_18PX \
                       HAVE_BAGL_FONT_NANODISPLAY_SEMIBOLD_24PX \
                       SCREEN_SIZE_WALLET \
                       MONITOR_ZOOM=2 \
                       NBGL_MASKING

TARGET_ID           := 0x33300004
SMALL_ICON_SIZE := 24px
LARGE_ICON_SIZE := 48px
APP_ICON_SIZE   := 32px_1bpp
endif


#if Nano
ifeq (, $(filter $(TARGET_DEFINES), SCREEN_SIZE_WALLET))
TARGET_DEFINES += \
                       MONITOR_ZOOM=4 \
                       HAVE_BRIGHTNESS_SETTING \
                       NBGL_USE_CASE \
                       NBGL_STEP \
                       HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX \
                       HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX \
                       HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX \
                       HAVE_SE_SCREEN \
                       HAVE_SEPROXYHAL_MCU

FLOWS_PATH  := $(SCREENSHOTS_SRC_DIR)/flows/nano

GLYPH_OPT := --reverse

else #if wallet
TARGET_DEFINES += \
                       HAVE_BLE \
                       HAVE_BATTERY \
                       HAVE_SE_TOUCH \
                       HAVE_NFC \
                       HAVE_PIEZO_SOUND \
                       HAVE_BACKGROUND_IMG \
                       NBGL_QRCODE \
                       NBGL_PAGE \
                       NBGL_USE_CASE \
                       HAVE_SE_EINK_DISPLAY

FLOWS_PATH  := $(SCREENSHOTS_SRC_DIR)/flows/wallet
endif

DEFINES += $(TARGET_DEFINES)

PROPERTIES_FILENAME = $(PRODUCT_NAME).properties

GLYPH_DESTC := $(GEN_DIR)/glyphs.c
GLYPH_DESTH := $(GEN_DIR)/glyphs.h

APP_ICON_DESTC := $(GEN_DIR)/app_icons.c
APP_ICON_DESTH := $(GEN_DIR)/app_icons.h

NBGL_PATH := $(PUBLIC_SDK_DIR)/lib_nbgl

INC:=                  \
                       $(APP_PATH) \
                       $(SRC_DIR)/main \
                       $(SRC_DIR)/uzlib \
                       $(PUBLIC_SDK_DIR)/target/$(TARGET_PRODUCT_NAME)/include \
                       $(PUBLIC_SDK_DIR)/include \
                       $(PUBLIC_SDK_DIR)/lib_ux_nbgl \
                       $(PUBLIC_SDK_DIR)/lib_cxng/include \
                       $(PUBLIC_SDK_DIR)/io/include \
                       $(PUBLIC_SDK_DIR)/io_legacy/include \
                       $(NBGL_PATH)/include \
                       $(PUBLIC_SDK_DIR)/qrcode/include \
                       $(GEN_DIR)

# Link with NBGL shared library provided in Bolos build
LDLIBS              := -lm -lbsd -lssl -lcrypto -lnbgl_shared_screenshots_$(PRODUCT_NAME)
LDFLAGS             := -Xlinker -Map=$(BUILD_DIR)/memory.map -Lshared_libs
BIN                 := $(BIN_DIR)/simulator

COMPILE              = $(GCC) $(CFLAGS) $(addprefix -I,$(INC)) $(addprefix -D,$(DEFINES))

# Automatically include all source files
SRCS       := $(shell find $(APP_PATH) -type f -name '*.c')
SRCS       += $(shell find $(SRC_DIR)/uzlib -type f -name '*.c')
SRCS       += $(shell find $(SRC_DIR)/main -type f -name '*.c')
LIB_SRCS   := $(shell find $(NBGL_PATH)/src -type f -name '*.c')
LIB_SRCS   += $(shell find $(PUBLIC_SDK_DIR)/qrcode/src -type f -name '*.c')

ifneq (, $(filter $(TARGET_DEFINES), SCREEN_SIZE_WALLET))
APP_GLYPHS_PATH := $(APP_PATH)/glyphs/$(APP_ICON_SIZE)/
GLYPH_FILES = $(addprefix $(APP_PATH)/glyphs/$(LARGE_ICON_SIZE)/,$(sort $(notdir $(shell find $(APP_PATH)/glyphs/$(LARGE_ICON_SIZE)/))))
else
APP_GLYPHS_PATH := $(APP_PATH)/glyphs/nano/
GLYPH_FILES = $(addprefix $(APP_GLYPHS_PATH),$(sort $(notdir $(shell find $(APP_GLYPHS_PATH)))))
endif
APP_ICON_FILENAMES = $(sort $(notdir $(shell find $(APP_GLYPHS_PATH))))
APP_ICON_FILES = $(addprefix $(APP_GLYPHS_PATH),$(APP_ICON_FILENAMES))

ifneq (, $(filter $(TARGET_DEFINES), SCREEN_SIZE_WALLET))
GLYPH_FILES += $(addprefix $(NBGL_PATH)/glyphs/wallet/,$(sort $(notdir $(shell find $(NBGL_PATH)/glyphs/wallet/))))
GLYPH_FILES += $(addprefix $(NBGL_PATH)/glyphs/$(LARGE_ICON_SIZE)/,$(sort $(notdir $(shell find $(NBGL_PATH)/glyphs/$(LARGE_ICON_SIZE)/))))
GLYPH_FILES += $(addprefix $(NBGL_PATH)/glyphs/$(SMALL_ICON_SIZE)/,$(sort $(notdir $(shell find $(NBGL_PATH)/glyphs/$(SMALL_ICON_SIZE)/))))
else
GLYPH_FILES += $(addprefix $(NBGL_PATH)/glyphs/nano/,$(sort $(notdir $(shell find $(NBGL_PATH)/glyphs/nano/))))
endif

ICON_SCRIPT := $(NBGL_PATH)/tools/icon2glyph.py

OBJECTS    := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(SRCS:.c=.o))
OBJECTS    += $(patsubst $(PUBLIC_SDK_DIR)/%,$(OBJ_DIR)/%,$(LIB_SRCS:.c=.o))
OBJECTS    += $(OBJ_DIR)/glyphs.o
OBJECTS    += $(OBJ_DIR)/app_icons.o

# Creation of glyphs.c, containing all icons, from all
$(GLYPH_DESTH): $(GLYPH_FILES) $(ICON_SCRIPT) | $(GEN_DIR)
	@echo [GLYPH] Compiling... $@
	@python3 $(ICON_SCRIPT) $(GLYPH_OPT) --glyphcheader $(GLYPH_DESTH) --glyphcfile $(GLYPH_DESTC) $(GLYPH_FILES)

$(GLYPH_DESTC): $(GLYPH_DESTH)

# Creation of special app_icon.h, containing all app icons,
$(APP_ICON_DESTH): $(APP_ICON_FILES) hexbitmap2c.py | $(GEN_DIR)
	@echo [ICON] Creating... $@
	@rm -f $(ICON_HEX_FILE)
	@echo "/* Generated */" > $(APP_ICON_DESTC)
	@echo "/* Generated */" > $(APP_ICON_DESTH)
	@for file in $(APP_ICON_FILENAMES) ; do \
	  python3 $(ICON_SCRIPT) $(GLYPH_OPT) --hexbitmap tmp_$$file.hex $(APP_GLYPHS_PATH)/$$file ; \
	  python3 hexbitmap2c.py --hexbitmap tmp_$$file.hex --inc $(APP_ICON_DESTH) --src $(APP_ICON_DESTC) --variable $$file; \
    rm -f tmp_$$file.hex; \
	done

$(APP_ICON_DESTC): $(APP_ICON_DESTH)

# Compile all C source files, except generated ones into .o (and .d for dependencies)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(GLYPH_DESTH) $(APP_ICON_DESTH) | $(OBJ_DIR)
	@echo 'Compiling source file: $<'
	@mkdir -p $(dir $@)
	@$(COMPILE) -c -MMD -MP -MF"$(OBJ_DIR)/$*.d" -o "$@" "$<"

# Compile lib C source files, except generated ones into .o (and .d for dependencies)
$(OBJ_DIR)/%.o: $(PUBLIC_SDK_DIR)/%.c $(GLYPH_DESTH) $(APP_ICON_DESTH) | $(OBJ_DIR)
	@echo 'Compiling source file: $<'
	@mkdir -p $(dir $@)
	@$(COMPILE) -c -MMD -MP -MF"$(OBJ_DIR)/$*.d" -o "$@" "$<"

# Compile generated C source files into .o (and .d for dependencies)
$(OBJ_DIR)/%.o: $(GEN_DIR)/%.c | $(OBJ_DIR)
	@echo 'Compiling generated file: $<'
	@$(COMPILE) -c -MMD -MP -MF"$(OBJ_DIR)/$*.d" -o "$@" "$<"

# Rule to link all objects into the final binary
$(BIN): $(OBJECTS) | $(BIN_DIR)
	@echo 'Linking $@'
	@$(GCC) -o $@ $+ $(LDFLAGS) -ljson-c -lpng ${LDLIBS}

# Rule to create directories
$(BIN_DIR) $(OBJ_DIR) $(GEN_DIR) $(BUILD_DIR) $(BUILD_DIR)/screenshots:
	@mkdir -p $@

.PHONY: screenshots

# how to generate all screenshots for all scenarios (flows)
# at the end, it creates a top level index
%_screenshots: $(BIN) | $(BUILD_DIR)/screenshots
	@echo "Building... "$@
	@python3 $(SCREENSHOTS_SRC_DIR)/launch_screenshots.py -b $(BIN) -w $(BUILD_DIR)/screenshots -f $(FLOWS_PATH) -p $(PROPERTIES_FILENAME) -n $(PRODUCT_NAME)
	@python3 $(SCREENSHOTS_SRC_DIR)/compare_screenshots.py -r $(GOLDEN_PATH)/$(PRODUCT_NAME) -t $(BUILD_DIR)/screenshots

# Update golden sample
%_golden:
	@python3 $(SCREENSHOTS_SRC_DIR)/compare_screenshots.py -r $(GOLDEN_PATH)/$(PRODUCT_NAME) -t $(BUILD_DIR)/screenshots -g

%_clean:
	rm -rf $(PRODUCT_DIR)


##
## Nanox
##

nanox: $(BIN)

nanox_clean:

nanox_screenshots: $(BIN)

#
##
## NanoSP
##

nanosp: $(BIN)

nanosp_clean:

nanosp_screenshots: $(BIN)

#
## Stax
##

stax: $(BIN)

stax_clean:

stax_screenshots: $(BIN)

##
## Flex
##

flex: $(BIN)

flex_clean:

flex_screenshots: $(BIN)

##
## Apex+
##

apex_p: $(BIN)

apex_p_clean:

apex_p_screenshots: $(BIN)

-include $(OBJECTS:%.o=%.d)
