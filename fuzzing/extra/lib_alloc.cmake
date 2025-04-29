include_guard()

add_library(lib_mock ${CMAKE_CURRENT_SOURCE_DIR}/mock/os_task.c)
add_library(lib_alloc ${CMAKE_CURRENT_SOURCE_DIR}/../lib_alloc/mem_alloc.c
)

set_target_properties(lib_alloc PROPERTIES SOVERSION 1)

target_include_directories(lib_mock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mock/)
target_include_directories(lib_alloc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib_alloc
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/
    ${CMAKE_CURRENT_SOURCE_DIR}/../target/${TARGET_DEVICE}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib_standard_app
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Define the executable and its properties here
add_executable(fuzz_alloc ${CMAKE_CURRENT_SOURCE_DIR}/fuzzer_alloc.c)
target_compile_options(fuzz_alloc PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_alloc PUBLIC ${COMPILATION_FLAGS})
target_link_libraries(fuzz_alloc PUBLIC lib_alloc lib_mock)
