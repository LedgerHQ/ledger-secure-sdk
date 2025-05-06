include_guard()
set(DEFINES USB_SEGMENT_SIZE=64 IO_HID_EP_LENGTH=64 HAVE_ECC HAVE_HASH HAVE_RNG)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_MOCK_SOURCES "${BOLOS_SDK}/lib_cxng/src/cx_rng.c"
     "${BOLOS_SDK}/src/os.c" "${BOLOS_SDK}/fuzzing/mock/written/os_task.c"
     "${BOLOS_SDK}/fuzzing/mock/generated/syscalls_generated.c")

add_library(lib_mock ${LIB_MOCK_SOURCES})
target_include_directories(lib_mock PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(lib_mock PUBLIC ${BOLOS_SDK}/fuzzing/mock/written/)
target_include_directories(lib_mock
                           PUBLIC ${BOLOS_SDK}/../target/${TARGET_DEVICE}/)
target_include_directories(lib_mock
                           PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/)
target_include_directories(lib_mock PUBLIC ${BOLOS_SDK}/lib_cxng/include/)

# Add preprocessor definitions
target_compile_definitions(lib_mock PUBLIC ${DEFINES})
