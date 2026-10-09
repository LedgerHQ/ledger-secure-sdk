#######################################################################
#                          Ledger target profile                      #
#######################################################################
# The device description (TARGET, LEDGER_TARGET_*) comes from the ledger-target-* presets.
# TARGET is also an if() operator: always test it as "${TARGET}".
if("${TARGET}" STREQUAL "" OR NOT DEFINED LEDGER_TARGET_DEFS)
    message(FATAL_ERROR "No device description: configure with a preset")
endif()

# Cached, for ledger_app() which runs in the app scope.
string(TOUPPER "TARGET_${TARGET}" _target_name)
set(TARGET_NAME ${_target_name} CACHE INTERNAL "Target name, as in Makefile.target")

# Same value as the Makefile build.
file(STRINGS ${LEDGER_SDK_ROOT}/Makefile.defines _api_level_line REGEX "^API_LEVEL *:=")
string(REGEX MATCH "[0-9]+" _api_level "${_api_level_line}")
set(LEDGER_API_LEVEL ${_api_level} CACHE STRING "API level exposed to the app")
set(LEDGER_APPVERSION ""   CACHE STRING "Application version string")
if(NOT LEDGER_APPVERSION)
    set(LEDGER_APPVERSION "${CMAKE_PROJECT_VERSION}")
endif()
# Resolved value, for ledger_app() which runs in the app scope.
set(LEDGER_APP_VERSION "${LEDGER_APPVERSION}" CACHE INTERNAL "Application version")

# From the environment by default, overridable by a preset.
set(LEDGER_APPNAME "$ENV{APPNAME}" CACHE STRING "Application name")

#######################################################################
#                           Feature options                           #
#######################################################################
option(ENABLE_ADDRESS_BOOK "Build with the address book feature" OFF)
option(ENABLE_ADDRESS_BOOK_LEDGER_ACCOUNT "Address book for Ledger accounts" OFF)
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

# Makefile.standard_app:130-132
option(DEBUG_OS_STACK_CONSUMPTION "Track the stack consumption of the OS" OFF)

option(DISABLE_STANDARD_USB "Drop the USB stack" OFF)
option(DISABLE_STANDARD_WEBUSB "Drop WebUSB" OFF)
option(DISABLE_STANDARD_U2F "Drop the U2F transport" OFF)
option(DISABLE_STANDARD_SNPRINTF "Use newlib's snprintf instead of the SDK one" OFF)
option(DISABLE_STANDARD_APP_FILES "Drop lib_standard_app" OFF)
option(DISABLE_STANDARD_SEPROXYHAL "Drop OS_IO_SEPROXYHAL" OFF)
option(DISABLE_STANDARD_APP_SYNC_RAPDU "Drop STANDARD_APP_SYNC_RAPDU" OFF)
option(DISABLE_OS_IO_STACK_USE "Drop USE_OS_IO_STACK" OFF)

if(ENABLE_ADDRESS_BOOK)
    set(ENABLE_TLV_LIBRARY ON)
endif()

#######################################################################
#                          Resolved features                          #
#######################################################################
set(LEDGER_FEATURE_BLE    OFF)
set(LEDGER_FEATURE_NFC    OFF)
set(LEDGER_FEATURE_QRCODE OFF)

if(ENABLE_BLUETOOTH AND LEDGER_TARGET_HAS_BLE)
    set(LEDGER_FEATURE_BLE ON)
endif()

if(ENABLE_NFC AND LEDGER_TARGET_HAS_NFC)
    set(LEDGER_FEATURE_NFC ON)
endif()

if(ENABLE_NBGL_QRCODE AND LEDGER_TARGET_HAS_QRCODE)
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

#######################################################################
#                          Application flags                          #
#######################################################################
# Same computation as Makefile.standard_app, see include/appflags.h.
option(HAVE_APPLICATION_FLAG_DERIVE_MASTER "APPLICATION_FLAG_DERIVE_MASTER" OFF)
option(HAVE_APPLICATION_FLAG_GLOBAL_PIN "APPLICATION_FLAG_GLOBAL_PIN" OFF)
option(HAVE_APPLICATION_FLAG_BOLOS_SETTINGS "APPLICATION_FLAG_BOLOS_SETTINGS" OFF)
option(HAVE_APPLICATION_FLAG_LIBRARY "APPLICATION_FLAG_LIBRARY" OFF)
option(HAVE_APPLICATION_FLAG_NOT_REVIEWED "APPLICATION_FLAG_NOT_REVIEWED" OFF)
option(HAVE_APPLICATION_FLAG_PRELOADED "APPLICATION_FLAG_PRELOADED" OFF)
set(CUSTOM_APP_FLAGS 0x000 CACHE STRING "Extra application flags")

set(_app_flags 0)
if(HAVE_APPLICATION_FLAG_DERIVE_MASTER)
    math(EXPR _app_flags "${_app_flags} + 0x010")
endif()
if(HAVE_APPLICATION_FLAG_GLOBAL_PIN)
    math(EXPR _app_flags "${_app_flags} + 0x040")
endif()
if(HAVE_APPLICATION_FLAG_BOLOS_SETTINGS OR LEDGER_FEATURE_BLE OR LEDGER_FEATURE_NFC)
    math(EXPR _app_flags "${_app_flags} + 0x200")
endif()
if(HAVE_APPLICATION_FLAG_LIBRARY OR ENABLE_SWAP)
    math(EXPR _app_flags "${_app_flags} + 0x800")
endif()
if(HAVE_APPLICATION_FLAG_NOT_REVIEWED)
    math(EXPR _app_flags "${_app_flags} + 0x20000")
endif()
if(HAVE_APPLICATION_FLAG_PRELOADED)
    math(EXPR _app_flags "${_app_flags} | 0x20")
endif()
math(EXPR _app_flags "${_app_flags} + ${CUSTOM_APP_FLAGS}" OUTPUT_FORMAT HEXADECIMAL)
set(LEDGER_APP_FLAGS ${_app_flags} CACHE INTERNAL "Application flags")

include(${CMAKE_CURRENT_LIST_DIR}/common_defines.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/crypto_defines.cmake)

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

#######################################################################
#                          App metadata                               #
#######################################################################
set(LEDGER_APP_DEFS "")

if(LEDGER_APPVERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    list(APPEND LEDGER_APP_DEFS
        MAJOR_VERSION=${CMAKE_MATCH_1}
        MINOR_VERSION=${CMAKE_MATCH_2}
        PATCH_VERSION=${CMAKE_MATCH_3})
endif()

# Apps only: the SDK built alone has no manifest.
if(NOT ledger-secure-sdk_IS_TOP_LEVEL)
    include(${CMAKE_CURRENT_LIST_DIR}/../app/manifest.cmake)
    ledger_app_manifest_read(metadata author _author)
    if(NOT LEDGER_APP_COPYRIGHT)
        message(FATAL_ERROR "ledger_app: COPYRIGHT is required")
    endif()
    list(APPEND LEDGER_APP_DEFS
        APP_METADATA_AUTHOR="${_author}"
        APP_METADATA_COPYRIGHT="${LEDGER_APP_COPYRIGHT}")
endif()

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
    TARGET="${TARGET}"
    TARGET_NAME="${TARGET_NAME}"
    API_LEVEL=${LEDGER_API_LEVEL}
    APPNAME="${LEDGER_APPNAME}"
    APPVERSION="${LEDGER_APPVERSION}"
    ${LEDGER_APP_DEFS}
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


if(ENABLE_ADDRESS_BOOK)
    target_compile_definitions(ledger_target_profile INTERFACE HAVE_ADDRESS_BOOK)
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

if(DEBUG_OS_STACK_CONSUMPTION)
    target_compile_definitions(ledger_target_profile INTERFACE DEBUG_OS_STACK_CONSUMPTION=1)
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

target_compile_definitions(ledger_target_profile INTERFACE IO_HID_EP_LENGTH=64)

if(NOT DISABLE_STANDARD_SEPROXYHAL)
    target_compile_definitions(ledger_target_profile INTERFACE OS_IO_SEPROXYHAL)
endif()

if(NOT DISABLE_STANDARD_APP_SYNC_RAPDU)
    target_compile_definitions(ledger_target_profile INTERFACE STANDARD_APP_SYNC_RAPDU)
endif()

if(NOT DISABLE_OS_IO_STACK_USE)
    target_compile_definitions(ledger_target_profile INTERFACE USE_OS_IO_STACK)
endif()

# XXX DEBUG (HAVE_PRINTF) not handled yet.
# Function-style macro: not supported by compile definitions.
target_compile_options(ledger_target_profile INTERFACE "-DPRINTF(...)=")

get_target_property(_profile ledger_target_profile INTERFACE_COMPILE_DEFINITIONS)
string(REPLACE ";" "\n" _profile "${_profile}")
file(WRITE ${CMAKE_BINARY_DIR}/target_profile_defines.txt "${_profile}\n")
