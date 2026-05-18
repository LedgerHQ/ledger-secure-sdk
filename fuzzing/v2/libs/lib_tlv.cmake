include_guard()
include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_cxng.cmake)

file(GLOB LIB_TLV_SOURCES
  "${BOLOS_SDK}/lib_tlv/*.c"
  "${BOLOS_SDK}/lib_tlv/use_cases/*.c")

add_library(ledger_fuzz_tlv ${LIB_TLV_SOURCES})
target_compile_options(ledger_fuzz_tlv PRIVATE ${COMPILATION_FLAGS})
target_link_libraries(ledger_fuzz_tlv PUBLIC ledger_fuzz_macros ledger_fuzz_mock ledger_fuzz_cxng)
target_include_directories(
  ledger_fuzz_tlv
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_tlv/" "${BOLOS_SDK}/lib_tlv/use_cases"
         "${BOLOS_SDK}/lib_cxng/include" "${BOLOS_SDK}/lib_pki")
