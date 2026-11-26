set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(LEDGER_SDK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

if(NOT DEFINED LEDGER_CLANG_CC)
    set(LEDGER_CLANG_CC clang)
endif()

# Clang has no arm-none-eabi headers of its own: borrow the GCC ones.
if(NOT DEFINED LEDGER_SYSROOT)
    execute_process(
        COMMAND arm-none-eabi-gcc -print-sysroot
        OUTPUT_VARIABLE LEDGER_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT LEDGER_SYSROOT)
        set(LEDGER_SYSROOT "/usr/lib/arm-none-eabi")
    endif()
endif()
set(CMAKE_SYSROOT ${LEDGER_SYSROOT})

set(CMAKE_C_COMPILER   ${LEDGER_CLANG_CC})
set(CMAKE_ASM_COMPILER ${LEDGER_CLANG_CC})
set(CMAKE_C_COMPILER_TARGET   arm-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET arm-none-eabi)

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy)
find_program(CMAKE_OBJDUMP arm-none-eabi-objdump)
find_program(CMAKE_SIZE    arm-none-eabi-size)

# The app link script and entry point are unknown here: probe with a static lib.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(LEDGER_ARCH_FLAGS "-mcpu=${LEDGER_TARGET_CPU} -mlittle-endian -mthumb")

# ROPI for code, RWPI for data: bolos apps are relocated at install time.
string(APPEND CMAKE_C_FLAGS_INIT
       " ${LEDGER_ARCH_FLAGS} ${LEDGER_FLOAT_FLAGS} -fropi -frwpi -mno-unaligned-access")

string(APPEND CMAKE_ASM_FLAGS_INIT " ${LEDGER_ARCH_FLAGS}")

# Emptied: DEBUG sets -O/-g on compile only, as make (none at link).
set(CMAKE_C_FLAGS_RELEASE "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG   "" CACHE STRING "" FORCE)

string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT
       " ${LEDGER_ARCH_FLAGS} -mno-unaligned-access -fuse-ld=lld -nostdlib -nodefaultlibs"
       " -L${LEDGER_SDK_ROOT}/arch/${LEDGER_ARCH_LIB_DIR}/lib/")

# Not provided by the Generic platform.
set(CMAKE_C_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--whole-archive" "<LINK_ITEM>" "LINKER:--no-whole-archive")
set(CMAKE_C_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

# Look for headers/libraries in the sysroot only; tools stay on the host.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
