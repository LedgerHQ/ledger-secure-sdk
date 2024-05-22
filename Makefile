.PHONY: doc

BUILD_DIR := build

# default target is stax
TARGET ?= stax

NBGL_DEFINES_stax := \
                     HAVE_SE_TOUCH \
                     NBGL_PAGE \
					 NBGL_QRCODE \
					 SCREEN_SIZE_WALLET

NBGL_DEFINES_nano := \
                     NBGL_STEP \
					 NBGL_FLOW \
					 SCREEN_SIZE_NANO

NBGL_DEFINES := $(NBGL_DEFINES_$(TARGET))

export NBGL_DEFINES

doc: | $(BUILD_DIR)
	@doxygen doc/Doxyfile
	@echo 'HTML doc can be found in '$(BUILD_DIR)/doc/html/index.html

$(BUILD_DIR):
	@mkdir -p $@
