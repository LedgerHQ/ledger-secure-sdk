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

# default fuzz device target
if(NOT TARGET_DEVICE)
  set(TARGET_DEVICE "flex")
endif()
message("TARGET_DEVICE = ${TARGET_DEVICE}")

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
  
    set(LINK_FLAGS -ffunction-sections -fdata-sections -fprofile-instr-generate -fcoverage-mapping)
  if(SANITIZER MATCHES "address")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS}
                          -fsanitize=fuzzer,address,undefined)
    set(LINK_FLAGS ${LINK_FLAGS} -fsanitize=fuzzer,address,undefined)
  elseif(SANITIZER MATCHES "memory")
    set(COMPILATION_FLAGS
        ${COMPILATION_FLAGS} -fsanitize=fuzzer,memory,undefined
        -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link)
    set(LINK_FLAGS
        ${LINK_FLAGS} -fsanitize=fuzzer,memory,undefined
        -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link)
  else()
    message(
      FATAL_ERROR
        "Unknown sanitizer type. It must be set to `address` or `memory`.")
  endif()
else()
  set(COMPILATION_FLAGS "$ENV{LIB_FUZZING_ENGINE} $ENV{CFLAGS}")
  set(LINK_FLAGS "$ENV{LIB_FUZZING_ENGINE} $ENV{LDFLAGS}")
  separate_arguments(COMPILATION_FLAGS)
  separate_arguments(LINK_FLAGS)
endif()

if(NOT DEFINED BOLOS_SDK)
  set(BOLOS_SDK ${CMAKE_SOURCE_DIR})
  get_filename_component(BOLOS_SDK ${CMAKE_SOURCE_DIR} DIRECTORY)
endif()

# Fuzzing an APP
if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  file(GLOB LIBS "${BOLOS_SDK}/fuzzing/libs/*.cmake")

  foreach(file IN LISTS LIBS)
    include(${file})
  endforeach()

  add_library(extra INTERFACE)
  target_link_libraries(
    extra
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
  include("libs/lib_io.cmake")
  # Include harness cmake files
  file(GLOB cmake_extra_files "extra/*.cmake")

  foreach(file IN LISTS cmake_extra_files)
    include(${file})
  endforeach()

endif()
