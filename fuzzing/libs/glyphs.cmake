include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

find_package(Python3 REQUIRED)

set(GEN_GLYPHS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_glyphs)

set(GLYPH_OPT "")

# Building from App
if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND GLYPH_PATHS "${APP_GLYPH_DIR}/*")
endif()

if(TARGET_DEVICE STREQUAL "flex" OR TARGET_DEVICE STREQUAL "stax")
  list(APPEND GLYPH_PATHS ${BOLOS_SDK}/lib_nbgl/glyphs/wallet/*
       ${BOLOS_SDK}/lib_nbgl/glyphs/64px/*)
  if(TARGET_DEVICE STREQUAL "flex")
    list(APPEND GLYPH_PATHS ${BOLOS_SDK}/lib_nbgl/glyphs/40px/*)
  elseif(TARGET_DEVICE STREQUAL "stax")
    list(APPEND GLYPH_PATHS ${BOLOS_SDK}/lib_nbgl/glyphs/32px/*)
  endif()
else()
  list(APPEND GLYPH_PATHS ${BOLOS_SDK}/lib_nbgl/glyphs/nano/*)
  set(GLYPH_OPT "--reverse")
endif()

set(GLYPH_SCRIPT ${BOLOS_SDK}/lib_nbgl/tools/icon2glyph.py)

file(GLOB_RECURSE GLYPH_FILES ${GLYPH_PATHS})

# Outputs
set(GLYPHS_C ${GEN_GLYPHS_DIR}/glyphs.c)
set(GLYPHS_H ${GEN_GLYPHS_DIR}/glyphs.h)

# Add target to generate glyphs
set(GEN_GLYPHS_CMD "${BOLOS_SDK}/lib_nbgl/tools/icon2glyph.py")

file(MAKE_DIRECTORY ${GEN_GLYPHS_DIR})
add_custom_command(
  OUTPUT ${GLYPHS_C} ${GLYPHS_H}
  COMMAND ${Python3_EXECUTABLE} ${GEN_GLYPHS_CMD} ${GLYPH_OPT} --glyphcheader
          ${GLYPHS_H} --glyphcfile ${GLYPHS_C} ${GLYPH_FILES}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/"
  DEPENDS ${GLYPH_FILES}
  COMMENT "Generating glyphs..."
  VERBATIM)

add_library(glyphs STATIC EXCLUDE_FROM_ALL ${GLYPHS_C})
target_include_directories(
  glyphs
  PUBLIC ${GEN_GLYPHS_DIR} ${BOLOS_SDK}/include/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/include
         ${BOLOS_SDK}/lib_nbgl/include)
target_link_libraries(glyphs macros)
