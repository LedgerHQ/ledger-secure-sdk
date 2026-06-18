#######################################################################
#                           LedgerUT setup                            #
#######################################################################
include(FetchContent)
include(CTest)
enable_testing()
set(LEDGER_UT_DIR ${CMAKE_CURRENT_LIST_DIR})

#######################################################################
#                    include defines and includes                     #
#######################################################################
include(${LEDGER_UT_DIR}/ut_include_dirs.cmake)
include(${LEDGER_UT_DIR}/ut_compile_defines.cmake)

#######################################################################
#                     Fetch UT/Mocking framework                      #
#######################################################################
FetchContent_Declare(unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity
    GIT_TAG        bbf8f3728a937c7627b8094de7ae13559d220ed5
)

FetchContent_Declare(cmock
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/CMock
    GIT_TAG        3d4ddfdc4cd7fab87b5b45b5799ea3a5320024e2
)

FetchContent_MakeAvailable(unity cmock)

set(UNITY_DIR ${unity_SOURCE_DIR} CACHE INTERNAL "Unity source dir")
set(CMOCK_DIR ${cmock_SOURCE_DIR} CACHE INTERNAL "CMock source dir")

find_program(RUBY_EXECUTABLE ruby REQUIRED)

#######################################################################
#                       ledger_unit_tests_init                        #
#######################################################################
macro(ledger_unit_tests_init)
    # setup ctest
    include(CTest)
    enable_testing()

    # guard against bad build-type strings
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug")
    endif()

    # specify C standard
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED True)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -g -O0 --coverage")

    # coverage flags for linking (for gcov)
    set(GCC_COVERAGE_LINK_FLAGS "--coverage -lgcov")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

    # guard against in-source builds
    if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
    endif()

    set(CMOCK_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/mocks)
    file(MAKE_DIRECTORY ${CMOCK_GEN_DIR})
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/mocks)
    configure_file(${LEDGER_UT_DIR}/cmock_config.yml.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmock_config.yml
        @ONLY)

    add_compile_definitions(TEST)
    add_compile_options("-DPRINTF(...)=")
endmacro()

#######################################################################
#                   ledger_unit_tests_find_sources                    #
#######################################################################
macro(ledger_unit_tests_find_sources)
    # Build all app sources except main to have a relevant overall coverage.
    file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../src/*.c)
    list(FILTER APP_SOURCES EXCLUDE REGEX "/(app_)?main\\.c$")
endmacro()

#######################################################################
#             ledger_unit_tests_build_app_sources_as_lib              #
#######################################################################
function(ledger_unit_tests_build_app_sources_as_lib)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDE_DIRS;COMPILE_DEFS" ${ARGN})
    add_library(app STATIC ${ARG_SOURCES})
    target_include_directories(app PRIVATE ${UT_INCLUDE_DIRS_SDK} ${ARG_INCLUDE_DIRS})
    target_compile_definitions(app PRIVATE ${UT_COMPILE_DEFS_SDK} ${ARG_COMPILE_DEFS})
endfunction()

# To check if a header is already mocked (generate mock only once, used by ledger_unit_tests_add_test)
define_property(GLOBAL PROPERTY CMOCK_MOCKED_HEADERS
    BRIEF_DOCS "Headers for which a CMock generation rule exists"
    FULL_DOCS  "Used by add_cmock_test to avoid duplicate custom commands.")
set_property(GLOBAL PROPERTY CMOCK_MOCKED_HEADERS "")

#######################################################################
# ledger_unit_tests_add_test — declare a Unity/CMock unit-test executable.
# Usage:
#   ledger_unit_tests_add_test(
#       NAME          test_foo
#       SOURCES       ../src/foo.c ../src/bar.c    # sources under test
#       MOCK_HEADERS  ${SDK_DIR}/include/os.h      # headers to be mocked (optional)
#       INCLUDE_DIRS  ../src                       # extra -I paths (optional)
#       COMPILE_DEFS  TARGET_NANOX APPNAME="App"   # extra -D flags  (optional)
#   )
#######################################################################
function(ledger_unit_tests_add_test)
    # get args
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;MOCK_HEADERS;INCLUDE_DIRS;COMPILE_DEFS" ${ARGN})

    # for all MOCK_HEADERS, generate a mock source file if not already generated
    set(_mock_sources "")
    foreach(_header ${ARG_MOCK_HEADERS})
        get_filename_component(_basename ${_header} NAME_WE)
        set(_mock_c ${CMOCK_GEN_DIR}/Mock${_basename}.c)
        list(APPEND _mock_sources ${_mock_c})

        get_property(_already GLOBAL PROPERTY CMOCK_MOCKED_HEADERS)
        if(NOT "${_header}" IN_LIST _already)
            add_custom_command(
                OUTPUT ${_mock_c}
                COMMAND ${RUBY_EXECUTABLE} ${CMOCK_DIR}/lib/cmock.rb
                        -o${CMAKE_CURRENT_BINARY_DIR}/cmock_config.yml
                        ${_header}
                DEPENDS ${_header}
                        ${CMAKE_CURRENT_BINARY_DIR}/cmock_config.yml
                COMMENT "CMock: generating Mock${_basename}"
                VERBATIM)
            set_property(GLOBAL APPEND PROPERTY CMOCK_MOCKED_HEADERS "${_header}")
        endif()
    endforeach()

    # add test executable
    add_executable(${ARG_NAME}
        ${ARG_NAME}.c
        ${ARG_SOURCES}
        ${_mock_sources}
        ${UNITY_DIR}/src/unity.c
        ${CMOCK_DIR}/src/cmock.c)

    target_include_directories(${ARG_NAME} BEFORE PRIVATE
        ${CMOCK_GEN_DIR}
        ${UNITY_DIR}/src
        ${CMOCK_DIR}/src
        ${ARG_INCLUDE_DIRS})

    if(ARG_COMPILE_DEFS)
        target_compile_definitions(${ARG_NAME} PRIVATE ${ARG_COMPILE_DEFS})
    endif()

    # register test
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
endfunction()

#######################################################################
#                           custom targets                            #
#######################################################################
add_custom_target(generate_coverage
    COMMAND ${LEDGER_UT_DIR}/gen_coverage.sh ${CMAKE_SOURCE_DIR}/../src
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating coverage report"
    VERBATIM
)
