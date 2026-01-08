include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

file(GLOB LIB_C_LIST_SOURCES "${BOLOS_SDK}/lib_c_list/*.c")

add_library(c_list ${LIB_C_LIST_SOURCES})
target_link_libraries(c_list PUBLIC macros)
target_compile_options(c_list PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  c_list PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/lib_c_list/"
                "${BOLOS_SDK}/target/${TARGET}/include")
