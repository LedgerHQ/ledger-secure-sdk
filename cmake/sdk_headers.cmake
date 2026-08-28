#######################################################################
#                        ledger::sdk-headers                          #
#######################################################################
set(LEDGER_SDK_INCLUDE_DIRS
    ${LEDGER_SDK_ROOT}
    ${LEDGER_GLYPHS_GEN_DIR}
    ${LEDGER_SDK_ROOT}/include
    ${LEDGER_SDK_ROOT}/include/arm
    ${LEDGER_SDK_ROOT}/target/${LEDGER_TARGET}/include
    ${LEDGER_SDK_ROOT}/io/include
    ${LEDGER_SDK_ROOT}/io_legacy/include
    ${LEDGER_SDK_ROOT}/protocol/include
    ${LEDGER_SDK_ROOT}/qrcode/include
    ${LEDGER_SDK_ROOT}/lib_alloc
    ${LEDGER_SDK_ROOT}/lib_blewbxx/include
    ${LEDGER_SDK_ROOT}/lib_blewbxx_impl/include
    ${LEDGER_SDK_ROOT}/lib_cxng/include
    ${LEDGER_SDK_ROOT}/lib_cxng/src
    ${LEDGER_SDK_ROOT}/lib_lists
    ${LEDGER_SDK_ROOT}/lib_nbgl/include
    ${LEDGER_SDK_ROOT}/lib_nbgl/src
    ${LEDGER_SDK_ROOT}/lib_pki
    ${LEDGER_SDK_ROOT}/lib_standard_app
    ${LEDGER_SDK_ROOT}/lib_stusb/include
    ${LEDGER_SDK_ROOT}/lib_stusb_impl/include
    ${LEDGER_SDK_ROOT}/lib_tlv
    ${LEDGER_SDK_ROOT}/lib_tlv/use_cases
    ${LEDGER_SDK_ROOT}/lib_u2f/include
    ${LEDGER_SDK_ROOT}/lib_u2f_legacy/include
    ${LEDGER_SDK_ROOT}/lib_ux_nbgl
)

add_library(ledger_sdk_headers INTERFACE)
add_library(ledger::sdk-headers ALIAS ledger_sdk_headers)

target_include_directories(ledger_sdk_headers INTERFACE ${LEDGER_SDK_INCLUDE_DIRS})

target_link_libraries(ledger_sdk_headers INTERFACE nbgl_glyphs)
