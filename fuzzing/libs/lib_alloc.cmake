include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_ALLOC_SOURCES "${BOLOS_SDK}/lib_alloc/mem_alloc.c")

add_library(alloc ${LIB_ALLOC_SOURCES})
target_link_libraries(alloc PUBLIC macros)
target_compile_options(alloc PUBLIC ${COMPILATION_FLAGS})
target_include_directories(alloc PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(alloc PUBLIC ${BOLOS_SDK}/lib_alloc/)
target_include_directories(alloc PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
