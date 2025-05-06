include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_QRCODE_SOURCES "${BOLOS_SDK}/qrcode/src/qrcodegen.c")

add_library(lib_qrcode ${LIB_QRCODE_SOURCES})
target_include_directories(lib_qrcode PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(lib_qrcode PUBLIC ${BOLOS_SDK}/qrcode/include/)
target_include_directories(lib_qrcode
                           PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
