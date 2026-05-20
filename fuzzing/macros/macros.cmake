include_guard()

add_library(macros INTERFACE)

# Detect if CMAKE_SOURCE_DIR is inside the SDK fuzzing tree (sdk-fuzz or similar).
string(FIND "${CMAKE_SOURCE_DIR}" "${BOLOS_SDK}/fuzzing/" _SDK_FUZZ_PREFIX_POS)

# Building from App (source dir is NOT under BOLOS_SDK/fuzzing/)
if(_SDK_FUZZ_PREFIX_POS EQUAL -1 AND NOT "${BOLOS_SDK}/fuzzing" STREQUAL "${CMAKE_SOURCE_DIR}")
  set(DEFINES_MAKEFILE_DIRECTORY "${APP_BUILD_PATH}")
  # Building from SDK (source dir IS BOLOS_SDK/fuzzing or a subdirectory thereof)
else()
  set(DEFINES_MAKEFILE_DIRECTORY "${BOLOS_SDK}/fuzzing/macros")
endif()

# getting DEFINES from Makefile
execute_process(
  COMMAND
    bash "-c"
    "make list-defines BOLOS_SDK=${BOLOS_SDK} TARGET=${TARGET} | awk '/DEFINITIONS LIST/{flag=1;next}flag'"
  WORKING_DIRECTORY "${DEFINES_MAKEFILE_DIRECTORY}"
  OUTPUT_VARIABLE MACRO_OUTPUT
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

if(NOT MACRO_OUTPUT)
  message(FATAL_ERROR "Failed to extract macros via 'make list-defines'")
endif()

string(REPLACE "\n" ";" MACRO_LIST "${MACRO_OUTPUT}")

# Adding extra macro definitions
set(ADD_MACRO_FILE "${CMAKE_CURRENT_LIST_DIR}/add_macros.txt")
if(EXISTS "${ADD_MACRO_FILE}")
  file(STRINGS "${ADD_MACRO_FILE}" ADD_MACRO_LIST)
  list(FILTER ADD_MACRO_LIST EXCLUDE REGEX "^#.*$")
  list(APPEND MACRO_LIST ${ADD_MACRO_LIST})
endif()

set(_APP_ADD_MACROS "${CMAKE_SOURCE_DIR}/macros/add_macros.txt")
if(EXISTS "${_APP_ADD_MACROS}")
  file(STRINGS "${_APP_ADD_MACROS}" _ADD_LIST)
  list(FILTER _ADD_LIST EXCLUDE REGEX "^#.*$")
  list(APPEND MACRO_LIST ${_ADD_LIST})
endif()

# Excluding macro definitions
foreach(_EXCLUDE_FILE
    "${CMAKE_CURRENT_LIST_DIR}/exclude_macros.txt"
    "${CMAKE_SOURCE_DIR}/macros/exclude_macros.txt")
  if(EXISTS "${_EXCLUDE_FILE}")
    file(STRINGS "${_EXCLUDE_FILE}" _EXCLUDE_LIST)
    list(FILTER _EXCLUDE_LIST EXCLUDE REGEX "^#.*$")
    foreach(_EXCL ${_EXCLUDE_LIST})
      list(REMOVE_ITEM MACRO_LIST "${_EXCL}")
    endforeach()
  endif()
endforeach()

target_compile_definitions(macros INTERFACE ${MACRO_LIST})
