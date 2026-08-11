set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT DEFINED LEDGER_HOST_CC)
    set(LEDGER_HOST_CC gcc)
endif()

if(NOT DEFINED LEDGER_HOST_32BIT)
    set(LEDGER_HOST_32BIT OFF)
endif()

set(CMAKE_C_COMPILER ${LEDGER_HOST_CC})

#######################################################################
#                              arch flags                             #
#######################################################################
if(LEDGER_HOST_32BIT)
    # Mimics the 32-bit pointer/size_t layout of the device.
    set(LEDGER_HOST_ARCH_FLAGS "-m32")
    string(APPEND CMAKE_C_FLAGS_INIT " ${LEDGER_HOST_ARCH_FLAGS}")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " ${LEDGER_HOST_ARCH_FLAGS}")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " ${LEDGER_HOST_ARCH_FLAGS}")
endif()

# Everything is built and run on the host: search paths stay native.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
