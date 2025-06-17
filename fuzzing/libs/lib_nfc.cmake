include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)

file(GLOB LIB_NFC_SOURCES "${BOLOS_SDK}/lib_nfc/src/*.c")

add_library(nfc ${LIB_NFC_SOURCES})
target_link_libraries(nfc PUBLIC macros io)
target_compile_options(nfc PUBLIC ${COMPILATION_FLAGS})
target_include_directories(nfc PUBLIC ${BOLOS_SDK}/include/
                                      ${BOLOS_SDK}/lib_nfc/include/)
