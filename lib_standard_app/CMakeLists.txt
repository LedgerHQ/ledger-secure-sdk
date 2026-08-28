project(lib_standard_app C)

# no main.c here (unit-tests have their own main)
set(STDAPP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/app_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/base58.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bip32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto_helpers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/format.c
    ${CMAKE_CURRENT_SOURCE_DIR}/io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/read.c
    ${CMAKE_CURRENT_SOURCE_DIR}/swap_error_code_helpers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/swap_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/varint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/write.c
)

add_library(${PROJECT_NAME} STATIC ${STDAPP_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
