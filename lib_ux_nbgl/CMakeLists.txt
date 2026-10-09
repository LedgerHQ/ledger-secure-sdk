project(lib_ux_nbgl C)

file(GLOB UX_NBGL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_library(${PROJECT_NAME} STATIC ${UX_NBGL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC nbgl_glyphs)

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
