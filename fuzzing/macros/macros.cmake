include_guard()

add_library(macros INTERFACE)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(Blue "${Esc}[34m")
endif()

# Building from App
if(NOT "${BOLOS_SDK}/fuzzing" STREQUAL ${CMAKE_SOURCE_DIR})
  set(DEFINES_MAKEFILE_DIRECTORY "${APP_BUILD_PATH}")
  # Building from SDK
else()
  set(DEFINES_MAKEFILE_DIRECTORY "${BOLOS_SDK}/fuzzing/macros")
endif()

# getting DEFINES from Makefile
execute_process(
  COMMAND
    bash "-c"
    "make list-defines BOLOS_SDK=${BOLOS_SDK} TARGET=${TARGET} | awk '/DEFINITIONS LIST/{flag=1;next}flag'" # prints the
                                                                                                            # output of
                                                                                                            # command
                                                                                                            # only after
                                                                                                            # "DEFINITIONS
                                                                                                            # LIST"
                                                                                                            # marker to
                                                                                                            # avoid
                                                                                                            # warnings
                                                                                                            # and other
                                                                                                            # logs
  WORKING_DIRECTORY "${DEFINES_MAKEFILE_DIRECTORY}"
  OUTPUT_VARIABLE MACRO_OUTPUT
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

if(NOT MACRO_OUTPUT)
  message(FATAL_ERROR "Failed to extract macros via 'make list-defines'")
endif()

string(REPLACE "\n" ";" MACRO_LIST "${MACRO_OUTPUT}")

# Adding extra macro definitions
set(ADD_MACRO_FILE "${CMAKE_SOURCE_DIR}/macros/add_macros.txt")
if(EXISTS "${ADD_MACRO_FILE}")
  file(STRINGS "${ADD_MACRO_FILE}" ADD_MACRO_LIST)
  list(APPEND ADD_MACRO_LIST "FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1")
  list(APPEND MACRO_LIST ${ADD_MACRO_LIST})
endif()

# Excluding macro definitions
set(EXCLUDE_MACRO_FILE "${CMAKE_SOURCE_DIR}/macros/exclude_macros.txt")
if(EXISTS "${EXCLUDE_MACRO_FILE}")
  file(STRINGS "${EXCLUDE_MACRO_FILE}" EXCLUDE_MACRO_LIST)
  foreach(EXCLUDE_MACRO ${EXCLUDE_MACRO_LIST})
    list(REMOVE_ITEM MACRO_LIST "${EXCLUDE_MACRO}")
  endforeach()
endif()

target_compile_definitions(macros INTERFACE ${MACRO_LIST})
