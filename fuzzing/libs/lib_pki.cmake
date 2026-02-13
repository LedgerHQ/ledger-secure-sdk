include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

file(GLOB LIB_PKI_SOURCES "${BOLOS_SDK}/lib_pki/*.c")

add_library(pki ${LIB_PKI_SOURCES})
target_compile_options(pki PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(pki PUBLIC macros mock cxng)
target_include_directories(
  pki
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_pki/" "${BOLOS_SDK}/lib_cxng/include")
