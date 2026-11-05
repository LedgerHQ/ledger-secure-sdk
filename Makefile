.PHONY: doc doc-wallet doc-nano doc-all

BUILD_DIR := build
DOC_SITE_DIR := $(BUILD_DIR)/doc/site

# default target is stax
TARGET ?= stax

# Every nano% target (nanox, nanos2, ...) is Nano, everything else is wallet (touch).
ifneq ($(filter nano%,$(TARGET)),)
DOC_FAMILY := nano
else
DOC_FAMILY := wallet
endif

NBGL_DEFINES_wallet := \
                     HAVE_SE_TOUCH \
                     NBGL_PAGE \
                     NBGL_QRCODE \
                     SCREEN_SIZE_WALLET

NBGL_DEFINES_nano := \
                     NBGL_STEP \
                     NBGL_FLOW \
                     SCREEN_SIZE_NANO

# Wallet docs drop Nano/BAGL-only headers that clash with the touch symbols
UX_BAGL_EXCLUDE_wallet   := lib_ux/include/ux_bagl.h
NBGL_NANO_EXCLUDE_wallet := lib_nbgl/src/nbgl_layout_internal_nanos.h

NBGL_DEFINES      := $(NBGL_DEFINES_$(DOC_FAMILY))
UX_BAGL_EXCLUDE   := $(UX_BAGL_EXCLUDE_$(DOC_FAMILY))
NBGL_NANO_EXCLUDE := $(NBGL_NANO_EXCLUDE_$(DOC_FAMILY))
export NBGL_DEFINES
export UX_BAGL_EXCLUDE
export NBGL_NANO_EXCLUDE

doc: | $(BUILD_DIR)/doc
	@rm -rf $(BUILD_DIR)/doc/html
	@doxygen doc/Doxyfile
	@echo "HTML doc for '$(TARGET)' can be found in '$(BUILD_DIR)/doc/html/index.html'"

doc-wallet:
	@$(MAKE) --no-print-directory TARGET=stax doc

doc-nano:
	@$(MAKE) --no-print-directory TARGET=nanox doc

doc-all:
	@mkdir -p $(DOC_SITE_DIR)
	@$(MAKE) --no-print-directory TARGET=stax doc
	@mkdir -p $(DOC_SITE_DIR)/wallet
	@cp -a $(BUILD_DIR)/doc/html/. $(DOC_SITE_DIR)/wallet/
	@$(MAKE) --no-print-directory TARGET=nanox doc
	@mkdir -p $(DOC_SITE_DIR)/nano
	@cp -a $(BUILD_DIR)/doc/html/. $(DOC_SITE_DIR)/nano/
	@cp doc/landing.html $(DOC_SITE_DIR)/index.html
	@echo 'Combined documentation can be found in '$(DOC_SITE_DIR)/index.html

$(BUILD_DIR) $(BUILD_DIR)/doc:
	@mkdir -p $@
