cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
    VERSION 0.1
    DESCRIPTION "Unit tests for PKI helper"
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

# Common source files
set(COMMON_SOURCES
  mock_os.c
  ../../lib_tlv/tlv_library.c
  ../../lib_cxng/src/cx_crc32.c
  ../../mock/src/cx_crc.c
)

# Common source files
set(USE_CASE_SOURCES
  ../../lib_tlv/use_cases/tlv_use_case_dynamic_descriptor.c
  ../../lib_tlv/use_cases/tlv_use_case_trusted_name.c
)

# Common include directories
set(COMMON_INCLUDES
  ./
  ../../target/stax/include
  ../../include
  ../../lib_pki
  ../../lib_tlv
  ../../lib_tlv/use_cases
  ../../lib_cxng/include
  ../../lib_nbgl/include
  ../../lib_nbgl/include/fonts
  ../../lib_ux_nbgl
  ../../lib_standard_app
  ../mock/include
)

# Common compile options
set(COMMON_COMPILE_OPTIONS
  -Wall
  -Werror
  -O0
  --coverage
  -Wno-comment
  -fsanitize=address,undefined
)

# Common compile definitions
set(COMMON_DEFINITIONS
  UNIT_TESTING
  SCREEN_SIZE_WALLET
  USB_SEGMENT_SIZE=64
  HAVE_NBGL
  HAVE_HASH
  HAVE_SHA3
  HAVE_SHA256
  HAVE_SHA512
  HAVE_ED25519_CURVE
  HAVE_ECC_WEIERSTRASS
  HAVE_SECP256K1_CURVE
  HAVE_RIPEMD160
  APPNAME="UnitTests"
  HAVE_SHA224
  HAVE_LEDGER_PKI
  HAVE_ECC
  HAVE_ECC_TWISTED_EDWARDS
  WITH_STDIO
  HAVE_APP_STORAGE
  APP_STORAGE_SIZE=5555
  HAVE_APP_STORAGE_PROP_SETTINGS
  HAVE_APP_STORAGE_PROP_DATA
  HAVE_CRC
)

# Common link options
set(COMMON_LINK_OPTIONS
  --coverage
  -lgcov
  -fsanitize=address,undefined
)

# Function to create a test
function(add_tlv_test TEST_NAME TEST_SOURCE)
  set(options USE_CASE)
  cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})

  if(ARG_USE_CASE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${COMMON_SOURCES} ${USE_CASE_SOURCES})
  else()
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${COMMON_SOURCES})
  endif()

  target_include_directories(${TEST_NAME} PRIVATE ${COMMON_INCLUDES})
  target_compile_options(${TEST_NAME} PRIVATE ${COMMON_COMPILE_OPTIONS})
  target_compile_definitions(${TEST_NAME} PRIVATE ${COMMON_DEFINITIONS})
  target_link_libraries(${TEST_NAME} PRIVATE cmocka gcov)
  target_link_options(${TEST_NAME} PRIVATE ${COMMON_LINK_OPTIONS})

  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Add tests
add_tlv_test(test_tlv_helpers test_tlv_helpers.c)
add_tlv_test(test_tlv_macros test_tlv_macros.c)
add_tlv_test(test_tlv_use_case_dynamic_descriptor test_tlv_use_case_dynamic_descriptor.c USE_CASE)
add_tlv_test(test_tlv_use_case_trusted_name test_tlv_use_case_trusted_name.c USE_CASE)
