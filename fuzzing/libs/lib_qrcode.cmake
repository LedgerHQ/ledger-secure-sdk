include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_QRCODE_SOURCES "${BOLOS_SDK}/qrcode/src/qrcodegen.c")

add_library(qrcode ${LIB_QRCODE_SOURCES})
target_link_libraries(qrcode PUBLIC macros)
target_compile_options(qrcode PUBLIC ${COMPILATION_FLAGS})
target_include_directories(qrcode PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(qrcode PUBLIC ${BOLOS_SDK}/qrcode/include/)
target_include_directories(qrcode
                           PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
