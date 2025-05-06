include_guard()

include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_NFC_SOURCES "${BOLOS_SDK}/fuzzing/../lib_nfc/src/nfc_ndef.c")

add_library(lib_nfc ${LIB_NFC_SOURCES})
target_include_directories(lib_nfc PUBLIC ${BOLOS_SDK}/fuzzing/../include/)
target_include_directories(lib_nfc
                           PUBLIC ${BOLOS_SDK}/fuzzing/../lib_nfc/include/)
