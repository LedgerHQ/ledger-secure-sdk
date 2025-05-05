include_guard()
set(DEFINES USB_SEGMENT_SIZE=64 IO_HID_EP_LENGTH=64 HAVE_ECC HAVE_HASH HAVE_RNG)

# Commented lines need os mock or depend of another function that needs a mock
file(
  GLOB
  LIB_MOCK_SOURCES
  "${CMAKE_SOURCE_DIR}/../lib_cxng/src/cx_rng.c"
  "${CMAKE_SOURCE_DIR}/../src/os.c"
  "${CMAKE_SOURCE_DIR}/mock/written/os_task.c"
  "${CMAKE_SOURCE_DIR}/mock/generated/syscalls_generated.c")

add_library(lib_mock ${LIB_MOCK_SOURCES})
target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/mock/written/)
target_include_directories(
  lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/../target/${TARGET_DEVICE}/)
target_include_directories(
  lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/../target/${TARGET_DEVICE}/include/)
target_include_directories(lib_mock
                           PUBLIC ${CMAKE_SOURCE_DIR}/../lib_cxng/include/)

# Add preprocessor definitions
target_compile_definitions(lib_mock PUBLIC ${DEFINES})
