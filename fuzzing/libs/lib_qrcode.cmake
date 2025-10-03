include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

file(GLOB LIB_QRCODE_SOURCES "${BOLOS_SDK}/qrcode/src/*.c")

add_library(qrcode ${LIB_QRCODE_SOURCES})
target_link_libraries(qrcode PUBLIC macros)
target_compile_options(qrcode PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  qrcode PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/qrcode/include/"
                "${BOLOS_SDK}/target/${TARGET}/include/")
