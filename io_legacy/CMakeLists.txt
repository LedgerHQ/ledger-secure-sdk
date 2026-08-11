project(io_legacy C)

set(IO_LEGACY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/os_io_legacy.c
)

add_library(${PROJECT_NAME} STATIC ${IO_LEGACY_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)

if(LEDGER_DEVICE_BUILD)
    target_link_libraries(${PROJECT_NAME} PUBLIC lib_u2f_legacy)
endif()
