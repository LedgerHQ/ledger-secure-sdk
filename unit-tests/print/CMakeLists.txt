cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
        VERSION 0.1
	      DESCRIPTION "Unit tests for printf and snprintf"
        LANGUAGES C)

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

include(CTest)
ENABLE_TESTING()

# specify C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(TEST_APP test_print)
set(SDK_BASE ../..)

add_compile_definitions(TEST)

add_executable(
  ${TEST_APP}
  test_print.c
  ${SDK_BASE}/src/os_printf.c
)

target_include_directories(
  ${TEST_APP}
  PRIVATE
          /usr/include/bsd
          ./
          ${SDK_BASE}/target/stax/include
          ${SDK_BASE}/include
)

target_link_libraries(
  ${TEST_APP}
  PRIVATE
          cmocka
          gcov
          bsd
)

target_compile_options(
  ${TEST_APP}
  PRIVATE
          -Wall
          -Werror
          -g
          -O0
          --coverage
          -fsanitize=address,undefined
)

target_compile_definitions(
  ${TEST_APP}
  PRIVATE
          LIBBSD_OVERLAY
          UNIT_TESTING
          HAVE_PRINTF
          HAVE_SPRINTF
          HAVE_SNPRINTF_FORMAT_U
          HAVE_SNPRINTF_FORMAT_LL
)

target_link_options(
  ${TEST_APP}
  PRIVATE
          --coverage
          -lgcov
          -fsanitize=address,undefined
)

add_test(
  NAME    ${TEST_APP}
  COMMAND ${TEST_APP}
)
