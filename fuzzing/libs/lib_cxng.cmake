include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

file(GLOB LIB_CXNG_SOURCES "${BOLOS_SDK}/lib_cxng/src/*.c")

add_library(cxng ${LIB_CXNG_SOURCES})
target_compile_options(cxng PUBLIC ${COMPILATION_FLAGS})
target_compile_definitions(cxng PUBLIC HAVE_FIXED_SCALAR_LENGTH)
target_link_libraries(cxng PUBLIC macros mock)
target_include_directories(
  cxng
  PUBLIC ${BOLOS_SDK}/include/ ${BOLOS_SDK}/target/${TARGET_DEVICE}/include
         ${BOLOS_SDK}/lib_cxng/ ${BOLOS_SDK}/lib_cxng/include
         ${BOLOS_SDK}/lib_cxng/src ${BOLOS_SDK}/)
