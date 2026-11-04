cmake_minimum_required(VERSION 3.10)
project(unit_tests_lib_ux LANGUAGES C)

set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${SDK_DIR}/cmake/LedgerUT.cmake)

ledger_unit_tests_init()

# BAGL flows only exist on Nano devices, the default UT setup targets Flex (NBGL).
ledger_unit_tests_add_test(
    NAME            test_ux_flow_buttons
    SOURCES         ${SDK_DIR}/lib_ux/src/ux_flow_engine.c
                    ${SDK_DIR}/lib_ux/src/ux_stack.c
                    ${SDK_DIR}/lib_ux/src/ux_layout_paging.c
                    ${SDK_DIR}/io/src/os_io_seph_ux.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${SDK_DIR}
                    ${SDK_DIR}/include
                    ${SDK_DIR}/include/arm
                    ${SDK_DIR}/target/nanox/include
                    ${SDK_DIR}/lib_ux/include
                    ${SDK_DIR}/lib_bagl/include
                    ${SDK_DIR}/lib_cxng/include
                    ${SDK_DIR}/io/include
                    ${SDK_DIR}/io_legacy/include
                    ${SDK_DIR}/protocol/include
                    ${SDK_DIR}/lib_standard_app
    COMPILE_DEFS    TARGET_NANOX
                    OS_UNIT_TEST
                    noreturn=
                    gcc
                    __IO=volatile
                    NATIVE_LITTLE_ENDIAN
                    HAVE_BAGL
                    HAVE_UX_FLOW
                    BAGL_WIDTH=128
                    BAGL_HEIGHT=64
                    OS_IO_SEPROXYHAL
                    REVAMPED_IO
                    USE_OS_IO_STACK
                    OS_IO_SEPH_BUFFER_SIZE=272
                    IO_HID_EP_LENGTH=64
                    IO_USB_MAX_ENDPOINTS=4
                    HAVE_ECC
                    HAVE_ECC_WEIERSTRASS
                    HAVE_SECP256K1_CURVE
                    HAVE_HASH
                    HAVE_SHA256
    COMPILE_OPTIONS -fsanitize=address,undefined -fno-sanitize-recover=all
    LINK_OPTIONS    -fsanitize=address,undefined
)

# BAGL screen macros (UX_DISPLAY & co), in the flow configuration used by applications.
ledger_unit_tests_add_test(
    NAME            test_ux_display_buttons
    SOURCES         ${SDK_DIR}/lib_ux/src/ux_flow_engine.c
                    ${SDK_DIR}/lib_ux/src/ux_stack.c
                    ${SDK_DIR}/lib_ux/src/ux_layout_paging.c
                    ${SDK_DIR}/io/src/os_io_seph_ux.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${SDK_DIR}
                    ${SDK_DIR}/include
                    ${SDK_DIR}/include/arm
                    ${SDK_DIR}/target/nanox/include
                    ${SDK_DIR}/lib_ux/include
                    ${SDK_DIR}/lib_bagl/include
                    ${SDK_DIR}/lib_cxng/include
                    ${SDK_DIR}/io/include
                    ${SDK_DIR}/io_legacy/include
                    ${SDK_DIR}/protocol/include
                    ${SDK_DIR}/lib_standard_app
    COMPILE_DEFS    TARGET_NANOX
                    OS_UNIT_TEST
                    noreturn=
                    gcc
                    __IO=volatile
                    NATIVE_LITTLE_ENDIAN
                    HAVE_BAGL
                    HAVE_UX_FLOW
                    BAGL_WIDTH=128
                    BAGL_HEIGHT=64
                    OS_IO_SEPROXYHAL
                    REVAMPED_IO
                    USE_OS_IO_STACK
                    OS_IO_SEPH_BUFFER_SIZE=272
                    IO_HID_EP_LENGTH=64
                    IO_USB_MAX_ENDPOINTS=4
                    HAVE_ECC
                    HAVE_ECC_WEIERSTRASS
                    HAVE_SECP256K1_CURVE
                    HAVE_HASH
                    HAVE_SHA256
    COMPILE_OPTIONS -fsanitize=address,undefined -fno-sanitize-recover=all
    LINK_OPTIONS    -fsanitize=address,undefined
)
