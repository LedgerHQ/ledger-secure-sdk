include_guard()

add_library(macros INTERFACE)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(Blue "${Esc}[34m")
endif()

# Building from App
if(NOT "${BOLOS_SDK}/fuzzing" STREQUAL ${CMAKE_SOURCE_DIR})
  message(
    "${Blue}Importing macros from ${CMAKE_SOURCE_DIR}/macros/generated/macros.txt")
  file(STRINGS "${CMAKE_SOURCE_DIR}/macros/generated/macros.txt" MACRO_LIST)
  list(APPEND MACRO_LIST "FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1")
  target_compile_definitions(macros INTERFACE ${MACRO_LIST})

  # Building from SDK
else()
  if(${TARGET_DEVICE} STREQUAL "stax" OR ${TARGET_DEVICE} STREQUAL "flex")
    # Run `make list-defines` and capture output
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E env BOLOS_SDK=${BOLOS_SDK} TARGET=${TARGET_DEVICE} CC=${CMAKE_C_COMPILER} make list-defines
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/macros"
      OUTPUT_VARIABLE MACRO_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )

    if(NOT MACRO_OUTPUT)
      message(FATAL_ERROR "Failed to extract macros via 'make list-defines'")
    endif()

    string(REPLACE " " ";" MACRO_LIST "${MACRO_OUTPUT}")

    # Append the special fuzzing macro
    list(APPEND MACRO_LIST "FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1")
    list(REMOVE_ITEM MACRO_LIST "PRINTF\\(...\\)=")
    list(REMOVE_ITEM MACRO_LIST "USE_OS_IO_STACK")

    target_compile_definitions(macros INTERFACE ${MACRO_LIST})
  endif()
endif()

message("${Blue}Loaded macros from 'make list-defines' (${CMAKE_SOURCE_DIR}/macros):")
foreach(M ${MACRO_LIST})
  message("  - ${M}")
endforeach()
