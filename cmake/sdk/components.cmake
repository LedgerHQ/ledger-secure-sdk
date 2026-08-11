#######################################################################
#                        Ledger SDK components                        #
#######################################################################
set(_ledger_sdk_dirs "")
set(_ledger_sdk_targets "")

macro(_ledger_sdk_component dir target)
    list(APPEND _ledger_sdk_dirs ${dir})
    list(APPEND _ledger_sdk_targets ${target})
endmacro()

#######################################################################
#                          Always present                             #
#######################################################################
_ledger_sdk_component(src          sdk_core)
_ledger_sdk_component(io           io)
_ledger_sdk_component(io_legacy    io_legacy)
_ledger_sdk_component(protocol     protocol)
_ledger_sdk_component(lib_u2f_legacy lib_u2f_legacy)

# XXX not handling bagl in cmake
_ledger_sdk_component(lib_nbgl     lib_nbgl)
_ledger_sdk_component(lib_ux_nbgl  lib_ux_nbgl)

#######################################################################
#                        Selected by the profile                      #
#######################################################################
if(NOT DISABLE_STANDARD_APP_FILES)
    _ledger_sdk_component(lib_standard_app lib_standard_app)
endif()

if(LEDGER_FEATURE_USB)
    _ledger_sdk_component(lib_stusb      lib_stusb)
    _ledger_sdk_component(lib_stusb_impl lib_stusb_impl)
endif()

if(ENABLE_USB_CCID)
    _ledger_sdk_component(lib_ccid       lib_ccid)
    _ledger_sdk_component(lib_stusb_impl lib_stusb_impl)
endif()

if(LEDGER_FEATURE_U2F)
    _ledger_sdk_component(lib_u2f lib_u2f)
endif()

if(LEDGER_FEATURE_BLE)
    _ledger_sdk_component(lib_blewbxx      lib_blewbxx)
    _ledger_sdk_component(lib_blewbxx_impl lib_blewbxx_impl)
endif()

if(LEDGER_FEATURE_NFC)
    _ledger_sdk_component(lib_nfc lib_nfc)
endif()

if(ENABLE_DYNAMIC_ALLOC)
    _ledger_sdk_component(lib_alloc lib_alloc)
endif()

if(ENABLE_LISTS_LIBRARY)
    _ledger_sdk_component(lib_lists lib_lists)
endif()

if(ENABLE_ADDRESS_BOOK)
    _ledger_sdk_component(app_features/address_book address_book)
endif()

if(ENABLE_TLV_LIBRARY)
    _ledger_sdk_component(lib_tlv lib_tlv)
endif()

if(ENABLE_PKI_LIBRARY)
    _ledger_sdk_component(lib_pki lib_pki)
endif()

if(LEDGER_FEATURE_QRCODE)
    _ledger_sdk_component(qrcode qrcode)
endif()

list(REMOVE_DUPLICATES _ledger_sdk_dirs)
list(REMOVE_DUPLICATES _ledger_sdk_targets)

#######################################################################
#                          Host-only additions                        #
#######################################################################
if(NOT LEDGER_DEVICE_BUILD)
    set(_ledger_host_dirs
        lib_alloc lib_ccid lib_cxng lib_lists lib_nfc lib_pki lib_standard_app
        lib_stusb lib_stusb_impl lib_tlv lib_u2f qrcode)
    if(ENABLE_ADDRESS_BOOK)
        list(APPEND _ledger_host_dirs app_features/address_book)
    endif()
    list(APPEND _ledger_sdk_dirs ${_ledger_host_dirs})
    list(REMOVE_DUPLICATES _ledger_sdk_dirs)
endif()

#######################################################################
#                              Build them                             #
#######################################################################
# lib_nbgl first: it declares nbgl_glyphs, which the others query for.
list(REMOVE_ITEM _ledger_sdk_dirs lib_nbgl)
add_subdirectory(lib_nbgl)

foreach(_dir IN LISTS _ledger_sdk_dirs)
    add_subdirectory(${_dir})
endforeach()

#######################################################################
#                             ledger::sdk                             #
#######################################################################
add_library(ledger_sdk INTERFACE)
add_library(ledger::sdk ALIAS ledger_sdk)

target_link_libraries(ledger_sdk INTERFACE
    ledger::sdk-headers
    ledger::target-profile
    ledger::compile-options
    ${_ledger_sdk_targets}
)

if(TARGET sdk_app_metadata)
    target_sources(ledger_sdk INTERFACE $<TARGET_OBJECTS:sdk_app_metadata>)
endif()

string(REPLACE ";" "\n" _components "${_ledger_sdk_targets}")
file(WRITE ${CMAKE_BINARY_DIR}/sdk_components.txt "${_components}\n")
