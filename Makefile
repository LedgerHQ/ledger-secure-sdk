.PHONY: doc doc-wallet doc-nano doc-all

BUILD_DIR := build
DOC_SITE_DIR := $(BUILD_DIR)/doc/site

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

# For nano-family targets (nano, nanox, nanosplus, etc.), default to nano defines.
ifeq ($(strip $(NBGL_DEFINES)),)
ifneq ($(filter nano%,$(TARGET)),)
NBGL_DEFINES := $(NBGL_DEFINES_nano)
endif
endif

export NBGL_DEFINES

doc: | $(BUILD_DIR)
	@doxygen doc/Doxyfile
	@echo "HTML doc for '$(TARGET)' can be found in '$(BUILD_DIR)/doc/html/index.html'"

doc-wallet:
	@$(MAKE) TARGET=stax doc

doc-nano:
	@$(MAKE) TARGET=nanox doc

doc-all:
	@mkdir -p $(DOC_SITE_DIR)
	@$(MAKE) TARGET=stax doc
	@mkdir -p $(DOC_SITE_DIR)/wallet
	@cp -a $(BUILD_DIR)/doc/html/. $(DOC_SITE_DIR)/wallet/
	@$(MAKE) TARGET=nanox doc
	@mkdir -p $(DOC_SITE_DIR)/nano
	@cp -a $(BUILD_DIR)/doc/html/. $(DOC_SITE_DIR)/nano/
	@cp doc/landing.html $(DOC_SITE_DIR)/index.html
	@echo 'Combined documentation can be found in '$(DOC_SITE_DIR)/index.html

$(BUILD_DIR):
	@mkdir -p $@
