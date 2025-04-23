cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CPP_COMPILER clang)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(FuzzBip32
        VERSION 1.0
	      DESCRIPTION "Fuzzing of BIP32"
        LANGUAGES C)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()


if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()


# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

# compatible with ClusterFuzzLite
if (NOT DEFINED ENV{LIB_FUZZING_ENGINE})
	set(COMPILATION_FLAGS_ "-g -Wall -fsanitize=fuzzer,address,undefined")
else()
	set(COMPILATION_FLAGS_ "$ENV{LIB_FUZZING_ENGINE} $ENV{CXXFLAGS}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(REPLACE " " ";" COMPILATION_FLAGS ${COMPILATION_FLAGS_})

include(extra/Bip32.cmake)

add_executable(fuzz_bip32 fuzzer_bip32.c)

target_compile_options(fuzz_bip32 PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_bip32 PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(fuzz_bip32 PUBLIC bip32)



#[===[

cmake_minimum_required(VERSION 3.10)

project(Fuzzer
        VERSION 1.0
	      DESCRIPTION "SDK Fuzzer"
        LANGUAGES C)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CPP_COMPILER clang)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -DFUZZ -g -O0 -fdata-sections -ffunction-sections -funsigned-char -fshort-enums -fsanitize=fuzzer,address,undefined -fprofile-instr-generate -fcoverage-mapping"
)

include_directories(
  mock/
  ../lib_standard_app/
  ../include/
  ../qrcode/include/
)

add_definitions(-DHAVE_NFC -DHAVE_NDEF_SUPPORT)

add_library(base58 SHARED ../../lib_standard_app/base58.c)
add_library(bip32 SHARED ../../lib_standard_app/bip32.c)
add_library(read SHARED ../../lib_standard_app/read.c)
add_library(apdu_parser SHARED ../../lib_standard_app/parser.c)
add_library(qrcodegen SHARED ../../qrcode/src/qrcodegen.c mock/os_task.c)

add_executable(fuzz_apdu_parser fuzzer_apdu_parser.c)
add_executable(fuzz_base58 fuzzer_base58.c)
add_executable(fuzz_bip32 fuzzer_bip32.c)
add_executable(fuzz_qrcodegen fuzzer_qrcodegen.c)

target_link_libraries(fuzz_apdu_parser apdu_parser)
target_link_libraries(fuzz_base58 base58)
target_link_libraries(fuzz_bip32 bip32 read)
target_link_libraries(fuzz_qrcodegen qrcodegen)
]===]
