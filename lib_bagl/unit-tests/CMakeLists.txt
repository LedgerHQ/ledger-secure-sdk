project(unit_tests_lib_bagl LANGUAGES C)

# LedgerUT and ledger_unit_tests_init() live in the root CMakeLists.txt.

ledger_unit_tests_add_test(
    NAME            test_bagl_animate
    SOURCES         ${LEDGER_SDK_ROOT}/lib_bagl/src/bagl_animate.c
    INCLUDE_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${LEDGER_SDK_ROOT}/lib_bagl/include
                    ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS    ${UT_COMPILE_DEFS_SDK}
                    BAGL_WIDTH=128
                    BAGL_HEIGHT=64
    COMPILE_OPTIONS -fsanitize=address,undefined -fno-sanitize-recover=all
    LINK_OPTIONS    -fsanitize=address,undefined
)
