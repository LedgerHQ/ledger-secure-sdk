include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nfc.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

file(GLOB LIB_STANDARD_APP_SOURCES "${BOLOS_SDK}/lib_standard_app/*.c" "${BOLOS_SDK}/src/os_printf.c")
list(REMOVE_ITEM LIB_STANDARD_APP_SOURCES
     "${BOLOS_SDK}/lib_standard_app/main.c")
add_library(standard_app ${LIB_STANDARD_APP_SOURCES})
target_link_libraries(standard_app PUBLIC macros mock cxng io nfc)
target_link_options(standard_app PUBLIC
                    -Wl,--undefined=io_send_response_buffers)
target_compile_options(standard_app PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  standard_app
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_cxng" "${BOLOS_SDK}/lib_cxng/include"
         "${BOLOS_SDK}/lib_standard_app")
