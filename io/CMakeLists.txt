project(io C)

set(IO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os_io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os_io_default_apdu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os_io_seph_cmd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os_io_seph_ux.c
)

add_library(${PROJECT_NAME} STATIC ${IO_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)

if(LEDGER_FEATURE_BLE AND LEDGER_DEVICE_BUILD)
    target_link_libraries(${PROJECT_NAME} PUBLIC lib_blewbxx)
endif()

if(LEDGER_FEATURE_USB AND LEDGER_DEVICE_BUILD)
    target_link_libraries(${PROJECT_NAME} PUBLIC lib_stusb)
endif()
