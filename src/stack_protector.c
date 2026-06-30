// These macros are defined by clang and gcc when -fstack-protector arguments
// are passed to the compiler.
#if defined(__SSP__) || defined(__SSP_ALL__) || defined(__SSP_STRONG__) || defined(__SSP_EXPLICIT__)

#include "os_task.h"

// used and retain attributes to avoid LTO removing it
__attribute__((noreturn, used, retain)) void __wrap___stack_chk_fail(void)
{
    os_sched_exit(37);
}

// in case of LTO build, __stack_chk_fail gets optimized out before it is redirected to
// __wrap___stack_chk_fail, so it needs to be defined.
#if defined(HAS_LTO)
__attribute__((noreturn, used, retain, weak)) void __stack_chk_fail(void)
{
    os_sched_exit(37);
}
#endif

#endif
