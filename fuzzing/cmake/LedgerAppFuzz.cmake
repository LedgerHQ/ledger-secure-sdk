include_guard()

# LedgerAppFuzz.cmake — SDK-native CMake module for Ledger app fuzz builds.

set(LEDGER_FUZZ_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "SDK fuzzing root")

# Optional grammar-aware mutator source apps add to SOURCES to opt in.
set(LEDGER_FUZZ_TLV_MUTATOR_SOURCE "${LEDGER_FUZZ_DIR}/mock/tlv_mutator.c"
    CACHE PATH "TLV grammar-aware mutator source")

# Pinned Absolution release fetched when no local install is provided. Overridable with -D.
set(LEDGER_FUZZ_ABSOLUTION_VERSION "v1.1.2"
    CACHE STRING "Absolution release tag to fetch")

if(NOT EXISTS "${LEDGER_FUZZ_DIR}/include/fuzz_mutator.h")
  message(FATAL_ERROR "SDK fuzz headers not found at ${LEDGER_FUZZ_DIR}/include/")
endif()

# Writes `content` to `path` if it does not exist yet, so Absolution refines it on first build.
function(ledger_fuzz_bootstrap_file path content)
  if(EXISTS "${path}")
    return()
  endif()
  get_filename_component(_dir "${path}" DIRECTORY)
  file(MAKE_DIRECTORY "${_dir}")
  file(WRITE "${path}" "${content}")
  message(STATUS "LedgerFuzz: bootstrapped ${path}")
endfunction()

# Bootstraps fuzz_globals.zon to a minimal stub; mocks.h is app-owned and stays a
# hard requirement. APP_BUILD_PATH points at the app root, where `make
# list-defines` and the glyph glob run.
if(NOT APP_BUILD_PATH)
  get_filename_component(APP_BUILD_PATH "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
  message(STATUS "LedgerFuzz: APP_BUILD_PATH not set, using ${APP_BUILD_PATH}")
endif()

# Collect every directory containing a header under the given roots, so an app's
# internal includes resolve without restating its source layout. Three apps wrote
# this loop; ethereum, bitcoin and solana each had their own copy.
function(ledger_fuzz_collect_include_dirs out_var)
  set(_headers "")
  foreach(_root ${ARGN})
    file(GLOB_RECURSE _found CONFIGURE_DEPENDS "${_root}/*.h")
    list(APPEND _headers ${_found})
  endforeach()
  set(_dirs "")
  foreach(_h ${_headers})
    get_filename_component(_d "${_h}" DIRECTORY)
    list(APPEND _dirs "${_d}")
  endforeach()
  if(_dirs)
    list(REMOVE_DUPLICATES _dirs)
  endif()
  set(${out_var} "${_dirs}" PARENT_SCOPE)
endfunction()

function(ledger_fuzz_validate_app_files)
  set(_fuzz_dir "${CMAKE_SOURCE_DIR}")
  ledger_fuzz_bootstrap_file(
    "${_fuzz_dir}/invariants/fuzz_globals.zon"
    ".{}\n")
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
    # Content-pinned, not just tag-pinned: a GitHub release asset can be replaced
    # under an existing tag, so without a hash two configures of the same source
    # tree can silently get different code generators. Update both together.
    set(_absolution_url_hash
        SHA256=7aafa55856c2c7fa71ba3d0f615cdaced742d6580adb4855f2f6c59017697a03)
    FetchContent_Declare(absolution
      URL https://github.com/Ledger-Donjon/absolution/releases/download/${LEDGER_FUZZ_ABSOLUTION_VERSION}/release-ubuntu-latest-ReleaseFast.zip
      URL_HASH ${_absolution_url_hash}
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

# Mitigations every Absolution-generated fuzzer needs, applied to the exported
# target (set_source_files_properties() is directory-scoped and misses the TU
# created inside absolution_add_fuzzer()). The generated TU overestimates global
# alignment and emits aligned memset/memcpy vector stores that SIGSEGV on
# weaker-aligned real globals, so drop those builtins; its global discovery can
# also pick up libc's stdio externs as weak byte arrays that shadow the real
# streams (libFuzzer Printf() null-derefs), so neutralise <stdio.h> and rename them.
function(ledger_fuzz_harden_target target)
  target_compile_options(${target} PRIVATE "-fno-builtin-memset" "-fno-builtin-memcpy")

  # Absolution's state-prefix size, compiled in as data (see EmitPrefixSize.cmake).
  # A normal build dependency: absolution runs, the file is rewritten only if the
  # number changed, one object recompiles, relink.
  set(_lf_fuzzer_c "${${target}_FUZZER_C}")
  if(NOT _lf_fuzzer_c)
    set(_lf_fuzzer_c "${CMAKE_CURRENT_BINARY_DIR}/_absolution/${target}/fuzzer.c")
  endif()
  set(_lf_prefix_c "${CMAKE_CURRENT_BINARY_DIR}/_absolution/${target}/fuzz_prefix_size.c")
  add_custom_command(
    OUTPUT "${_lf_prefix_c}"
    COMMAND "${CMAKE_COMMAND}"
            -D "FUZZER_C=${_lf_fuzzer_c}"
            -D "OUT=${_lf_prefix_c}"
            -P "${LEDGER_FUZZ_DIR}/cmake/EmitPrefixSize.cmake"
    DEPENDS "${_lf_fuzzer_c}"
    COMMENT "[ledger-fuzz] state-prefix size for ${target}"
    VERBATIM)
  target_sources(${target} PRIVATE "${_lf_prefix_c}")
  # os_explicit_zero_BSS_segment: zeroing BSS would erase the state Absolution
  # just restored. explicit_bzero: MSan cannot see through it, so every
  # SDK-zeroed buffer reads as uninitialised. Both are wrapped rather than
  # shadowed, so neither depends on link order. Bodies in mock/fuzz_runtime.c.
  target_link_options(${target} PRIVATE
    "-Wl,--wrap=os_explicit_zero_BSS_segment"
    "-Wl,--wrap=explicit_bzero"
    # The real PKI gate always fails in a fuzz build (the certificate syscalls
    # are stubbed), which hides everything behind it. mock/pki/ledger_pki_policy.c
    # decides the verdict from the fuzzable fuzz_mock_pki_fail instead.
    "-Wl,--wrap=check_signature_with_pki"
    # Crypto the fuzzer cannot satisfy: real signing/keygen would reject every
    # mocked value early, so these are intercepted and their verdict driven by
    # fuzz_mock_crypto_fail. Bodies in mock/cx/cx_crypto.c.
    "-Wl,--wrap=bip32_derive_with_seed_init_privkey_256"
    "-Wl,--wrap=cx_ecfp_generate_pair_no_throw"
    "-Wl,--wrap=cx_ecdsa_sign_no_throw"
    "-Wl,--wrap=cx_ecschnorr_sign_no_throw")
  target_compile_definitions(${target} PRIVATE
    "_STDIO_H=1"
    "stdin=absltn_libc_stdin"
    "stdout=absltn_libc_stdout"
    "stderr=absltn_libc_stderr"
    "sys_nerr=absltn_libc_sys_nerr"
    "sys_errlist=absltn_libc_sys_errlist")
endfunction()

# Convenience wrapper over absolution_add_fuzzer() for the single-target shape Ledger apps share; auto-resolves SANITIZERS when omitted.
function(ledger_fuzz_add_app_target)
  cmake_parse_arguments(F
    ""
    "NAME;HARNESS;ENTRY;INVARIANT;SANITIZERS"
    "SOURCES;INCLUDE_DIRECTORIES;COMPILE_DEFINITIONS"
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
    TARGETS             ${F_SOURCES}
    HARNESS             ${F_HARNESS}
    ENTRY               ${F_ENTRY}
    INVARIANT           ${F_INVARIANT}
    SANITIZERS          ${F_SANITIZERS}
    INCLUDE_DIRECTORIES ${F_INCLUDE_DIRECTORIES}
    COMPILE_DEFINITIONS ${F_COMPILE_DEFINITIONS}
    LINK_LIBRARIES      secure_sdk
  )

  ledger_fuzz_harden_target(${F_NAME})
endfunction()
