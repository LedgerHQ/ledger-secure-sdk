include_guard()

# Builds fuzz_alloc and fuzz_alloc_utils against the alloc/nbgl/io/mock
# libraries.
include(${BOLOS_SDK}/fuzzing/libs/lib_alloc.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_io.cmake)

add_executable(fuzz_alloc ${BOLOS_SDK}/fuzzing/harness/fuzzer_alloc.c)
target_compile_options(fuzz_alloc PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_alloc PUBLIC ${LINK_FLAGS})
target_link_libraries(
  fuzz_alloc PUBLIC alloc nbgl io mock)

add_executable(fuzz_alloc_utils ${BOLOS_SDK}/fuzzing/harness/fuzzer_alloc_utils.c)
target_compile_options(fuzz_alloc_utils PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_alloc_utils PUBLIC ${LINK_FLAGS})
target_link_libraries(
  fuzz_alloc_utils PUBLIC alloc nbgl io mock)
