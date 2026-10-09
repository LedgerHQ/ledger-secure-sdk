project(io_legacy C)

file(GLOB IO_LEGACY_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_library(${PROJECT_NAME} STATIC ${IO_LEGACY_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)

if(LEDGER_DEVICE_BUILD)
    target_link_libraries(${PROJECT_NAME} PUBLIC lib_u2f_legacy)
endif()
