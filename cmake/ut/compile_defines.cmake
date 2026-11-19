# No preset (e.g. app-ethereum's standalone LedgerUT): keep the legacy defines.
if(NOT DEFINED LEDGER_TARGET_DEFS)
    include(${CMAKE_CURRENT_LIST_DIR}/legacy_compile_defines.cmake)
    return()
endif()

set(UT_COMPILE_DEFS_SDK
    OS_UNIT_TEST
    noreturn=
    DEPRECATED=
    CACHE INTERNAL "Compile defines for unit tests"
)
