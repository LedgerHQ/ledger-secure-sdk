include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

file(GLOB LIB_list_SOURCES "${BOLOS_SDK}/lib_lists/*.c")

add_library(list ${LIB_list_SOURCES})
target_link_libraries(list PUBLIC macros)
target_compile_options(list PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  list PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/lib_lists/"
                "${BOLOS_SDK}/target/${TARGET}/include")
