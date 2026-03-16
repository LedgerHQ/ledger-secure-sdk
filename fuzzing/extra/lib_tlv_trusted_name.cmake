include_guard()

# Include required libraries
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_tlv.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_pki.cmake)

# fuzz_tlv_common_mocks.c overrides the weak check_signature_with_pki to always succeed
add_executable(fuzz_tlv_trusted_name
    ${BOLOS_SDK}/fuzzing/harness/fuzzer_tlv_trusted_name.c
    ${BOLOS_SDK}/fuzzing/harness/tlv_mutator.c
    ${BOLOS_SDK}/fuzzing/harness/fuzz_tlv_common_mocks.c)
target_compile_options(fuzz_tlv_trusted_name PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_tlv_trusted_name PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_tlv_trusted_name PUBLIC standard_app mock tlv pki cxng nbgl_shared)
