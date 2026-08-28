project(lib_blewbxx C)

set(BLEWBXX_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ble_cmd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ble_ledger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ble_ledger_profile_apdu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ble_ledger_profile_u2f.c
)

add_library(${PROJECT_NAME} STATIC ${BLEWBXX_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
