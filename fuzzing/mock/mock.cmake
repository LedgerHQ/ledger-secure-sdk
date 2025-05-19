include_guard()

include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)

# Commented lines need os mock or depend of another function that needs a mock
file(GLOB LIB_MOCK_SOURCES 
     "${BOLOS_SDK}/src/os.c" 
     "${BOLOS_SDK}/src/ledger_assert.c"
     "${BOLOS_SDK}/src/cx_wrappers.c"
     "${BOLOS_SDK}/src/cx_hash_iovec.c"
     "${BOLOS_SDK}/fuzzing/mock/custom/os_task.c"
     "${BOLOS_SDK}/fuzzing/mock/custom/main.c"
     "${BOLOS_SDK}/fuzzing/mock/custom/pic.c"
     "${BOLOS_SDK}/fuzzing/mock/custom/custom_syscalls.c"
     "${BOLOS_SDK}/fuzzing/mock/generated/generated_syscalls.c"
)

add_library(mock ${LIB_MOCK_SOURCES})
target_link_libraries(mock PUBLIC macros cxng nbgl standard_app)
target_compile_options(mock PUBLIC ${COMPILATION_FLAGS})
target_include_directories(mock PUBLIC ${BOLOS_SDK}/lib_cxng/include/)
target_include_directories(mock PUBLIC ${BOLOS_SDK}/lib_standard_app/)
target_include_directories(mock PUBLIC ${BOLOS_SDK}/include/)
target_include_directories(mock PUBLIC ${BOLOS_SDK}/fuzzing/mock/written/)
target_include_directories(mock PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/)
target_include_directories(mock PUBLIC ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/)
