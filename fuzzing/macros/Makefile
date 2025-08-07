##
# Extract defines given the specified device
#
# @file
# @version 0.1
ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

########################################
#        Mandatory configuration       #
########################################
# Application name
APPNAME = "Fuzzing"

# Application version
APPVERSION_M = 1
APPVERSION_N = 0
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src
CURVE_APP_LOAD_PARAMS = secp256k1
PATH_APP_LOAD_PARAMS = "44'/1'"   # purpose=coin(44) / coin_type=Testnet(1)
VARIANT_PARAM = COIN
VARIANT_VALUES = BOL
ENABLE_BLUETOOTH = 1
ENABLE_NFC = 1
ENABLE_NBGL_FOR_NANO_DEVICES = 1
ENABLE_NBGL_QRCODE = 1

ifeq ($(APPNAME), "Fuzzing")
ENABLE_TESTING_SWAP = 1
endif

include $(BOLOS_SDK)/Makefile.standard_app

list-defines:
	@echo "$(DEFINES)"
