project(lib_nfc C)

set(NFC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nfc_ledger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nfc_ndef.c
)

add_library(${PROJECT_NAME} STATIC ${NFC_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
