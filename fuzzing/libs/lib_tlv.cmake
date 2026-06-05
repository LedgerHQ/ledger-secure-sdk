include_guard()
include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)

file(GLOB LIB_TLV_SOURCES
  "${BOLOS_SDK}/lib_tlv/*.c"
  "${BOLOS_SDK}/lib_tlv/use_cases/*.c")

add_library(tlv ${LIB_TLV_SOURCES})
target_compile_options(tlv PRIVATE ${COMPILATION_FLAGS})
target_link_libraries(tlv PUBLIC macros mock cxng)
target_include_directories(
  tlv
  PUBLIC "${BOLOS_SDK}/include/" "${BOLOS_SDK}/target/${TARGET}/include"
         "${BOLOS_SDK}/lib_tlv/" "${BOLOS_SDK}/lib_tlv/use_cases"
         "${BOLOS_SDK}/lib_cxng/include" "${BOLOS_SDK}/lib_pki")
