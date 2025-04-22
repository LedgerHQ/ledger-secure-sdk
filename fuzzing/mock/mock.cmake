include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_MOCK_SOURCES
    "${CMAKE_SOURCE_DIR}/mock/exceptions.c"
    "${CMAKE_SOURCE_DIR}/mock/os_task.c"
)

add_library(lib_mock ${LIB_MOCK_SOURCES})
target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_mock PUBLIC ${CMAKE_SOURCE_DIR}/)