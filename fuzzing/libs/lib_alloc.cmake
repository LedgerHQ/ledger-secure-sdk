include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_ALLOC_SOURCES
    "${CMAKE_SOURCE_DIR}/../lib_alloc/mem_alloc.c"
)

add_library(lib_alloc ${LIB_ALLOC_SOURCES})
target_include_directories(lib_alloc PUBLIC ${CMAKE_SOURCE_DIR}/../include/)
target_include_directories(lib_alloc PUBLIC ${CMAKE_SOURCE_DIR}/../lib_alloc/)
target_include_directories(lib_alloc PUBLIC ${CMAKE_SOURCE_DIR}/../target/${TARGET_DEVICE}/include)
