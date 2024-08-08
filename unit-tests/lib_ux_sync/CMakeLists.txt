cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
        VERSION 0.1
	      DESCRIPTION "Unit tests for UX_SYNC"
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
add_compile_definitions(SCREEN_SIZE_WALLET)
add_compile_definitions(USB_SEGMENT_SIZE=64)
add_compile_definitions(HAVE_NBGL)
add_compile_definitions(WITH_STDIO)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall ${DEFINES} -g -O0 --coverage")

set(GCC_COVERAGE_LINK_FLAGS "--coverage -lgcov")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

add_compile_definitions(TEST)

include_directories(.)
include_directories(../../target/stax/include)
include_directories(../../include)
include_directories(../../lib_nbgl/include)
include_directories(../../lib_nbgl/include/fonts)
include_directories(../../lib_ux_nbgl)
include_directories(../../lib_ux_sync/include)

add_executable(test_ux_sync test_ux_sync.c)

add_library(nbgl_stubs SHARED nbgl_stubs.c)
add_library(ux_sync SHARED ../../lib_ux_sync/src/ux_sync.c)

target_link_libraries(test_ux_sync PUBLIC cmocka gcov ux_sync nbgl_stubs)

add_test(test_ux_sync test_ux_sync)
