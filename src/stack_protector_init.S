#if defined(__SSP__) || defined(__SSP_ALL__) || defined(__SSP_STRONG__) || defined(__SSP_EXPLICIT__)

#include "syscalls.h"

.syntax unified
.thumb

.section .boot.ssp_init, "ax"

.global __wrap___stack_chk_init
.thumb_func
__wrap___stack_chk_init:
    // if r0 != 0, skip initialization and jump directly to main
    // (don't overwrite parent canary during a libcall)
    cmp r0, #0
    bne 1f

    // save arguments passed to main
    push {r0-r3}

    // call cx_get_random_bytes(&__stack_chk_guard, sizeof(__stack_chk_guard));
    // we can't use the function cx_get_random_bytes because of PIC
    ldr r0, =SYSCALL_cx_get_random_bytes_ID
    mov r2, r9
    movs r3, #4
    push {r2-r3}
    mov r1, sp
    bl SVC_Call
    pop {r2-r3}

    // restore arguments
    pop {r0-r3}

    // jump to the main function
    b 1f

// skip the constants pool
.ltorg

1:

#else

// Force the following linker error if -fstack-protector is enabled for CFLAGS
// but not for AFLAGS:
//
//     ld.lld: error: duplicate symbol: __stack_chk_fail
//
// It ensures that __stack_chk_init is always defined (and consequently
// __stack_chk_guard is always initialized to a random value) when SSP is
// enabled.
//
// This code will never end up in an app since the link step will fail.
.global __stack_chk_fail
.thumb_func
__stack_chk_fail:
    udf 255

#endif
