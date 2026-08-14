/* Exception mocks: os_sched_exit and os_lib_end longjmp back to the fuzz harness. */

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "exceptions.h"
#include "os_task.h"

static void __attribute__((noreturn)) fuzz_report_fatal_exit(unsigned int exit_code)
{
    const char *what
        = (exit_code == 37) ? "BOLOS stack canary overwritten" : "LEDGER_ASSERT failed";
    fprintf(stderr,
            "==fuzz== FATAL: app called os_sched_exit(%u): %s\n"
            "==fuzz== This is an app invariant the fuzzer broke, not an ordinary exit.\n",
            exit_code,
            what);
    fflush(stderr);
    abort();
}

try_context_t  fuzz_exit_jump_ctx  = {0};
try_context_t *G_exception_context = &fuzz_exit_jump_ctx;

try_context_t *try_context_get(void)
{
    return G_exception_context;
}

try_context_t *try_context_set(try_context_t *context)
{
    try_context_t *previous = G_exception_context;
    G_exception_context     = context;
    return previous;
}

/* 255 is ledger_assert.c's os_sched_exit(-1); 37 is the stack canary. Both mean a
 * broken app invariant, so abort rather than unwind. See mocks.dox. */
#define FUZZ_EXIT_ASSERT_FAILED 255
#define FUZZ_EXIT_STACK_CANARY  37

void __attribute__((noreturn)) os_sched_exit(bolos_task_status_t exit_code)
{
    if (exit_code == FUZZ_EXIT_ASSERT_FAILED || exit_code == FUZZ_EXIT_STACK_CANARY) {
        fuzz_report_fatal_exit(exit_code);
    }

    longjmp(fuzz_exit_jump_ctx.jmp_buf, 1);
}

void __attribute__((noreturn)) os_lib_end(void)
{
    longjmp(fuzz_exit_jump_ctx.jmp_buf, 1);
}
