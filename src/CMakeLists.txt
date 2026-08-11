project(sdk_core LANGUAGES C ASM)

if(BUILD_UNIT_TESTS AND ledger-secure-sdk_IS_TOP_LEVEL)
    add_subdirectory(unit-tests)
endif()

# not building below files in UT build
if(NOT LEDGER_DEVICE_BUILD)
    return()
endif()

add_library(sdk_app_metadata OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/app_metadata.c)

target_link_libraries(sdk_app_metadata PUBLIC
    ledger::sdk-headers ledger::target-profile ledger::compile-options)

set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/cx_hash_iovec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cx_wrappers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ledger_assert.c
    ${CMAKE_CURRENT_SOURCE_DIR}/os.c
    ${CMAKE_CURRENT_SOURCE_DIR}/os_printf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stack_protector.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cx_stubs.S
    ${CMAKE_CURRENT_SOURCE_DIR}/nbgl_stubs.S
    ${CMAKE_CURRENT_SOURCE_DIR}/stack_protector_init.S
    ${CMAKE_CURRENT_SOURCE_DIR}/svc_call.s
    ${CMAKE_CURRENT_SOURCE_DIR}/svc_cx_call.s
)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${LEDGER_SDK_ROOT}/include
    ${LEDGER_SDK_ROOT}/include/arm
    ${LEDGER_SDK_ROOT}/target/${LEDGER_TARGET}/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
