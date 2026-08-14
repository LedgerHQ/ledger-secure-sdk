include_guard()

include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)

find_package(Python3 REQUIRED)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(Blue "${Esc}[34m")
  set(ColourReset "${Esc}[m")
endif()

# Generated into the build tree, not the shared SDK source tree: otherwise
# concurrent builds race on one path and a read-only SDK checkout cannot
# configure.
set(GEN_SYSCALLS_DIR "${CMAKE_CURRENT_BINARY_DIR}/_generated")
file(MAKE_DIRECTORY ${GEN_SYSCALLS_DIR})
set(GEN_SYSCALLS_OUTPUT "${GEN_SYSCALLS_DIR}/generated_syscalls.c")

add_custom_command(
  OUTPUT ${GEN_SYSCALLS_OUTPUT}
  COMMAND ${Python3_EXECUTABLE} ${BOLOS_SDK}/fuzzing/mock/gen_mock.py
          ${BOLOS_SDK}/src/syscalls.c ${GEN_SYSCALLS_OUTPUT}
  DEPENDS ${BOLOS_SDK}/src/syscalls.c ${BOLOS_SDK}/fuzzing/mock/gen_mock.py
  COMMENT "${Blue}Generating syscall mocks...${ColourReset}"
  VERBATIM)

add_custom_target(generate_syscalls DEPENDS ${GEN_SYSCALLS_OUTPUT})

set(LIB_MOCK_SOURCES
    ${BOLOS_SDK}/fuzzing/mock/cx/cx_bn_ec.c
    ${BOLOS_SDK}/fuzzing/mock/cx/cx_crypto.c
    ${BOLOS_SDK}/fuzzing/mock/nbgl/nbgl_runtime.c
    ${BOLOS_SDK}/fuzzing/mock/nbgl/nbgl_use_case.c
    ${BOLOS_SDK}/fuzzing/mock/os/os_exceptions.c
    ${BOLOS_SDK}/fuzzing/mock/os/os_runtime.c
    ${BOLOS_SDK}/fuzzing/mock/fuzz_mutator.c
    ${BOLOS_SDK}/fuzzing/mock/fuzz_runtime.c
    ${BOLOS_SDK}/fuzzing/mock/pki/ledger_pki_policy.c
    ${BOLOS_SDK}/src/os.c
    ${BOLOS_SDK}/src/ledger_assert.c
    ${BOLOS_SDK}/src/cx_wrappers.c
    ${GEN_SYSCALLS_OUTPUT})

add_library(mock STATIC ${LIB_MOCK_SOURCES})
add_dependencies(mock generate_syscalls)

target_link_libraries(mock PUBLIC macros cxng nbgl standard_app)
target_compile_options(mock PRIVATE ${COMPILATION_FLAGS})
target_compile_options(mock PRIVATE -Wno-pointer-to-int-cast)
target_include_directories(
  mock
  PRIVATE "${BOLOS_SDK}/fuzzing/mock/_generated"
          # fuzz_mutator.c implements the header the harnesses include.
          "${BOLOS_SDK}/fuzzing/include"
          "${BOLOS_SDK}/target/${TARGET}"
          # pki/ledger_pki_policy.c needs ledger_pki.h; lib_tlv comes with it.
          "${BOLOS_SDK}/lib_pki"
          "${BOLOS_SDK}/lib_tlv")
