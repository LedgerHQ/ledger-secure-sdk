project(io C)

file(GLOB IO_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_library(${PROJECT_NAME} STATIC ${IO_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)

if(ENABLE_ADDRESS_BOOK)
    target_link_libraries(${PROJECT_NAME} PRIVATE address_book)
endif()

if(LEDGER_FEATURE_BLE AND LEDGER_DEVICE_BUILD)
    target_link_libraries(${PROJECT_NAME} PUBLIC lib_blewbxx)
endif()

if(LEDGER_FEATURE_USB AND LEDGER_DEVICE_BUILD)
    target_link_libraries(${PROJECT_NAME} PUBLIC lib_stusb)
endif()
