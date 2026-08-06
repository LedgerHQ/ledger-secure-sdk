include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_glyphs.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_qrcode.cmake)

file(GLOB LIB_NBGL_SOURCES CONFIGURE_DEPENDS "${BOLOS_SDK}/lib_nbgl/src/*.c"
     "${BOLOS_SDK}/lib_ux_nbgl/*.c")
# Excluded: fuzzing/mock provides auto-approve nbgl_use_case stubs so post-approval paths stay reachable.
list(FILTER LIB_NBGL_SOURCES EXCLUDE REGEX "nbgl_use_case[^/]*\\.c$")
add_library(nbgl STATIC ${LIB_NBGL_SOURCES})
target_link_libraries(nbgl PUBLIC macros glyphs qrcode mock io)
# Ten nbgl sources #include "glyphs.h", which glyphs generates. The link
# dependency above gives no ordering edge -- the SDK libraries form a cycle that
# CMake collapses -- so state it explicitly or cold parallel builds fail.
add_dependencies(nbgl glyphs)
target_compile_options(nbgl PRIVATE ${COMPILATION_FLAGS})
target_include_directories(
  nbgl
  PUBLIC "${BOLOS_SDK}/lib_cxng/include/"
         "${BOLOS_SDK}/include/"
         "${BOLOS_SDK}/lib_ux_nbgl/"
         "${BOLOS_SDK}/lib_nbgl/include/"
         "${BOLOS_SDK}/lib_nbgl/src/"
         "${BOLOS_SDK}/target/${TARGET}/include")
