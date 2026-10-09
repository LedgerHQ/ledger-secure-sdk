project(lib_nbgl C)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

#######################################################################
#                       glyph generation (NBGL)                       #
#######################################################################
set(GLYPHS_GEN_DIR ${LEDGER_GLYPHS_GEN_DIR})
set(GLYPHS_H       ${GLYPHS_GEN_DIR}/glyphs.h)
set(GLYPHS_C       ${GLYPHS_GEN_DIR}/glyphs.c)
set(ICON_SCRIPT    ${CMAKE_CURRENT_SOURCE_DIR}/tools/icon2glyph.py)

if(LEDGER_TARGET_IS_NANO)
    set(GLYPH_OPT --reverse)
endif()

# Same order as Makefile.glyphs: icon2glyph.py keeps the first glyph of a name.
set(GLYPH_FILES ${LEDGER_APP_GLYPH_FILES})
foreach(_dir IN LISTS LEDGER_TARGET_GLYPH_DIRS)
    file(GLOB _dir_glyphs CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/glyphs/${_dir}/*)
    list(APPEND GLYPH_FILES ${_dir_glyphs})
endforeach()
list(APPEND GLYPH_FILES ${LEDGER_APP_ICON})

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
# Read by the `listparams` target.
set_target_properties(nbgl_glyphs PROPERTIES LEDGER_GLYPH_FILES "${GLYPH_FILES}")

target_include_directories(nbgl_glyphs PUBLIC ${GLYPHS_GEN_DIR})

target_include_directories(nbgl_glyphs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#######################################################################
#                             lib_nbgl                                #
#######################################################################
file(GLOB NBGL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_layout*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_page*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_step*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nbgl_use_case*.c
)

add_library(${PROJECT_NAME} STATIC ${NBGL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC nbgl_glyphs)

target_link_libraries(${PROJECT_NAME} PUBLIC ledger::sdk-headers ledger::target-profile ledger::compile-options)
target_include_directories(nbgl_glyphs PRIVATE ${LEDGER_SDK_INCLUDE_DIRS})
target_link_libraries(nbgl_glyphs PRIVATE ledger::target-profile ledger::compile-options)
