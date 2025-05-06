include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_NFC_SOURCES "${BOLOS_SDK}/lib_nfc/src/nfc_ndef.c")

add_library(lib_nfc ${LIB_NFC_SOURCES})
target_include_directories(lib_nfc PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(lib_nfc PUBLIC ${BOLOS_SDK}/lib_nfc/include/)
