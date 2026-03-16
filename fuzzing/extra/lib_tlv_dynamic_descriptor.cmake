include_guard()

# Include required libraries
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_tlv.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_pki.cmake)

# fuzz_tlv_common_mocks.c overrides check_signature_with_pki to always succeed
# APPNAME must match the value in seed corpus TAG_APPLICATION_NAME
add_executable(fuzz_tlv_dynamic_descriptor
    ${BOLOS_SDK}/fuzzing/harness/fuzzer_tlv_dynamic_descriptor.c
    ${BOLOS_SDK}/fuzzing/harness/tlv_mutator.c
    ${BOLOS_SDK}/fuzzing/harness/fuzz_tlv_common_mocks.c)
target_compile_definitions(fuzz_tlv_dynamic_descriptor PRIVATE APPNAME="Fuzzing")
target_compile_options(fuzz_tlv_dynamic_descriptor PUBLIC ${COMPILATION_FLAGS})
target_link_options(fuzz_tlv_dynamic_descriptor PUBLIC ${LINK_FLAGS})

# Link with required libraries
target_link_libraries(fuzz_tlv_dynamic_descriptor PUBLIC standard_app mock tlv pki cxng nbgl_shared)
