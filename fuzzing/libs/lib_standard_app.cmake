include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(
  GLOB
  LIB_STANDARD_APP_SOURCES
  "${BOLOS_SDK}/lib_standard_app/app_storage.c"
  "${BOLOS_SDK}/lib_standard_app/bip32.c"
  "${BOLOS_SDK}/lib_standard_app/base58.c"
  "${BOLOS_SDK}/lib_standard_app/buffer.c"
  "${BOLOS_SDK}/lib_standard_app/crypto_helpers.c"
  "${BOLOS_SDK}/lib_standard_app/format.c"
  #"${BOLOS_SDK}/lib_standard_app/main.c"
  "${BOLOS_SDK}/lib_standard_app/io.c"
  "${BOLOS_SDK}/lib_standard_app/parser.c"
  "${BOLOS_SDK}/lib_standard_app/swap_error_code_helpers.c"
  "${BOLOS_SDK}/lib_standard_app/swap_utils.c"
  "${BOLOS_SDK}/lib_standard_app/varint.c"
  "${BOLOS_SDK}/lib_standard_app/write.c"
  "${BOLOS_SDK}/lib_standard_app/read.c")

add_library(standard_app ${LIB_STANDARD_APP_SOURCES})
target_link_libraries(standard_app PUBLIC macros mock cxng)
target_compile_options(standard_app PUBLIC ${COMPILATION_FLAGS})
target_include_directories(standard_app PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(standard_app PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
target_include_directories(standard_app PUBLIC ${BOLOS_SDK}/lib_cxng)
target_include_directories(standard_app PUBLIC ${BOLOS_SDK}/lib_cxng/include)
target_include_directories(standard_app PUBLIC ${BOLOS_SDK}/lib_standard_app)
