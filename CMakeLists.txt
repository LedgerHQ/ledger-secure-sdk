cmake_minimum_required(VERSION 3.28)

project(ledger-secure-sdk
        LANGUAGES C ASM
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LEDGER_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Ledger Secure SDK root")

include(${LEDGER_SDK_ROOT}/cmake/target_profile.cmake)
include(${LEDGER_SDK_ROOT}/cmake/compile_options.cmake)

set(LEDGER_GLYPHS_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib_nbgl/generated
    CACHE INTERNAL "Generated NBGL glyphs dir")

include(${LEDGER_SDK_ROOT}/cmake/sdk_headers.cmake)

if(BUILD_UNIT_TESTS)
    include(${LEDGER_SDK_ROOT}/cmake/LedgerUT.cmake)
    if(PROJECT_IS_TOP_LEVEL)
        ledger_unit_tests_init()
    endif()
endif()

include(${LEDGER_SDK_ROOT}/cmake/sdk_components.cmake)
