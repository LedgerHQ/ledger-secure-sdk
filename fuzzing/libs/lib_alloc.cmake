include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

file(GLOB LIB_ALLOC_SOURCES "${BOLOS_SDK}/lib_alloc/*.c")

add_library(alloc ${LIB_ALLOC_SOURCES})
target_link_libraries(alloc PUBLIC macros)
target_compile_options(alloc PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  alloc PUBLIC ${BOLOS_SDK}/include/ ${BOLOS_SDK}/lib_alloc/
               ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
