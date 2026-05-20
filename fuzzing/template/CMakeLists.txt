include_guard()
cmake_minimum_required(VERSION 3.14)

project(
  MyAppFuzzer
  VERSION 1.0
  DESCRIPTION "Ledger App Fuzzer"
  LANGUAGES C)

if(NOT DEFINED BOLOS_SDK)
  message(FATAL_ERROR "BOLOS_SDK must be defined")
endif()

include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)
ledger_fuzz_setup()

set(APP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)

file(GLOB_RECURSE C_SOURCES
  "${APP_SOURCE_DIR}/src/*.c"
  "${CMAKE_SOURCE_DIR}/mock/*.c"
)
list(REMOVE_ITEM C_SOURCES "${APP_SOURCE_DIR}/src/app_main.c")

ledger_fuzz_add_app_target(
    SOURCES ${C_SOURCES}
    INCLUDE_DIRECTORIES
        ${APP_SOURCE_DIR}/src/
        ${CMAKE_SOURCE_DIR}/mock/
        ${CMAKE_SOURCE_DIR}
    COMPILE_DEFINITIONS
        # Add app-specific defines here (e.g. HAVE_SWAP).
)
