include_guard()
cmake_minimum_required(VERSION 3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.14)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

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
    message(
      FATAL_ERROR
        " No TARGET passed."
      )
  endif()
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. "
  )
endif()

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
      -fprofile-instr-generate
      -fcoverage-mapping
      -funsigned-char
      -fshort-enums)

  set(LINK_FLAGS -fprofile-instr-generate -fcoverage-mapping -fuse-ld=lld)
  if(SANITIZER MATCHES "address")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS}
                          -fsanitize=fuzzer,address,undefined)
    set(LINK_FLAGS ${LINK_FLAGS} -fsanitize=fuzzer,address,undefined)
  elseif(SANITIZER MATCHES "memory")
    set(COMPILATION_FLAGS
        ${COMPILATION_FLAGS} -fsanitize=fuzzer,memory,undefined
        -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link)
    set(LINK_FLAGS ${LINK_FLAGS} -fsanitize=fuzzer,memory,undefined
                   -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link)
  else()
    message(
      FATAL_ERROR
        "Unknown sanitizer type. It must be set to `address` or `memory`.")
  endif()
else()
  execute_process(COMMAND clang --version OUTPUT_VARIABLE CLANG_VERSION)
  message(STATUS "Clang version: ${CLANG_VERSION}")

  execute_process(COMMAND ldd --version OUTPUT_VARIABLE LDD_VERSION)
  set(COMPILATION_FLAGS "$ENV{LIB_FUZZING_ENGINE} $ENV{CFLAGS}")
  set(LINK_FLAGS "$ENV{LIB_FUZZING_ENGINE} $ENV{CFLAGS}")
  separate_arguments(COMPILATION_FLAGS)
  separate_arguments(LINK_FLAGS)
endif()

# Fuzzing an APP
if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  file(GLOB LIBS "${BOLOS_SDK}/fuzzing/libs/*.cmake")

  foreach(file IN LISTS LIBS)
    include(${file})
  endforeach()

  add_library(secure_sdk INTERFACE)
  target_compile_options(secure_sdk INTERFACE ${COMPILATION_FLAGS})
  target_link_options(secure_sdk INTERFACE ${LINK_FLAGS})
  target_link_libraries(
    secure_sdk
    INTERFACE macros
              alloc
              cxng
              io
              nbgl
              nfc
              qrcode
              standard_app
              mock)

  # Fuzzing the SDK
else()
  set(BOLOS_SDK ${CMAKE_SOURCE_DIR})
  get_filename_component(BOLOS_SDK ${CMAKE_SOURCE_DIR} DIRECTORY)
  # Include harness cmake files
  file(GLOB cmake_extra_files "extra/*.cmake")
  foreach(file IN LISTS cmake_extra_files)
    include(${file})
  endforeach()

endif()
