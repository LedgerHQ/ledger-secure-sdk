include_guard()
cmake_minimum_required(VERSION 3.14)

# project information
project(
  SDKFuzzer
  VERSION 1.0
  DESCRIPTION "SDK Fuzzer"
  LANGUAGES C)

# guard against bad build-type strings
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()

# Check for BOLOS_SDK
if(NOT BOLOS_SDK)
  message(WARNING "BOLOS_SDK not passed.")
endif()

# Check for .target in BOLOS_SDK
if(EXISTS "${BOLOS_SDK}/.target")
  file(STRINGS "${BOLOS_SDK}/.target" TARGET)
  string(STRIP "${TARGET}" TARGET)
  if(NOT "${TARGET}" STREQUAL "stax" AND NOT "${TARGET}" STREQUAL "flex")
    message(FATAL_ERROR ".target must be 'flex' or 'stax' (found '${TARGET}')!")
  else()
    message("Using TARGET=${TARGET} from ${BOLOS_SDK}/.target")
  endif()
  # If no .target, checks for -DTARGET=
else()
  message(WARNING "No .target file found in BOLOS_SDK=${BOLOS_SDK}")
  if(TARGET)
    message("Using -DTARGET=${TARGET}")
  else()
    message(FATAL_ERROR " No TARGET passed.")
  endif()
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. "
  )
endif()

option(FUZZ_ENABLE_SOURCE_COVERAGE
       "Enable LLVM source coverage instrumentation for replay reporting."
       ON)

# compatible with ClusterFuzzLite
if(NOT DEFINED ENV{LIB_FUZZING_ENGINE})
  set(COMPILATION_FLAGS
      -fno-common
      -std=gnu99
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

  set(LINK_FLAGS -fuse-ld=lld)
  if(FUZZ_ENABLE_SOURCE_COVERAGE)
    list(APPEND COMPILATION_FLAGS -fprofile-instr-generate -fcoverage-mapping)
    list(APPEND LINK_FLAGS -fprofile-instr-generate -fcoverage-mapping)
  endif()
  # Compile-time carve-outs for benign UBSan hits (vtable polymorphism,
  # crypto wrapping arithmetic, allocator sign-change, etc.).
  set(_FUZZ_IGNORELIST
      "-fsanitize-ignorelist=${BOLOS_SDK}/fuzzing/v2/sanitizers/ubsan-ignorelist.txt")

  if(SANITIZER MATCHES "address")
    # Full sanitizer stack for memory-corruption hunting on BOLOS-targeted code.
    # Groups: ASan + the complete UBSan 'undefined' set + 'integer' (catches
    # implicit truncation/sign-change in length math) + 'nullability' (proven
    # valuable: memcpy(NULL,0)) + 'local-bounds' (stack OOB, not in 'undefined')
    # + 'float-divide-by-zero' (not in 'undefined'; no HW FPU on device).
    #
    # -fno-sanitize=alignment: the BOLOS allocator (lib_alloc) returns 4-byte
    # aligned memory; structs with pointers need 8.  Cortex-M33 handles
    # unaligned access natively, so this is safe on the target -- not a
    # security concern.  Keeping it enabled drowns real findings in 100k+
    # noise per campaign.
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
  elseif(SANITIZER MATCHES "memory")
    set(COMPILATION_FLAGS
        ${COMPILATION_FLAGS} -fsanitize=fuzzer,memory,undefined
        -fno-sanitize=alignment
        -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link
        ${_FUZZ_IGNORELIST})
    set(LINK_FLAGS ${LINK_FLAGS} -fsanitize=fuzzer,memory,undefined
                   -fno-sanitize=alignment
                   -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link
                   ${_FUZZ_IGNORELIST})
  else()
    message(
      FATAL_ERROR
        "Unknown sanitizer type. It must be set to `address` or `memory`.")
  endif()
else()
  execute_process(COMMAND clang --version OUTPUT_VARIABLE CLANG_VERSION)
  message(STATUS "Clang version: ${CLANG_VERSION}")

  set(COMPILATION_FLAGS "$ENV{CFLAGS}")
  set(LINK_FLAGS "$ENV{CFLAGS} $ENV{LIB_FUZZING_ENGINE}")
  separate_arguments(COMPILATION_FLAGS)
  separate_arguments(LINK_FLAGS)
  list(APPEND COMPILATION_FLAGS
      "-fsanitize-ignorelist=${BOLOS_SDK}/fuzzing/v2/sanitizers/ubsan-ignorelist.txt")
  list(APPEND LINK_FLAGS
      "-fsanitize-ignorelist=${BOLOS_SDK}/fuzzing/v2/sanitizers/ubsan-ignorelist.txt")
  if(FUZZ_ENABLE_SOURCE_COVERAGE)
    list(APPEND COMPILATION_FLAGS -fprofile-instr-generate -fcoverage-mapping)
    list(APPEND LINK_FLAGS -fprofile-instr-generate -fcoverage-mapping)
  endif()
  set(LINK_FLAGS ${LINK_FLAGS} -fuse-ld=lld)
endif()

# When configured from outside fuzzing/v2/ (an app or sdk-fuzz via the
# public LedgerAppFuzz.cmake entry point), expose a ledger_fuzz_secure_sdk
# INTERFACE target aggregating every SDK library, the mock layer, macro
# definitions, and the framework include path. Apps add
# `target_link_libraries(<my_fuzzer> PRIVATE ledger_fuzz_secure_sdk)` (or rely
# on `ledger_fuzz_add_app_target()` to do it) and get the full SDK fuzz
# environment without touching per-lib CMake modules.
if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  foreach(_lib alloc cxng glyphs io lists nbgl nfc pki qrcode standard_app tlv)
    include("${BOLOS_SDK}/fuzzing/v2/libs/lib_${_lib}.cmake")
  endforeach()

  add_library(ledger_fuzz_secure_sdk INTERFACE)
  target_compile_options(ledger_fuzz_secure_sdk INTERFACE ${COMPILATION_FLAGS})
  target_link_options(ledger_fuzz_secure_sdk INTERFACE ${LINK_FLAGS})
  target_link_libraries(
    ledger_fuzz_secure_sdk
    INTERFACE ledger_fuzz_macros
              ledger_fuzz_alloc
              ledger_fuzz_lists
              ledger_fuzz_mock
              ledger_fuzz_cxng
              ledger_fuzz_io
              ledger_fuzz_nbgl
              ledger_fuzz_nfc
              ledger_fuzz_pki
              ledger_fuzz_tlv
              ledger_fuzz_qrcode
              ledger_fuzz_standard_app)

  target_include_directories(ledger_fuzz_secure_sdk INTERFACE "${BOLOS_SDK}/fuzzing/v2/include")
endif()
