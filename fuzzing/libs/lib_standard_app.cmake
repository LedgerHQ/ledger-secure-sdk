include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_STANDARD_APP_SOURCES
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/app_storage.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/bip32.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/base58.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/buffer.c"
    #"${CMAKE_SOURCE_DIR}/../lib_standard_app/crypto_helpers.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/format.c"
    #"${CMAKE_SOURCE_DIR}/../lib_standard_app/main.c"
    #"${CMAKE_SOURCE_DIR}/../lib_standard_app/io.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/parser.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/swap_error_code_helpers.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/swap_utils.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/varint.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/write.c"
    "${CMAKE_SOURCE_DIR}/../lib_standard_app/read.c"
)

add_library(lib_standard_app ${LIB_STANDARD_APP_SOURCES})
target_include_directories(lib_standard_app INTERFACE ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_standard_app INTERFACE ${CMAKE_SOURCE_DIR}/../target/stax/include)
target_include_directories(lib_standard_app INTERFACE ${CMAKE_SOURCE_DIR}/../lib_cxng)
target_include_directories(lib_standard_app INTERFACE ${CMAKE_SOURCE_DIR}/../lib_cxng/include)
target_include_directories(lib_standard_app INTERFACE ${CMAKE_SOURCE_DIR}/../lib_standard_app)

