project(lib_tlv C)

set(TLV_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tlv_library.c
    ${CMAKE_CURRENT_SOURCE_DIR}/use_cases/tlv_use_case_dynamic_descriptor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/use_cases/tlv_use_case_transaction_check.c
    ${CMAKE_CURRENT_SOURCE_DIR}/use_cases/tlv_use_case_trusted_name.c
)

add_library(${PROJECT_NAME} STATIC ${TLV_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/use_cases
)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
