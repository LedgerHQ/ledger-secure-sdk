include_guard()

# LedgerAppFuzz.cmake — SDK-native CMake module for Ledger app fuzz builds.

set(LEDGER_FUZZ_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "SDK fuzzing root")

# Optional grammar-aware mutator source apps add to SOURCES/EXTRA_TARGETS to opt in.
set(LEDGER_FUZZ_TLV_MUTATOR_SOURCE "${LEDGER_FUZZ_DIR}/mock/tlv_mutator.c"
    CACHE PATH "TLV grammar-aware mutator source")

if(NOT EXISTS "${LEDGER_FUZZ_DIR}/include/fuzz_mutator.h")
  message(FATAL_ERROR "SDK fuzz headers not found at ${LEDGER_FUZZ_DIR}/include/")
endif()

# Interface version — apps can set LEDGER_FUZZ_MIN_VERSION to require a minimum.
set(LEDGER_FUZZ_INTERFACE_VERSION 2)
if(DEFINED LEDGER_FUZZ_MIN_VERSION)
  if(LEDGER_FUZZ_INTERFACE_VERSION LESS LEDGER_FUZZ_MIN_VERSION)
    message(FATAL_ERROR
      "SDK fuzz interface version ${LEDGER_FUZZ_INTERFACE_VERSION} "
      "is older than required ${LEDGER_FUZZ_MIN_VERSION}. Update the SDK.")
  endif()
endif()

# Bootstraps a contract file (from template or literal content) that Absolution refines on first build.
function(ledger_fuzz_bootstrap_file path template_path content)
  if(EXISTS "${path}")
    return()
  endif()
  get_filename_component(_dir "${path}" DIRECTORY)
  file(MAKE_DIRECTORY "${_dir}")
  if(template_path AND EXISTS "${template_path}")
    configure_file("${template_path}" "${path}" COPYONLY)
  else()
    file(WRITE "${path}" "${content}")
  endif()
  message(STATUS "LedgerFuzz: bootstrapped ${path}")
endfunction()

# Bootstraps fuzz_globals.zon/scenario_layout.h from the SDK template; mocks.h is app-owned and stays a hard requirement.
function(ledger_fuzz_validate_app_files)
  set(_fuzz_dir "${CMAKE_SOURCE_DIR}")
  ledger_fuzz_bootstrap_file(
    "${_fuzz_dir}/invariants/fuzz_globals.zon"
    "${BOLOS_SDK}/fuzzing/template/invariants/fuzz_globals.zon"
    ".{}\n")
  ledger_fuzz_bootstrap_file(
    "${_fuzz_dir}/mock/scenario_layout.h"
    "${BOLOS_SDK}/fuzzing/template/mock/scenario_layout.h"
    "")
  if(NOT EXISTS "${_fuzz_dir}/mock/mocks.h")
    message(FATAL_ERROR
      "Missing: ${_fuzz_dir}/mock/mocks.h\n"
      "Hint: copy from ${BOLOS_SDK}/fuzzing/template/mock/mocks.h")
  endif()
  message(STATUS "LedgerFuzz: app files validated in ${_fuzz_dir}")
endfunction()

ledger_fuzz_validate_app_files()

# Fetches the pinned Absolution release by default; set LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR (var or env) to a local install to skip the download.
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

# Call once after project() to pull in the SDK fuzz subtree and Absolution.
macro(ledger_fuzz_setup)
  _ledger_fuzz_resolve_absolution()
  add_subdirectory(
    ${LEDGER_FUZZ_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/ledger-secure-sdk
    EXCLUDE_FROM_ALL)
  find_package(Absolution REQUIRED CONFIG)
endmacro()

# Resolves the -fsanitize= set: mirror CFL's CFLAGS/LIB_FUZZING_ENGINE, else the SANITIZER var, else fuzzer,address — mirroring stops clang rejecting conflicting sanitizer combos.
function(_ledger_fuzz_resolve_sanitizers out_var)
  set(_san "")
  if(DEFINED ENV{LIB_FUZZING_ENGINE})
    set(_cflags "$ENV{CFLAGS} $ENV{CXXFLAGS}")
    if(_cflags MATCHES "-fsanitize=([a-zA-Z0-9_,-]+)")
      set(_san_raw "${CMAKE_MATCH_1}")
      string(REPLACE "," ";" _san_list "${_san_raw}")
      list(REMOVE_ITEM _san_list "fuzzer" "fuzzer-no-link")
      list(JOIN _san_list "," _san_clean)
      if(_san_clean)
        set(_san "fuzzer,${_san_clean}")
      endif()
    endif()
    if(NOT _san)
      set(_san "fuzzer")
    endif()
  elseif(DEFINED SANITIZER AND NOT SANITIZER STREQUAL "")
    if(SANITIZER STREQUAL "coverage")
      set(_san "fuzzer,address")
    else()
      set(_san "fuzzer,${SANITIZER}")
    endif()
  else()
    set(_san "fuzzer,address")
  endif()
  set(${out_var} "${_san}" PARENT_SCOPE)
endfunction()

# Cache the resolved default once so every call site shares it; per-target overrides still pass SANITIZERS explicitly.
_ledger_fuzz_resolve_sanitizers(_LEDGER_FUZZ_DEFAULT_SANITIZERS)
set(LEDGER_FUZZ_DEFAULT_SANITIZERS "${_LEDGER_FUZZ_DEFAULT_SANITIZERS}"
    CACHE STRING "Sanitizer set passed to absolution_add_fuzzer() by default")
unset(_LEDGER_FUZZ_DEFAULT_SANITIZERS)

# Convenience wrapper over absolution_add_fuzzer() for the single-target shape Ledger apps share; auto-resolves SANITIZERS when omitted.
function(ledger_fuzz_add_app_target)
  cmake_parse_arguments(F
    ""
    "NAME;HARNESS;ENTRY;INVARIANT;SANITIZERS"
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
  if(NOT F_SANITIZERS)
    set(F_SANITIZERS "${LEDGER_FUZZ_DEFAULT_SANITIZERS}")
  endif()
  message(STATUS "LedgerFuzz: ${F_NAME} using -fsanitize=${F_SANITIZERS}")

  absolution_add_fuzzer(
    NAME                ${F_NAME}
    TARGETS             ${F_SOURCES} ${F_EXTRA_TARGETS}
    HARNESS             ${F_HARNESS}
    ENTRY               ${F_ENTRY}
    INVARIANT           ${F_INVARIANT}
    SANITIZERS          ${F_SANITIZERS}
    INCLUDE_DIRECTORIES ${F_INCLUDE_DIRECTORIES}
    COMPILE_DEFINITIONS ${F_COMPILE_DEFINITIONS}
    LINK_LIBRARIES      secure_sdk
  )

  # Absolution's generated TU can overestimate global alignment and emit aligned memset/memcpy vector stores that SIGSEGV on weaker-aligned real globals, so disable those builtins.
  set_source_files_properties(
    "${CMAKE_CURRENT_BINARY_DIR}/_absolution/${F_NAME}/fuzzer.c"
    PROPERTIES COMPILE_OPTIONS "-fno-builtin-memset;-fno-builtin-memcpy")
endfunction()
