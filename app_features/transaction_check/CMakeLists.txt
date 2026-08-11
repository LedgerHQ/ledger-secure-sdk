project(transaction_check C)

file(GLOB TC_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_library(${PROJECT_NAME} STATIC ${TC_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC lib_tlv)
target_link_libraries(${PROJECT_NAME} PRIVATE app_features_common)

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
