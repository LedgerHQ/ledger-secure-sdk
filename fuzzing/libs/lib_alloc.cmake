include_guard()

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_ALLOC_SOURCES "${BOLOS_SDK}/lib_alloc/mem_alloc.c")

add_library(lib_alloc ${LIB_ALLOC_SOURCES})
target_include_directories(lib_alloc PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(lib_alloc PUBLIC ${BOLOS_SDK}/lib_alloc/)
target_include_directories(lib_alloc
                           PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include)
