project(unit_tests_lib_ux LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

# BAGL flows only exist on Nano devices, the default UT setup targets Flex (NBGL).
ledger_unit_tests_add_test(
    NAME            test_ux_flow_buttons
    NO_TARGET_PROFILE
    SOURCES         ${LEDGER_SDK_ROOT}/lib_ux/src/ux_flow_engine.c
                    ${LEDGER_SDK_ROOT}/lib_ux/src/ux_stack.c
                    ${LEDGER_SDK_ROOT}/lib_ux/src/ux_layout_paging.c
                    ${LEDGER_SDK_ROOT}/io/src/os_io_seph_ux.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${LEDGER_SDK_ROOT}
                    ${LEDGER_SDK_ROOT}/include
                    ${LEDGER_SDK_ROOT}/include/arm
                    ${LEDGER_SDK_ROOT}/target/nanox/include
                    ${LEDGER_SDK_ROOT}/lib_ux/include
                    ${LEDGER_SDK_ROOT}/lib_bagl/include
                    ${LEDGER_SDK_ROOT}/lib_cxng/include
                    ${LEDGER_SDK_ROOT}/io/include
                    ${LEDGER_SDK_ROOT}/io_legacy/include
                    ${LEDGER_SDK_ROOT}/protocol/include
                    ${LEDGER_SDK_ROOT}/lib_standard_app
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
    NO_TARGET_PROFILE
    SOURCES         ${LEDGER_SDK_ROOT}/lib_ux/src/ux_flow_engine.c
                    ${LEDGER_SDK_ROOT}/lib_ux/src/ux_stack.c
                    ${LEDGER_SDK_ROOT}/lib_ux/src/ux_layout_paging.c
                    ${LEDGER_SDK_ROOT}/io/src/os_io_seph_ux.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${LEDGER_SDK_ROOT}
                    ${LEDGER_SDK_ROOT}/include
                    ${LEDGER_SDK_ROOT}/include/arm
                    ${LEDGER_SDK_ROOT}/target/nanox/include
                    ${LEDGER_SDK_ROOT}/lib_ux/include
                    ${LEDGER_SDK_ROOT}/lib_bagl/include
                    ${LEDGER_SDK_ROOT}/lib_cxng/include
                    ${LEDGER_SDK_ROOT}/io/include
                    ${LEDGER_SDK_ROOT}/io_legacy/include
                    ${LEDGER_SDK_ROOT}/protocol/include
                    ${LEDGER_SDK_ROOT}/lib_standard_app
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
