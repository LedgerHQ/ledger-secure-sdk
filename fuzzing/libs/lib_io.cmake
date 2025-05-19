include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)

file(GLOB LIB_IO_SOURCES
     "${BOLOS_SDK}/src/os_io_usb.c"
     "${BOLOS_SDK}/src/os_io_seproxyhal.c"
     "${BOLOS_SDK}/src/os_io_task.c"
     "${BOLOS_SDK}/src/ledger_protocol.c"
     
     "${BOLOS_SDK}/lib_standard_app/io.c"
     
     "${BOLOS_SDK}/lib_blewbxx/core/auto/ble_gap_aci.c" 
     "${BOLOS_SDK}/lib_blewbxx/core/auto/ble_gatt_aci.c" 
     "${BOLOS_SDK}/lib_blewbxx/core/auto/ble_hal_aci.c"
     "${BOLOS_SDK}/lib_blewbxx/core/auto/ble_hci_le.c"
     "${BOLOS_SDK}/lib_blewbxx/core/auto/ble_l2cap_aci.c" 
     "${BOLOS_SDK}/lib_blewbxx/core/template/osal.c"
     
     "${BOLOS_SDK}/lib_blewbxx_impl/src/ledger_ble.c"
     
     "${BOLOS_SDK}/lib_stusb/usbd_conf.c"
     "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Core/Src/usbd_core.c"
     "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c"
     "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c"
     "${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/HID/Src/usbd_hid.c"
     
     "${BOLOS_SDK}/lib_stusb_impl/u2f_impl.c"
     "${BOLOS_SDK}/lib_stusb_impl/u2f_io.c"
     "${BOLOS_SDK}/lib_stusb_impl/usbd_impl.c"

     "${BOLOS_SDK}/lib_ux_nbgl/ux.c"
     )

add_library(io ${LIB_IO_SOURCES})
target_link_libraries(io PUBLIC nbgl cxng standard_app macros)
target_compile_options(io PUBLIC ${COMPILATION_FLAGS})
target_include_directories(io PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_blewbxx/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_blewbxx/core/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_blewbxx/core/auto/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_blewbxx/core/template/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_blewbxx_impl/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_blewbxx_impl/include/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_stusb/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Core/Inc)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/HID/Inc/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_stusb/STM32_USB_Device_Library/Class/HID/Inc/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/lib_stusb_impl/)
target_include_directories(io PUBLIC ${BOLOS_SDK}/)
