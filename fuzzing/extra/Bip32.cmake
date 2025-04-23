cmake_minimum_required(VERSION 3.10)

project(Bip32
        VERSION 1.0
	      DESCRIPTION "BIP_32"
        LANGUAGES C)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CPP_COMPILER clang)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-function -DFUZZ -pedantic -g -O0"
)

include_directories(
  ../lib_standard_app/
)

add_library(bip32
    ../lib_standard_app/read.c
    ../lib_standard_app/bip32.c
    
)
set_target_properties(bip32 PROPERTIES SOVERSION 1)

target_include_directories(bip32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib_standard_app
)
