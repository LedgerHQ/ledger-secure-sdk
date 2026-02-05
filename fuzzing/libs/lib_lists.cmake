include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

file(GLOB LIB_LISTS_SOURCES "${BOLOS_SDK}/lib_lists/*.c")

add_library(lists ${LIB_LISTS_SOURCES})
target_link_libraries(lists PUBLIC macros)
target_compile_options(lists PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  lists PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/lib_lists/"
                "${BOLOS_SDK}/target/${TARGET}/include")
