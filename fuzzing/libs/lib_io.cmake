include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

file(
  GLOB
  LIB_IO_SOURCES
  "${BOLOS_SDK}/src/os_io_usb.c"
  "${BOLOS_SDK}/lib_ux_nbgl/ux.c"
  "${BOLOS_SDK}/src/os_io_task.c"
  "${BOLOS_SDK}/src/os_io_seproxyhal.c"
  "${BOLOS_SDK}/src/ledger_protocol.c"
  "${BOLOS_SDK}/lib_standard_app/io.c"
  "${BOLOS_SDK}/lib_stusb/*.c"
  "${BOLOS_SDK}/lib_stusb_impl/*.c"
  "${BOLOS_SDK}/lib_blewbxx_impl/src/*.c"
  "${BOLOS_SDK}/lib_blewbxx/core/auto/*.c"
  "${BOLOS_SDK}/lib_blewbxx/core/template/*.c"
  "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Core/Src/*.c"
  "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/HID/Src/*.c"
  "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/CCID/src/*.c"
  "${BOLOS_SDK}/lib_u2f/src/*.c"
  )

add_library(io ${LIB_IO_SOURCES})
target_link_libraries(io PUBLIC nbgl cxng standard_app macros)
target_compile_options(io PUBLIC ${COMPILATION_FLAGS})
target_compile_definitions(io PUBLIC 
HAVE_BLE
HAVE_BLE_APDU
)
target_include_directories(
  io
  PUBLIC ${BOLOS_SDK}/include/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/
         ${BOLOS_SDK}/lib_blewbxx/
         ${BOLOS_SDK}/lib_blewbxx/core/
         ${BOLOS_SDK}/lib_blewbxx/core/auto/
         ${BOLOS_SDK}/lib_blewbxx/core/template/
         ${BOLOS_SDK}/lib_blewbxx_impl/
         ${BOLOS_SDK}/lib_blewbxx_impl/include/
         ${BOLOS_SDK}/lib_stusb/
         ${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Core/Inc
         ${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/HID/Inc/
         ${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/CCID/inc/
         ${BOLOS_SDK}/lib_stusb_impl/
         ${BOLOS_SDK}/lib_u2f/include
         ${BOLOS_SDK}/)
