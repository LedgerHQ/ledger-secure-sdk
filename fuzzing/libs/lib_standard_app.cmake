include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

file(GLOB LIB_STANDARD_APP_SOURCES "${BOLOS_SDK}/lib_standard_app/*.c")
list(REMOVE_ITEM LIB_STANDARD_APP_SOURCES
     "${BOLOS_SDK}/lib_standard_app/main.c")
add_library(standard_app ${LIB_STANDARD_APP_SOURCES})
target_link_libraries(standard_app PUBLIC macros mock cxng)
target_compile_options(standard_app PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  standard_app
  PUBLIC ${BOLOS_SDK}/include/ ${BOLOS_SDK}/target/${TARGET_DEVICE}/include
         ${BOLOS_SDK}/lib_cxng ${BOLOS_SDK}/lib_cxng/include
         ${BOLOS_SDK}/lib_standard_app)
