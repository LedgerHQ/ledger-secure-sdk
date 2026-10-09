project(lib_tlv C)

file(GLOB TLV_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/use_cases/*.c
)

add_library(${PROJECT_NAME} STATIC ${TLV_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/use_cases
)

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
