cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
        VERSION 0.1
        DESCRIPTION "Unit tests for Application storage"
        LANGUAGES C)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

include(CTest)
ENABLE_TESTING()

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

set(TEST test_app_storage)

add_executable(
  ${TEST}
  test_app_storage.c
  app_storage_stubs.c
  ../../lib_standard_app/app_storage.c
  ../../lib_cxng/src/cx_crc32.c
  ../../mock/src/cx_crc.c
)

target_include_directories(
  ${TEST}
  PRIVATE
  ./
  ../../target/stax/include
  ../../include
  ../../lib_cxng/include
  ../../lib_nbgl/include
  ../../lib_ux_nbgl
  ../../lib_standard_app
  ../mock/include
)

target_compile_options(
  ${TEST}
  PRIVATE
  -Wall
  -Werror
  -O0
  --coverage
  -fsanitize=address,undefined
)

target_compile_definitions(
  ${TEST}
  PRIVATE
  UNIT_TESTING
  SCREEN_SIZE_WALLET
  USB_SEGMENT_SIZE=64
  HAVE_NBGL
  WITH_STDIO
  HAVE_APP_STORAGE
  APP_STORAGE_SIZE=5555
  HAVE_APP_STORAGE_PROP_SETTINGS
  HAVE_APP_STORAGE_PROP_DATA
  HAVE_CRC
)

target_link_libraries(
  ${TEST}
  PRIVATE
  cmocka
  gcov
)

target_link_options(
  ${TEST}
  PRIVATE
  --coverage
  -lgcov
  -fsanitize=address,undefined
)

add_test(
  NAME ${TEST}
  COMMAND ${TEST}
)
