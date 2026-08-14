include_guard()
cmake_minimum_required(VERSION 3.20)

project(
  SDKFuzzer
  VERSION 1.0
  DESCRIPTION "SDK Fuzzer"
  LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()

if(NOT BOLOS_SDK)
  message(WARNING "BOLOS_SDK not passed.")
endif()

# Precedence matches the production Makefile (Makefile.target): an explicitly
# requested target wins, ${BOLOS_SDK}/.target is only the fallback.
if(TARGET)
  message(STATUS "LedgerFuzz: using requested TARGET=${TARGET}")
elseif(EXISTS "${BOLOS_SDK}/.target")
  file(STRINGS "${BOLOS_SDK}/.target" TARGET)
  string(STRIP "${TARGET}" TARGET)
  message(STATUS "LedgerFuzz: using TARGET=${TARGET} from ${BOLOS_SDK}/.target")
else()
  message(FATAL_ERROR "No TARGET passed and no ${BOLOS_SDK}/.target to fall back on.")
endif()
if(NOT "${TARGET}" STREQUAL "stax" AND NOT "${TARGET}" STREQUAL "flex")
  message(FATAL_ERROR "TARGET must be 'flex' or 'stax' (got '${TARGET}')")
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. "
  )
endif()

option(FUZZ_ENABLE_SOURCE_COVERAGE
       "Enable LLVM source coverage instrumentation for replay reporting."
       ON)

# ABI/warning flags applied in every environment: CFL supplies sanitizer flags via CFLAGS but not target ABI flags like -fshort-enums, whose absence changes enum sizes and yields sanitizer-only false positives.
set(_FUZZ_BASE_COMPILATION_FLAGS
    -fno-common
    -std=gnu99
    -DFUZZING
    -Wall
    -Wextra
    -Wno-main
    -Wno-error=int-conversion
    -Wimplicit-fallthrough
    -Wvla
    -Wundef
    -Wshadow
    -Wformat=2
    -Wformat-security
    -Wwrite-strings
    -ffunction-sections
    -fdata-sections
    -funsigned-char
    -fshort-enums)

if(NOT DEFINED ENV{LIB_FUZZING_ENGINE})
  set(COMPILATION_FLAGS ${_FUZZ_BASE_COMPILATION_FLAGS})

  set(LINK_FLAGS -fuse-ld=lld)
  if(FUZZ_ENABLE_SOURCE_COVERAGE)
    list(APPEND COMPILATION_FLAGS -fprofile-instr-generate -fcoverage-mapping)
    list(APPEND LINK_FLAGS -fprofile-instr-generate -fcoverage-mapping)
  endif()
  # Default so a plain `cmake -S <app>/fuzzing` works without the campaign
  # scripts, which set SANITIZER themselves.
  if(NOT SANITIZER)
    set(SANITIZER "address")
    message(STATUS "LedgerFuzz: SANITIZER not set, defaulting to 'address'")
  endif()
  if(NOT SANITIZER STREQUAL "address"
     AND NOT SANITIZER STREQUAL "memory"
     AND NOT SANITIZER STREQUAL "undefined")
    message(FATAL_ERROR
      "SANITIZER must be exactly 'address', 'memory' or 'undefined' (got '${SANITIZER}')")
  endif()

  # Ignorelist carves out benign UBSan hits (vtable polymorphism, crypto wrapping
  # arithmetic, allocator sign-change). An app adds its own with
  # -DLEDGER_FUZZ_APP_UBSAN_IGNORELIST=<file>; clang accepts the flag repeatedly.
  set(_FUZZ_IGNORELIST
      "-fsanitize-ignorelist=${BOLOS_SDK}/fuzzing/sanitizers/ubsan-ignorelist.txt")
  if(LEDGER_FUZZ_APP_UBSAN_IGNORELIST)
    if(NOT EXISTS "${LEDGER_FUZZ_APP_UBSAN_IGNORELIST}")
      message(FATAL_ERROR
        "LEDGER_FUZZ_APP_UBSAN_IGNORELIST does not exist: ${LEDGER_FUZZ_APP_UBSAN_IGNORELIST}")
    endif()
    list(APPEND _FUZZ_IGNORELIST
         "-fsanitize-ignorelist=${LEDGER_FUZZ_APP_UBSAN_IGNORELIST}")
    message(STATUS "LedgerFuzz: app UBSan ignorelist ${LEDGER_FUZZ_APP_UBSAN_IGNORELIST}")
  endif()

  if(SANITIZER STREQUAL "address")
    # -fno-sanitize=alignment: lib_alloc returns 4-byte-aligned memory and Cortex-M33 allows unaligned access, so alignment reports are on-device-safe noise.
    set(_FUZZ_SAN
        fuzzer,address,undefined,integer,nullability,local-bounds,float-divide-by-zero)
    list(APPEND COMPILATION_FLAGS
        -fsanitize=${_FUZZ_SAN}
        -fno-sanitize=alignment
        -fsanitize-recover=unsigned-integer-overflow
        ${_FUZZ_IGNORELIST})
    list(APPEND LINK_FLAGS
        -fsanitize=${_FUZZ_SAN}
        -fno-sanitize=alignment
        -fsanitize-recover=unsigned-integer-overflow
        ${_FUZZ_IGNORELIST})
  elseif(SANITIZER STREQUAL "memory")
    set(COMPILATION_FLAGS
        ${COMPILATION_FLAGS} -fsanitize=fuzzer,memory,undefined
        -fno-sanitize=alignment
        -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link
        ${_FUZZ_IGNORELIST})
    set(LINK_FLAGS ${LINK_FLAGS} -fsanitize=fuzzer,memory,undefined
                   -fno-sanitize=alignment
                   -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link
                   ${_FUZZ_IGNORELIST})
  elseif(SANITIZER STREQUAL "undefined")
    # Mirror CFL's standalone UBSan job for local reproduction without ASan/MSan.
    set(_FUZZ_SAN fuzzer,undefined,unsigned-integer-overflow)
    list(APPEND COMPILATION_FLAGS
        -fsanitize=${_FUZZ_SAN}
        -fno-sanitize=alignment
        -fsanitize-recover=unsigned-integer-overflow
        ${_FUZZ_IGNORELIST})
    list(APPEND LINK_FLAGS
        -fsanitize=${_FUZZ_SAN}
        -fno-sanitize=alignment
        -fsanitize-recover=unsigned-integer-overflow
        ${_FUZZ_IGNORELIST})
  else()
    message(
      FATAL_ERROR
        "Unknown sanitizer type. It must be set to `address`, `memory`, or `undefined`.")
  endif()
else()
  execute_process(COMMAND clang --version OUTPUT_VARIABLE CLANG_VERSION)
  message(STATUS "Clang version: ${CLANG_VERSION}")

  set(COMPILATION_FLAGS "$ENV{CFLAGS}")
  set(LINK_FLAGS "$ENV{CFLAGS} $ENV{LIB_FUZZING_ENGINE}")
  separate_arguments(COMPILATION_FLAGS)
  separate_arguments(LINK_FLAGS)
  set(COMPILATION_FLAGS ${_FUZZ_BASE_COMPILATION_FLAGS} ${COMPILATION_FLAGS})
  list(APPEND COMPILATION_FLAGS
      "-fsanitize-ignorelist=${BOLOS_SDK}/fuzzing/sanitizers/ubsan-ignorelist.txt")
  list(APPEND LINK_FLAGS
      "-fsanitize-ignorelist=${BOLOS_SDK}/fuzzing/sanitizers/ubsan-ignorelist.txt")
  if(FUZZ_ENABLE_SOURCE_COVERAGE)
    list(APPEND COMPILATION_FLAGS -fprofile-instr-generate -fcoverage-mapping)
    list(APPEND LINK_FLAGS -fprofile-instr-generate -fcoverage-mapping)
  endif()
  set(LINK_FLAGS ${LINK_FLAGS} -fuse-ld=lld)
endif()

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  message(FATAL_ERROR
    "fuzzing/ is not a standalone build entry point. Build the SDK self-fuzz "
    "targets via fuzzing/sdk-fuzz, or an app via its own fuzzing/ directory.")
endif()

# Expose fuzzing/include/ to all intermediate static libraries (standard_app,
# tlv, etc.) so that our string.h wrapper is on the path when they compile.
include_directories("${BOLOS_SDK}/fuzzing/include")

# Each lib_*.cmake aggregator pulls in mock.cmake (which defines `mock`).
foreach(_lib alloc cxng glyphs io lists nbgl nfc pki qrcode standard_app tlv)
  include("${BOLOS_SDK}/fuzzing/libs/lib_${_lib}.cmake")
endforeach()

add_library(secure_sdk INTERFACE)
target_compile_options(secure_sdk INTERFACE ${COMPILATION_FLAGS})
# No --allow-multiple-definition here: every mock that must beat a real SDK
# definition goes through -Wl,--wrap (see ledger_fuzz_harden_target), which is
# order-independent. Absolution's own module still passes the flag, so duplicates
# are not a hard error yet.
target_link_options(secure_sdk INTERFACE ${LINK_FLAGS})
target_link_libraries(
  secure_sdk
  INTERFACE macros
            alloc
            lists
            mock
            cxng
            io
            nbgl
            nfc
            pki
            tlv
            qrcode
            standard_app)

target_include_directories(secure_sdk INTERFACE "${BOLOS_SDK}/fuzzing/include")
