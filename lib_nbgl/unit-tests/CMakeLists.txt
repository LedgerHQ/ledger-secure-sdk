project(unit_tests_lib_nbgl LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

# Nano (non touch) build of the NBGL use cases: UT_COMPILE_DEFS_SDK targets Flex, so the
# Nano defines are given here explicitly.
ledger_unit_tests_add_test(
    NAME            test_nbgl_use_case_nanos
    NO_TARGET_PROFILE
    SOURCES         ${LEDGER_SDK_ROOT}/lib_nbgl/src/nbgl_use_case_nanos.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${LEDGER_SDK_ROOT}
                    ${LEDGER_SDK_ROOT}/include
                    ${LEDGER_SDK_ROOT}/io/include
                    ${LEDGER_SDK_ROOT}/lib_cxng/include
                    ${LEDGER_SDK_ROOT}/lib_nbgl/include
                    ${LEDGER_SDK_ROOT}/lib_nbgl/src
                    ${LEDGER_SDK_ROOT}/lib_ux_nbgl
                    ${LEDGER_SDK_ROOT}/target/nanox/include
    COMPILE_DEFS    TARGET_NANOX
                    OS_UNIT_TEST
                    HAVE_NBGL
                    NBGL_STEP
                    NBGL_USE_CASE
                    NBGL_KEYBOARD
                    NBGL_KEYPAD
                    SCREEN_SIZE_NANO
                    BAGL_WIDTH=128
                    BAGL_HEIGHT=64
                    noreturn=
                    __IO=volatile
                    IO_HID_EP_LENGTH=64
                    OS_IO_SEPH_BUFFER_SIZE=272
                    APPNAME="UnitTests"
)

# Nano (non touch) build of the NBGL steps
ledger_unit_tests_add_test(
    NAME            test_nbgl_step_nanos
    NO_TARGET_PROFILE
    SOURCES         ${LEDGER_SDK_ROOT}/lib_nbgl/src/nbgl_step.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${LEDGER_SDK_ROOT}
                    ${LEDGER_SDK_ROOT}/include
                    ${LEDGER_SDK_ROOT}/io/include
                    ${LEDGER_SDK_ROOT}/lib_cxng/include
                    ${LEDGER_SDK_ROOT}/lib_nbgl/include
                    ${LEDGER_SDK_ROOT}/lib_nbgl/src
                    ${LEDGER_SDK_ROOT}/lib_ux_nbgl
                    ${LEDGER_SDK_ROOT}/target/nanox/include
    COMPILE_DEFS    TARGET_NANOX
                    OS_UNIT_TEST
                    HAVE_NBGL
                    NBGL_STEP
                    SCREEN_SIZE_NANO
                    BAGL_WIDTH=128
                    BAGL_HEIGHT=64
                    noreturn=
                    __IO=volatile
                    IO_HID_EP_LENGTH=64
                    OS_IO_SEPH_BUFFER_SIZE=272
                    APPNAME="UnitTests"
)

# Touch (Flex) build of the NBGL pages
ledger_unit_tests_add_test(
    NAME            test_nbgl_page
    SOURCES         ${LEDGER_SDK_ROOT}/lib_nbgl/src/nbgl_page.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    APPNAME="UnitTests"
)

# Touch (Flex) build of the NBGL use cases
ledger_unit_tests_add_test(
    NAME            test_nbgl_use_case
    SOURCES         ${LEDGER_SDK_ROOT}/lib_nbgl/src/nbgl_use_case.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    APPNAME="UnitTests"
)

# Touch (Flex) build of the NBGL keyboard layout
ledger_unit_tests_add_test(
    NAME            test_nbgl_layout_keyboard
    SOURCES         ${LEDGER_SDK_ROOT}/lib_nbgl/src/nbgl_layout_keyboard.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    NBGL_KEYBOARD
                    APPNAME="UnitTests"
)
