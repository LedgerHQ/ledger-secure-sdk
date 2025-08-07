include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nfc.cmake)

file(
  GLOB
  LIB_IO_SOURCES
  "${BOLOS_SDK}/io/src/*.c"
  "${BOLOS_SDK}/io_legacy/src/*.c"
  "${BOLOS_SDK}/lib_blewbxx/src/*.c"
  "${BOLOS_SDK}/lib_blewbxx_impl/src/*.c"
  "${BOLOS_SDK}/lib_ccid/src/*.c"
  "${BOLOS_SDK}/lib_stusb/src/*.c"
  "${BOLOS_SDK}/lib_stusb_impl/src/*.c"
  "${BOLOS_SDK}/lib_u2f/src/*.c"
  "${BOLOS_SDK}/lib_u2f_legacy/src/*.c"
  "${BOLOS_SDK}/protocol/src/*.c"
  )

add_library(io ${LIB_IO_SOURCES})
target_link_libraries(io PUBLIC nbgl cxng macros nfc)
target_compile_options(io PUBLIC ${COMPILATION_FLAGS} -Wno-implicit-function-declaration)
target_include_directories(
  io
  PUBLIC ${BOLOS_SDK}/include/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/
         ${BOLOS_SDK}/io/include
         ${BOLOS_SDK}/io_legacy/include
         ${BOLOS_SDK}/lib_blewbxx/include
         ${BOLOS_SDK}/lib_blewbxx_impl/include
         ${BOLOS_SDK}/lib_ccid/include
         ${BOLOS_SDK}/lib_stusb/include
         ${BOLOS_SDK}/lib_stusb_impl/include
         ${BOLOS_SDK}/lib_u2f/include
         ${BOLOS_SDK}/lib_u2f_legacy/include
         ${BOLOS_SDK}/protocol/include
         )
