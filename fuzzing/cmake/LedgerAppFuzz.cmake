include_guard()

# LedgerAppFuzz.cmake — SDK-native CMake module for Ledger app fuzz builds.
#
# Include from your app's fuzzing/CMakeLists.txt:
#   include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)
#
# See ${BOLOS_SDK}/fuzzing/docs/APP_CONTRACT.md for the full contract.

set(LEDGER_FUZZ_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "SDK fuzzing root")

# Optional mutator sources — apps add these to their target's SOURCES /
# EXTRA_TARGETS to opt into a grammar-aware mutator.
set(LEDGER_FUZZ_TLV_MUTATOR_SOURCE "${LEDGER_FUZZ_DIR}/mock/tlv_mutator.c"
    CACHE PATH "TLV grammar-aware mutator source")

# Header validation
if(NOT EXISTS "${BOLOS_SDK}/fuzzing/include/fuzz_mutator.h")
  message(FATAL_ERROR "SDK fuzz headers not found at ${BOLOS_SDK}/fuzzing/include/")
endif()

# Interface version — apps can set LEDGER_FUZZ_MIN_VERSION to require a minimum.
# v2: Absolution is auto-fetched from GitHub releases; ABSOLUTION_DIR / sibling
#     checkout / find_package(Absolution) at app-side are no longer required.
set(LEDGER_FUZZ_INTERFACE_VERSION 2)
if(DEFINED LEDGER_FUZZ_MIN_VERSION)
  if(LEDGER_FUZZ_INTERFACE_VERSION LESS LEDGER_FUZZ_MIN_VERSION)
    message(FATAL_ERROR
      "SDK fuzz interface version ${LEDGER_FUZZ_INTERFACE_VERSION} "
      "is older than required ${LEDGER_FUZZ_MIN_VERSION}. Update the SDK.")
  endif()
endif()

# App file validation
function(ledger_fuzz_validate_app_files)
  set(_fuzz_dir "${CMAKE_SOURCE_DIR}")
  if(NOT EXISTS "${_fuzz_dir}/invariants/fuzz_globals.zon")
    message(FATAL_ERROR
      "Missing: ${_fuzz_dir}/invariants/fuzz_globals.zon\n"
      "Hint: start with .{} for new apps. See ${BOLOS_SDK}/fuzzing/docs/APP_CONTRACT.md")
  endif()
  if(NOT EXISTS "${_fuzz_dir}/mock/scenario_layout.h")
    message(FATAL_ERROR
      "Missing: ${_fuzz_dir}/mock/scenario_layout.h\n"
      "Hint: copy from ${BOLOS_SDK}/fuzzing/template/mock/scenario_layout.h")
  endif()
  if(NOT EXISTS "${_fuzz_dir}/mock/mocks.h")
    message(FATAL_ERROR
      "Missing: ${_fuzz_dir}/mock/mocks.h\n"
      "Hint: copy from ${BOLOS_SDK}/fuzzing/template/mock/mocks.h")
  endif()
  message(STATUS "LedgerFuzz: app files validated in ${_fuzz_dir}")
endfunction()

ledger_fuzz_validate_app_files()

# ── Absolution dependency ────────────────────────────────────────────────────
# By default, fetch the pinned Absolution Linux release zip from GitHub at
# configure time. To skip the download (offline / unreleased Absolution), set
# either the CMake variable or env var LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR to a
# directory that contains bin/absolution and lib/cmake/Absolution/.
function(_ledger_fuzz_resolve_absolution)
  set(_local "${LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR}")
  if(NOT _local AND DEFINED ENV{LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR})
    set(_local "$ENV{LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR}")
  endif()

  if(_local)
    if(NOT EXISTS "${_local}/bin/absolution"
       OR NOT EXISTS "${_local}/lib/cmake/Absolution")
      message(FATAL_ERROR
        "LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR=${_local} is not a valid Absolution "
        "install (expected bin/absolution and lib/cmake/Absolution/).")
    endif()
    set(_root "${_local}")
    message(STATUS "LedgerFuzz: using local Absolution at ${_root}")
  else()
    include(FetchContent)
    set(LEDGER_FUZZ_ABSOLUTION_VERSION "v1.1.0")
    FetchContent_Declare(absolution
      URL https://github.com/Ledger-Donjon/absolution/releases/download/${LEDGER_FUZZ_ABSOLUTION_VERSION}/release-ubuntu-latest-ReleaseFast.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(absolution)
    set(_root "${absolution_SOURCE_DIR}")
    message(STATUS "LedgerFuzz: fetched Absolution ${LEDGER_FUZZ_ABSOLUTION_VERSION} into ${_root}")
  endif()

  set(Absolution_DIR "${_root}/lib/cmake/Absolution"
      CACHE PATH "Absolution CMake package directory" FORCE)
  set(ABSOLUTION_EXECUTABLE "${_root}/bin/absolution"
      CACHE FILEPATH "Absolution code generator binary" FORCE)

  list(APPEND CMAKE_BUILD_RPATH   "${_root}/lib")
  list(APPEND CMAKE_INSTALL_RPATH "${_root}/lib")
  set(CMAKE_BUILD_RPATH   "${CMAKE_BUILD_RPATH}"   PARENT_SCOPE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}" PARENT_SCOPE)
endfunction()

# ── Setup helper ─────────────────────────────────────────────────────────────
# Call once after project() to pull in the SDK fuzz subtree and Absolution.
#
#   project(MyFuzzer ...)
#   include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)
#   ledger_fuzz_setup()
#
macro(ledger_fuzz_setup)
  _ledger_fuzz_resolve_absolution()
  add_subdirectory(
    ${BOLOS_SDK}/fuzzing
    ${CMAKE_CURRENT_BINARY_DIR}/ledger-secure-sdk
    EXCLUDE_FROM_ALL)
  find_package(Absolution REQUIRED CONFIG)
endmacro()

# ── App-target helper ────────────────────────────────────────────────────────
# Convenience wrapper over absolution_add_fuzzer() for the common single-target
# shape that all Ledger apps share.  Apps pass sources / includes / defines as
# data; the skeleton is identical.
#
#   ledger_fuzz_add_app_target(
#     SOURCES             ${C_SOURCES}
#     INCLUDE_DIRECTORIES ${_inc_dirs}
#     COMPILE_DEFINITIONS HAVE_SWAP APPNAME="Foo"
#     [EXTRA_TARGETS      ${LEDGER_FUZZ_TLV_MUTATOR_SOURCE}]
#     [NAME               fuzz_globals]
#     [HARNESS            ${CMAKE_SOURCE_DIR}/harness/fuzz_dispatcher.c]
#     [ENTRY              fuzz_entry]
#     [INVARIANT          ${CMAKE_SOURCE_DIR}/invariants/fuzz_globals.zon]
#   )
#
function(ledger_fuzz_add_app_target)
  cmake_parse_arguments(F
    ""
    "NAME;HARNESS;ENTRY;INVARIANT"
    "SOURCES;INCLUDE_DIRECTORIES;COMPILE_DEFINITIONS;EXTRA_TARGETS"
    ${ARGN})

  if(NOT F_NAME)
    set(F_NAME fuzz_globals)
  endif()
  if(NOT F_HARNESS)
    set(F_HARNESS "${CMAKE_SOURCE_DIR}/harness/fuzz_dispatcher.c")
  endif()
  if(NOT F_ENTRY)
    set(F_ENTRY fuzz_entry)
  endif()
  if(NOT F_INVARIANT)
    set(F_INVARIANT "${CMAKE_SOURCE_DIR}/invariants/fuzz_globals.zon")
  endif()

  absolution_add_fuzzer(
    NAME                ${F_NAME}
    TARGETS             ${F_SOURCES} ${F_EXTRA_TARGETS}
    HARNESS             ${F_HARNESS}
    ENTRY               ${F_ENTRY}
    INVARIANT           ${F_INVARIANT}
    INCLUDE_DIRECTORIES ${F_INCLUDE_DIRECTORIES}
    COMPILE_DEFINITIONS ${F_COMPILE_DEFINITIONS}
    LINK_LIBRARIES      secure_sdk
  )
endfunction()
