include_guard()

include(${BOLOS_SDK}/fuzzing/v2/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/v2/libs/lib_cxng.cmake)

find_package(Python3 REQUIRED)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(Blue "${Esc}[34m")
  set(ColourReset "${Esc}[m")
endif()

set(GEN_SYSCALLS_DIR "${BOLOS_SDK}/fuzzing/v2/mock/_generated")
file(MAKE_DIRECTORY ${GEN_SYSCALLS_DIR})
set(GEN_SYSCALLS_OUTPUT "${GEN_SYSCALLS_DIR}/generated_syscalls.c")

add_custom_command(
  OUTPUT ${GEN_SYSCALLS_OUTPUT}
  COMMAND ${Python3_EXECUTABLE} ${BOLOS_SDK}/fuzzing/mock/gen_mock.py
          ${BOLOS_SDK}/src/syscalls.c ${GEN_SYSCALLS_OUTPUT}
  COMMENT "${Blue}Generating syscall mocks...${ColourReset}"
  VERBATIM)

add_custom_target(ledger_fuzz_generate_syscalls DEPENDS ${GEN_SYSCALLS_OUTPUT})

set(LIB_MOCK_SOURCES
    ${BOLOS_SDK}/fuzzing/v2/mock/cx/cx_bn_ec.c
    ${BOLOS_SDK}/fuzzing/v2/mock/cx/cx_crypto.c
    ${BOLOS_SDK}/fuzzing/v2/mock/nbgl/nbgl_runtime.c
    ${BOLOS_SDK}/fuzzing/v2/mock/nbgl/nbgl_use_case.c
    ${BOLOS_SDK}/fuzzing/v2/mock/os/os_exceptions.c
    ${BOLOS_SDK}/fuzzing/v2/mock/os/os_runtime.c
    ${BOLOS_SDK}/fuzzing/v2/mock/os/pic.c
    ${BOLOS_SDK}/src/os.c
    ${BOLOS_SDK}/src/ledger_assert.c
    ${BOLOS_SDK}/src/cx_wrappers.c
    ${GEN_SYSCALLS_OUTPUT})

add_library(ledger_fuzz_mock ${LIB_MOCK_SOURCES})
add_dependencies(ledger_fuzz_mock ledger_fuzz_generate_syscalls)

target_link_libraries(ledger_fuzz_mock PUBLIC ledger_fuzz_macros ledger_fuzz_cxng ledger_fuzz_nbgl ledger_fuzz_standard_app)
target_compile_options(ledger_fuzz_mock PRIVATE ${COMPILATION_FLAGS})
target_compile_options(ledger_fuzz_mock PUBLIC -Wno-pointer-to-int-cast)
target_include_directories(
  ledger_fuzz_mock
  PRIVATE "${BOLOS_SDK}/fuzzing/v2/mock/_generated"
          "${BOLOS_SDK}/target/${TARGET}")
