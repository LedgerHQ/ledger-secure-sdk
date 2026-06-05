include_guard()
cmake_minimum_required(VERSION 3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.14)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(
  SDKSelfFuzzer
  VERSION 1.0
  DESCRIPTION "SDK self-fuzz targets (Absolution)"
  LANGUAGES C)

if(NOT DEFINED BOLOS_SDK)
  message(FATAL_ERROR "BOLOS_SDK must be defined, CMake will exit.")
  return()
endif()

include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)
ledger_fuzz_setup()

# Mirror the surrounding build's sanitizer; otherwise Absolution defaults to fuzzer,address and clang rejects it against CFL's -fsanitize=memory matrix.
set(_SDK_FUZZ_SAN "${LEDGER_FUZZ_DEFAULT_SANITIZERS}")

# Per-target invariants are .gitignored and regenerated each build; bootstrap missing ones to `.{}` so Absolution can refine them on first configure.
foreach(_fuzzer
        fuzz_alloc fuzz_alloc_utils fuzz_apdu_parser fuzz_base58 fuzz_bip32
        fuzz_lists fuzz_nfc_ndef fuzz_qrcodegen fuzz_tlv_dynamic_descriptor
        fuzz_tlv_trusted_name)
  ledger_fuzz_bootstrap_file(
    "${CMAKE_SOURCE_DIR}/invariants/${_fuzzer}.zon"
    ""
    ".{}\n")
endforeach()

set(SDK_FUZZ_MOCK_SOURCES
    ${CMAKE_SOURCE_DIR}/mock/mocks.c
)

set(SDK_FUZZ_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/mock/
    ${CMAKE_SOURCE_DIR}/
    ${BOLOS_SDK}/lib_standard_app/
)

# TLV targets need a PKI stub and the custom mutator
set(TLV_EXTRA_SOURCES
    ${CMAKE_SOURCE_DIR}/mock/mock_check_signature_with_pki.c
    ${LEDGER_FUZZ_TLV_MUTATOR_SOURCE}
)

absolution_add_fuzzer(
    NAME fuzz_alloc
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_alloc.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_alloc.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_alloc_utils
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_alloc_utils.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_alloc_utils.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_lists
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_lists.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_lists.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_apdu_parser
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_apdu_parser.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_apdu_parser.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_base58
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_base58.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_base58.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_bip32
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_bip32.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_bip32.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_qrcodegen
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_qrcodegen.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_qrcodegen.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_tlv_dynamic_descriptor
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
        ${TLV_EXTRA_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_tlv_dynamic_descriptor.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_tlv_dynamic_descriptor.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES
        ${SDK_FUZZ_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/harness/
    COMPILE_DEFINITIONS APPNAME="Fuzzing"
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_tlv_trusted_name
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
        ${TLV_EXTRA_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_tlv_trusted_name.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_tlv_trusted_name.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES
        ${SDK_FUZZ_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/harness/
    COMPILE_DEFINITIONS APPNAME="Fuzzing"
    LINK_LIBRARIES secure_sdk
)

absolution_add_fuzzer(
    NAME fuzz_nfc_ndef
    TARGETS
        ${SDK_FUZZ_MOCK_SOURCES}
    HARNESS
        ${CMAKE_SOURCE_DIR}/harness/fuzz_nfc_ndef.c
    ENTRY fuzz_entry
    INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_nfc_ndef.zon
    SANITIZERS ${_SDK_FUZZ_SAN}
    INCLUDE_DIRECTORIES ${SDK_FUZZ_INCLUDE_DIRS}
    LINK_LIBRARIES secure_sdk
)
