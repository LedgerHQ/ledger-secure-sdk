project(lib_stusb C)

set(STUSB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ctlreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_desc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ioreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger_ccid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger_cdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger_hid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger_hid_kbd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger_hid_u2f.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_ledger_webusb.c
)

add_library(${PROJECT_NAME} STATIC ${STUSB_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
