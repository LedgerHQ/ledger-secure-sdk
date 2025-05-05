include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_MOCK_SOURCES
    "${CMAKE_SOURCE_DIR}/mock/written/os_task.c"
    #"${CMAKE_SOURCE_DIR}/mock/generated/syscalls_generated.c"
)

add_library(lib_mock ${LIB_MOCK_SOURCES})

#target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/../${TARGET_DEVICE}/include/)
target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/mock/written/)

