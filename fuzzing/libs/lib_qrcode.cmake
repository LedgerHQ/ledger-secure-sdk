include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_QRCODE_SOURCES "${CMAKE_SOURCE_DIR}/../qrcode/src/qrcodegen.c")

add_library(lib_qrcode ${LIB_QRCODE_SOURCES})
target_include_directories(lib_qrcode PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_qrcode
                           PUBLIC ${CMAKE_SOURCE_DIR}/../qrcode/include/)
target_include_directories(
  lib_qrcode PUBLIC ${CMAKE_SOURCE_DIR}/../target/${TARGET_DEVICE}/include)
