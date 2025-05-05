include_guard()

include(${CMAKE_SOURCE_DIR}/mock/mock.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_NFC_SOURCES
    "${CMAKE_SOURCE_DIR}/../lib_nfc/src/nfc_ndef.c"
)

add_library(lib_nfc ${LIB_NFC_SOURCES})
target_include_directories(lib_nfc PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_nfc PUBLIC ${CMAKE_SOURCE_DIR}/../lib_nfc/include/)