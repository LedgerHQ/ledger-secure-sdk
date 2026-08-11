project(lib_nbgl C)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

#######################################################################
#                       glyph generation (NBGL)                       #
#######################################################################
set(GLYPHS_GEN_DIR ${LEDGER_GLYPHS_GEN_DIR})
set(GLYPHS_H       ${GLYPHS_GEN_DIR}/glyphs.h)
set(GLYPHS_C       ${GLYPHS_GEN_DIR}/glyphs.c)
set(ICON_SCRIPT    ${CMAKE_CURRENT_SOURCE_DIR}/tools/icon2glyph.py)

if(LEDGER_TARGET STREQUAL "stax")
    set(GLYPH_DIRS wallet 64px 32px)
elseif(LEDGER_TARGET STREQUAL "flex")
    set(GLYPH_DIRS wallet 64px 40px)
elseif(LEDGER_TARGET MATCHES "^apex_[pm]$")
    set(GLYPH_DIRS wallet 48px 24px)
else()
    set(GLYPH_DIRS nano)
    set(GLYPH_OPT --reverse)
endif()

list(TRANSFORM GLYPH_DIRS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/glyphs/)
list(TRANSFORM GLYPH_DIRS APPEND /*)
file(GLOB GLYPH_FILES CONFIGURE_DEPENDS ${GLYPH_DIRS})

list(APPEND GLYPH_FILES ${LEDGER_APP_GLYPH_FILES})

add_custom_command(
    OUTPUT ${GLYPHS_C} ${GLYPHS_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GLYPHS_GEN_DIR}
    COMMAND ${Python3_EXECUTABLE} ${ICON_SCRIPT}
            --glyphcheader ${GLYPHS_H}
            --glyphcfile   ${GLYPHS_C}
            ${GLYPH_OPT}
            ${GLYPH_FILES}
    DEPENDS ${GLYPH_FILES} ${ICON_SCRIPT}
    COMMENT "Generating NBGL glyphs"
    VERBATIM
)

# Consumers link this target to get both glyphs.h and the build ordering.
add_library(nbgl_glyphs STATIC ${GLYPHS_C})

target_include_directories(nbgl_glyphs PUBLIC ${GLYPHS_GEN_DIR})

target_include_directories(nbgl_glyphs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#######################################################################
#                             lib_nbgl                                #
#######################################################################
set(NBGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout_keyboard.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout_keyboard_nanos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout_keypad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout_keypad_nanos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout_nanos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout_navigation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_page.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_step.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_use_case.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_use_case_nanos.c
)

add_library(${PROJECT_NAME} STATIC ${NBGL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC nbgl_glyphs)

if(BUILD_UNIT_TESTS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${UT_COMPILE_DEFS_SDK})
    target_include_directories(nbgl_glyphs PRIVATE ${UT_INCLUDE_DIRS_SDK})
    target_compile_definitions(nbgl_glyphs PRIVATE ${UT_COMPILE_DEFS_SDK})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
target_include_directories(nbgl_glyphs PRIVATE ${LEDGER_SDK_INCLUDE_DIRS})
target_link_libraries(nbgl_glyphs PRIVATE ledger::target-profile ledger::compile-options)
