include_guard()

include(${BOLOS_SDK}/fuzzing/macros/macros.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_nbgl.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_standard_app.cmake)
include(${BOLOS_SDK}/fuzzing/libs/lib_cxng.cmake)

find_package(Python3 REQUIRED)

# --- Setup generation paths ---
set(GEN_SYSCALLS_DIR "${BOLOS_SDK}/fuzzing/mock/generated")
file(MAKE_DIRECTORY ${GEN_SYSCALLS_DIR})
set(GEN_SYSCALLS_OUTPUT "${GEN_SYSCALLS_DIR}/generated_syscalls.c")

# --- Custom command to generate syscall mocks ---
add_custom_command(
  OUTPUT ${GEN_SYSCALLS_OUTPUT}
  COMMAND ${Python3_EXECUTABLE} ${BOLOS_SDK}/fuzzing/mock/gen_mock.py
          ${BOLOS_SDK}/src/syscalls.c
          ${GEN_SYSCALLS_OUTPUT}
  COMMENT "Generating syscalls..."
  VERBATIM
)

# --- Custom target to group generation ---
add_custom_target(generate_syscalls_only DEPENDS ${GEN_SYSCALLS_OUTPUT})

# --- List mock sources (exclude GLOB on custom mocks) ---
file(GLOB CUSTOM_MOCK_SOURCES "${BOLOS_SDK}/fuzzing/mock/custom/*.c")

set(LIB_MOCK_SOURCES
  ${BOLOS_SDK}/src/os.c
  ${BOLOS_SDK}/src/ledger_assert.c
  ${BOLOS_SDK}/src/cx_wrappers.c
  ${BOLOS_SDK}/src/cx_hash_iovec.c
  ${CUSTOM_MOCK_SOURCES}
  ${GEN_SYSCALLS_OUTPUT}
)

# --- Add library and hook up dependencies ---
add_library(mock ${LIB_MOCK_SOURCES})
add_dependencies(mock generate_syscalls_only)

target_link_libraries(mock PUBLIC macros cxng nbgl standard_app)
target_compile_options(mock PUBLIC ${COMPILATION_FLAGS})
target_include_directories(
  mock
  PUBLIC ${BOLOS_SDK}/lib_cxng/include/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/include/
         ${BOLOS_SDK}/lib_standard_app/
         ${BOLOS_SDK}/include/
         ${BOLOS_SDK}/fuzzing/mock/written/
         ${BOLOS_SDK}/target/${TARGET_DEVICE}/)
