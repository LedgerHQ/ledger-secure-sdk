#######################################################################
#                          Ledger target profile                      #
#######################################################################
set(LEDGER_TARGETS nanox nanos2 stax flex apex_p apex_m)

if(NOT LEDGER_TARGET)
    set(LEDGER_TARGET "flex")
endif()

if(NOT LEDGER_TARGET IN_LIST LEDGER_TARGETS)
    message(FATAL_ERROR "LEDGER_TARGET='${LEDGER_TARGET}' is not one of: ${LEDGER_TARGETS}")
endif()

set(LEDGER_API_LEVEL 26    CACHE STRING "API level exposed to the app")
set(LEDGER_APPVERSION ""   CACHE STRING "Application version string")

# From the environment by default, overridable by a preset.
set(LEDGER_APPNAME "$ENV{APPNAME}" CACHE STRING "Application name")

#######################################################################
#                           Feature options                           #
#######################################################################
option(ENABLE_ADDRESS_BOOK "Build with the address book feature" OFF)
option(ENABLE_BLUETOOTH "Build with BLE support" OFF)
option(ENABLE_NFC "Build with NFC support" OFF)
option(ENABLE_NFC_READER "Build with the NFC reader" OFF)
option(ENABLE_DYNAMIC_ALLOC "Build with the dynamic allocator (lib_alloc)" OFF)
option(ENABLE_LISTS_LIBRARY "Build with the linked-list library (lib_lists)" OFF)
option(ENABLE_TLV_LIBRARY "Build with the TLV library (lib_tlv)" OFF)
option(ENABLE_PKI_LIBRARY "Build with the PKI library (lib_pki)" OFF)
option(ENABLE_USB_CCID "Build with the USB CCID class (lib_ccid)" OFF)
option(ENABLE_NBGL_QRCODE "NBGL QR code support" OFF)
option(ENABLE_NBGL_KEYBOARD "NBGL keyboard support" OFF)
option(ENABLE_NBGL_KEYPAD "NBGL keypad support" OFF)

option(ENABLE_SWAP "Build with the swap feature" OFF)
option(ENABLE_TESTING_SWAP "Build with the swap feature, Speculos only" OFF)

option(DISABLE_STANDARD_USB "Drop the USB stack" OFF)
option(DISABLE_STANDARD_WEBUSB "Drop WebUSB" OFF)
option(DISABLE_STANDARD_U2F "Drop the U2F transport" OFF)
option(DISABLE_STANDARD_SNPRINTF "Use newlib's snprintf instead of the SDK one" OFF)
option(DISABLE_STANDARD_APP_FILES "Drop lib_standard_app" OFF)

if(ENABLE_ADDRESS_BOOK)
    set(ENABLE_TLV_LIBRARY ON CACHE BOOL "" FORCE)
endif()

#######################################################################
#                          Resolved features                          #
#######################################################################
set(LEDGER_FEATURE_BLE    OFF)
set(LEDGER_FEATURE_NFC    OFF)
set(LEDGER_FEATURE_QRCODE OFF)

if(ENABLE_BLUETOOTH AND LEDGER_TARGET MATCHES "^(nanox|stax|flex|apex_p)$")
    set(LEDGER_FEATURE_BLE ON)
endif()

if(ENABLE_NFC AND LEDGER_TARGET MATCHES "^(stax|flex|apex_p)$")
    set(LEDGER_FEATURE_NFC ON)
endif()

if(ENABLE_NBGL_QRCODE AND LEDGER_TARGET MATCHES "^(stax|flex|apex_p|apex_m)$")
    set(LEDGER_FEATURE_QRCODE ON)
endif()

if(DISABLE_STANDARD_USB)
    set(LEDGER_FEATURE_USB OFF)
else()
    set(LEDGER_FEATURE_USB ON)
endif()
if(DISABLE_STANDARD_U2F)
    set(LEDGER_FEATURE_U2F OFF)
else()
    set(LEDGER_FEATURE_U2F ON)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/targets/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/targets/crypto.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/targets/${LEDGER_TARGET}.cmake)

#######################################################################
#                          SDK metadata                               #
#######################################################################
find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} -C ${LEDGER_SDK_ROOT} describe --tags --exact-match --match "v[0-9]*" --dirty
                OUTPUT_VARIABLE LEDGER_SDK_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} -C ${LEDGER_SDK_ROOT} describe --always --dirty --exclude "*" --abbrev=40
                OUTPUT_VARIABLE LEDGER_SDK_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
foreach(_var LEDGER_SDK_VERSION LEDGER_SDK_HASH)
    if(NOT ${_var})
        set(${_var} "None")
    endif()
endforeach()

# Address book needs a larger buffer
if(ENABLE_ADDRESS_BOOK)
    set(LEDGER_SEPH_BUFFER_SIZE 448)
else()
    set(LEDGER_SEPH_BUFFER_SIZE 272)
endif()

#######################################################################
#                        ledger::target-profile                       #
#######################################################################
add_library(ledger_target_profile INTERFACE)
add_library(ledger::target-profile ALIAS ledger_target_profile)

target_compile_definitions(ledger_target_profile INTERFACE
    ${LEDGER_COMMON_DEFS}
    ${LEDGER_CRYPTO_DEFS}
    ${LEDGER_TARGET_DEFS}
    API_LEVEL=${LEDGER_API_LEVEL}
    APPNAME="${LEDGER_APPNAME}"
    APPVERSION="${LEDGER_APPVERSION}"
    SDK_NAME="ledger-secure-sdk"
    SDK_VERSION="${LEDGER_SDK_VERSION}"
    SDK_HASH="${LEDGER_SDK_HASH}"
    OS_IO_SEPH_BUFFER_SIZE=${LEDGER_SEPH_BUFFER_SIZE}
)

if(LEDGER_FEATURE_BLE)
    target_compile_definitions(ledger_target_profile INTERFACE
        HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000 HAVE_BLE_APDU)
endif()

if(LEDGER_FEATURE_NFC)
    target_compile_definitions(ledger_target_profile INTERFACE HAVE_NFC)

    if(ENABLE_NFC_READER)
        target_compile_definitions(ledger_target_profile INTERFACE HAVE_NFC_READER)
    endif()
endif()


if(LEDGER_FEATURE_QRCODE)
    target_compile_definitions(ledger_target_profile INTERFACE NBGL_QRCODE)
endif()
if(ENABLE_NBGL_KEYBOARD)
    target_compile_definitions(ledger_target_profile INTERFACE NBGL_KEYBOARD)
endif()
if(ENABLE_NBGL_KEYPAD)
    target_compile_definitions(ledger_target_profile INTERFACE NBGL_KEYPAD)
endif()

if(ENABLE_SWAP OR ENABLE_TESTING_SWAP)
    target_compile_definitions(ledger_target_profile INTERFACE HAVE_SWAP)
endif()


if(LEDGER_FEATURE_USB)
    target_compile_definitions(ledger_target_profile INTERFACE
        HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 HAVE_USB_APDU
        USB_SEGMENT_SIZE=64)
endif()

if(ENABLE_USB_CCID)
    target_compile_definitions(ledger_target_profile INTERFACE HAVE_CCID_USB)
endif()

if(NOT DISABLE_STANDARD_WEBUSB)
    set(APP_WEBUSB_URL "" CACHE STRING "WebUSB URL exposed by the app")
    string(LENGTH "${APP_WEBUSB_URL}" _weburl_len)
    target_compile_definitions(ledger_target_profile INTERFACE
        HAVE_WEBUSB WEBUSB_URL_SIZE_B=${_weburl_len} WEBUSB_URL=)
endif()

if(LEDGER_FEATURE_U2F)
    target_compile_definitions(ledger_target_profile INTERFACE HAVE_IO_U2F)
endif()

if(NOT DISABLE_STANDARD_SNPRINTF)
    target_compile_definitions(ledger_target_profile INTERFACE
        HAVE_SPRINTF HAVE_SNPRINTF_FORMAT_U HAVE_SNPRINTF_FORMAT_LL)
endif()

get_target_property(_profile ledger_target_profile INTERFACE_COMPILE_DEFINITIONS)
string(REPLACE ";" "\n" _profile "${_profile}")
file(WRITE ${CMAKE_BINARY_DIR}/target_profile_defines.txt "${_profile}\n")
