include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_NFC_SOURCES "${BOLOS_SDK}/lib_nfc/src/nfc_ndef.c")

add_library(nfc ${LIB_NFC_SOURCES})
target_link_libraries(nfc PUBLIC macros)
target_compile_options(nfc PUBLIC ${COMPILATION_FLAGS})
target_include_directories(nfc PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(nfc PUBLIC ${BOLOS_SDK}/lib_nfc/include/)
