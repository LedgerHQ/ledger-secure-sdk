# These are includes necessary to compile apps unit tests and generate mocks.
set(UT_INCLUDE_DIRS_SDK
    ${SDK_DIR}
    ${SDK_DIR}/lib_cxng/include
    ${SDK_DIR}/lib_blewbxx/include/
    ${SDK_DIR}/lib_tlv/
    ${SDK_DIR}/lib_tlv/use_cases/
    ${SDK_DIR}/lib_pki/
    ${SDK_DIR}/qrcode/include/
    ${SDK_DIR}/io/include/
    ${SDK_DIR}/protocol/include/
    ${SDK_DIR}/lib_stusb/include/
    ${SDK_DIR}/lib_stusb_impl/include/
    ${SDK_DIR}/lib_u2f/include/
    ${SDK_DIR}/lib_standard_app/
    ${SDK_DIR}/lib_ux_nbgl/
    ${SDK_DIR}/io_legacy/include/
    ${SDK_DIR}/lib_u2f_legacy/include/
    ${SDK_DIR}/target/flex/include/
    ${SDK_DIR}/lib_nbgl/src/
    ${SDK_DIR}/lib_nbgl/include/
    ${SDK_DIR}/include
    ${SDK_DIR}/include/arm
    CACHE INTERNAL "Include directories for unit tests"
)
