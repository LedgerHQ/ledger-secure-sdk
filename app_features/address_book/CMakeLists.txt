project(address_book C)

set(AB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ledger_account_edit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address_book.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/identity_edit_contact_name.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address_book_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/identity_edit_identifier.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/identity_edit_scope.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/address_book_crypto.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ledger_account_provide_contact.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/identity_provide_contact.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/identity_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ledger_account_register.c
)

add_library(${PROJECT_NAME} STATIC ${AB_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE nbgl_glyphs)

option(ENABLE_ADDRESS_BOOK_LEDGER_ACCOUNT "Address book for Ledger accounts" OFF)

target_compile_definitions(${PROJECT_NAME} PUBLIC HAVE_ADDRESS_BOOK)
if(ENABLE_ADDRESS_BOOK_LEDGER_ACCOUNT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)
endif()

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK} WITH_STDIO)
endif()

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
