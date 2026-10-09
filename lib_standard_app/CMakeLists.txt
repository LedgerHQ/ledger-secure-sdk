project(lib_standard_app C)

file(GLOB STDAPP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)
# Built by ledger_app() into app.elf; unit tests have their own main.
list(REMOVE_ITEM STDAPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

add_library(${PROJECT_NAME} STATIC ${STDAPP_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
