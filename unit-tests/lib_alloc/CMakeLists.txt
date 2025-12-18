cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(unit_tests
        VERSION 0.1
	      DESCRIPTION "Unit tests for Dynamic Allocator"
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

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall ${DEFINES} -g -O0 --coverage")

set(GCC_COVERAGE_LINK_FLAGS "--coverage -lgcov")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

add_compile_definitions(TEST)
set(SDK_SRC ../..)

include_directories(.)
include_directories(${SDK_SRC}/target/stax/include)
include_directories(${SDK_SRC}/include)
include_directories(${SDK_SRC}/lib_alloc)

add_executable(test_mem_alloc
  test_mem_alloc.c
  ${SDK_SRC}/lib_alloc/mem_alloc.c
  ${SDK_SRC}/lib_alloc/app_mem_utils.c
)

target_link_libraries(test_mem_alloc PUBLIC cmocka gcov)

target_link_options(
  test_mem_alloc
  PRIVATE
  -Wl,--wrap=os_longjmp
)

add_test(test_mem_alloc test_mem_alloc)
