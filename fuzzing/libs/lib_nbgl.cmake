include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/libs/glyphs.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_qrcode.cmake)
include(${BOLOS_SDK}/fuzzing/mock/mock.cmake)

file(GLOB LIB_NBGL_SOURCES "${BOLOS_SDK}/lib_nbgl/src/*.c"
     "${BOLOS_SDK}/lib_ux_nbgl/*.c")
add_library(nbgl ${LIB_NBGL_SOURCES})
target_link_libraries(nbgl PUBLIC macros glyphs qrcode mock io)
target_compile_options(nbgl PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  nbgl
  PUBLIC "${LIB_NBGL_FONT_DIRS}"
         "${BOLOS_SDK}/lib_nbgl/include/fonts/"
         "${BOLOS_SDK}/lib_cxng/include/"
         "${BOLOS_SDK}/include/"
         "${BOLOS_SDK}/lib_ux_nbgl/"
         "${BOLOS_SDK}/lib_nbgl/include/"
         "${BOLOS_SDK}/lib_nbgl/src/"
         "${BOLOS_SDK}/target/${TARGET}/include")
