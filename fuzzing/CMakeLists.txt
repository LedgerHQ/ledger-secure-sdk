cmake_minimum_required(VERSION 3.14)

# project information
project(SDKFuzzer
        VERSION 1.0
	      DESCRIPTION "SDK Fuzzer"
        LANGUAGES C)

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# default fuzz device target
if (NOT TARGET_DEVICE)
  set(TARGET_DEVICE "flex")
endif()

# compatible with ClusterFuzzLite
if (NOT DEFINED ENV{LIB_FUZZING_ENGINE})
	set(COMPILATION_FLAGS -g -O0 -Wall -Wextra -fprofile-instr-generate -fcoverage-mapping)
  if (SANITIZER MATCHES "address")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS} -fsanitize=fuzzer,address,undefined)
  elseif (SANITIZER MATCHES "memory")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS} -fsanitize=fuzzer,memory,undefined -fsanitize-memory-track-origins -fsanitize=fuzzer-no-link)
  else()
    message(FATAL_ERROR "Unknown sanitizer type. It must be set to `address` or `memory`.")
  endif()
else()
	set(COMPILATION_FLAGS "$ENV{LIB_FUZZING_ENGINE} $ENV{CFLAGS}")
  separate_arguments(COMPILATION_FLAGS)
endif()

add_compile_options(${COMPILATION_FLAGS})
add_link_options(${COMPILATION_FLAGS})

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()


include_directories(
  mock/
  ../include/
  ../target/${TARGET_DEVICE}/include
  ../lib_alloc/
  ../lib_standard_app/
  ../qrcode/include/
)

# Libraries
add_library(lib_alloc ../../lib_alloc/mem_alloc.c)
add_library(lib_standard ../../lib_standard_app/base58.c ../../lib_standard_app/bip32.c ../../lib_standard_app/read.c ../../lib_standard_app/parser.c)
add_library(lib_mock mock/os_task.c)

# Targets
add_executable(fuzz_alloc fuzzer_alloc.c)
add_executable(fuzz_apdu_parser fuzzer_apdu_parser.c)
add_executable(fuzz_base58 fuzzer_base58.c)
add_executable(fuzz_bip32 fuzzer_bip32.c)
add_executable(fuzz_qrcodegen fuzzer_qrcodegen.c ../../qrcode/src/qrcodegen.c)


target_link_libraries(fuzz_alloc lib_alloc)
target_link_libraries(fuzz_apdu_parser lib_standard lib_mock)
target_link_libraries(fuzz_base58 lib_standard lib_mock)
target_link_libraries(fuzz_bip32 lib_standard lib_mock)
target_link_libraries(fuzz_qrcodegen lib_mock)


