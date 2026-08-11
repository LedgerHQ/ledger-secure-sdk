if(NOT LEDGER_CMAKE_MINIMUM_VERSION)
    message(FATAL_ERROR "Configure with a preset: cmake --preset <name>")
endif()
cmake_minimum_required(VERSION ${LEDGER_CMAKE_MINIMUM_VERSION})

project(ledger-secure-sdk
        LANGUAGES C ASM
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LEDGER_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Ledger Secure SDK root")

include(${LEDGER_SDK_ROOT}/cmake/sdk/target_profile.cmake)
include(${LEDGER_SDK_ROOT}/cmake/sdk/compile_options.cmake)

set(LEDGER_GLYPHS_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib_nbgl/generated
    CACHE INTERNAL "Generated NBGL glyphs dir")

include(${LEDGER_SDK_ROOT}/cmake/sdk/headers.cmake)

if(BUILD_UNIT_TESTS)
    include(${LEDGER_SDK_ROOT}/cmake/LedgerUT.cmake)
    # Reaches every component through its compile options.
    target_link_libraries(ledger_compile_options INTERFACE ledger::ut-host-shims)
    if(PROJECT_IS_TOP_LEVEL)
        ledger_unit_tests_init()
    endif()
endif()

if(NOT PROJECT_IS_TOP_LEVEL)
    set(LEDGER_APP_ICON ${LEDGER_APP_ICON_${TARGET}})
    if(LEDGER_DEVICE_BUILD AND NOT LEDGER_APP_ICON)
        message(FATAL_ERROR "ledger_app: an ICON_<TARGET> is required for TARGET '${TARGET}'")
    endif()
    if(LEDGER_APP_GLYPHS_DIR)
        file(GLOB _app_glyphs CONFIGURE_DEPENDS ${LEDGER_APP_GLYPHS_DIR}/*)
    endif()
    set(LEDGER_APP_GLYPH_FILES ${_app_glyphs})

    # Nano home screen icon: the app icon reversed, generated as in Makefile.glyphs.
    if(LEDGER_APP_ICON_HOME_NANO AND LEDGER_TARGET_IS_NANO)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        get_filename_component(_home_name ${LEDGER_APP_ICON_HOME_NANO} NAME)
        set(_home_icon ${CMAKE_BINARY_DIR}/home_icon/${_home_name})
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/home_icon)
        execute_process(
            COMMAND ${Python3_EXECUTABLE} ${LEDGER_SDK_ROOT}/lib_nbgl/tools/mask_icon.py
                    --input ${LEDGER_APP_ICON} --output ${_home_icon}
            COMMAND_ERROR_IS_FATAL ANY)
        set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LEDGER_APP_ICON})
        # A stale copy left in the app glyphs by a Makefile build would be a duplicate.
        list(FILTER LEDGER_APP_GLYPH_FILES EXCLUDE REGEX "/${_home_name}$")
        list(APPEND LEDGER_APP_GLYPH_FILES ${_home_icon})
    endif()
endif()

include(${LEDGER_SDK_ROOT}/cmake/sdk/components.cmake)
