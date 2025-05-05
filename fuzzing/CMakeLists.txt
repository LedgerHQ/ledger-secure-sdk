include_guard()
cmake_minimum_required(VERSION 3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.14)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(SDKFuzzer
        VERSION 1.0
	      DESCRIPTION "SDK Fuzzer"
        LANGUAGES C)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()


if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()

# default fuzz device target
if (NOT TARGET_DEVICE)
  set(TARGET_DEVICE "flex")
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
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

# Include haness cmake files
file(GLOB cmake_extra_files "extra/*.cmake")

foreach(file IN LISTS cmake_extra_files)
    include(${file})
endforeach()
