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

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.S
    ${CMAKE_CURRENT_SOURCE_DIR}/*.s
)
# Built above as sdk_app_metadata.
list(REMOVE_ITEM CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/app_metadata.c)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${LEDGER_SDK_ROOT}/include
    ${LEDGER_SDK_ROOT}/include/arm
    ${LEDGER_SDK_ROOT}/target/${TARGET}/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
