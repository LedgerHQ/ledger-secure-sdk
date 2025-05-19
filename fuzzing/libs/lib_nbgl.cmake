include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/libs/glyphs.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_qrcode.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(
  GLOB
  LIB_NBGL_SOURCES
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_buttons.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_draw.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_flow.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_fonts.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_internal_nanos.h"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_internal.h"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_keyboard_nanos.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_keyboard.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_keypad_nanos.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_keypad.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_nanos.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout_navigation.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_layout.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_obj_keyboard_nanos.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_obj_keyboard.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_obj_keypad_nanos.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_obj_keypad.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_obj_pool.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_obj.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_page.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_screen.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_serialize.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_step.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_touch.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_use_case.c"
  "${BOLOS_SDK}/lib_nbgl/src/nbgl_use_case_nanos.c")

add_library(nbgl ${LIB_NBGL_SOURCES})
target_link_libraries(nbgl PUBLIC macros glyphs qrcode mock io)
target_compile_options(nbgl PUBLIC ${COMPILATION_FLAGS})
target_include_directories(nbgl PUBLIC ${LIB_NBGL_FONT_DIRS})
target_include_directories(
  nbgl
  PUBLIC ${BOLOS_SDK}/fuzzing/glyphs/
         ${BOLOS_SDK}/lib_nbgl/include/fonts/
         ${BOLOS_SDK}/lib_cxng/include/
         ${BOLOS_SDK}/include/
         ${BOLOS_SDK}/lib_ux_nbgl/
         ${BOLOS_SDK}/lib_nbgl/include/
         ${BOLOS_SDK}/lib_nbgl/src/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
