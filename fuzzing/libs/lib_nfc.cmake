include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_NFC_SOURCES "${CMAKE_SOURCE_DIR}/../lib_nfc/src/nfc_ndef.c")

add_library(lib_nfc ${LIB_NFC_SOURCES})
target_compile_definitions(nfc_ndef PUBLIC HAVE_NFC HAVE_NDEF_SUPPORT)
target_include_directories(lib_nfc PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_nfc
                           PUBLIC ${CMAKE_SOURCE_DIR}/../lib_nfc/include/)
