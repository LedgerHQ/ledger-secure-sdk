/* Exception and NVM mocks.
 * os_sched_exit and os_lib_end longjmp back to the fuzz harness.
 */

#include <setjmp.h>
#include <string.h>

#include "exceptions.h"
#include "os_task.h"

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

void __attribute__((noreturn)) os_sched_exit(bolos_task_status_t exit_code __attribute__((unused)))
{
    longjmp(fuzz_exit_jump_ctx.jmp_buf, 1);
}

void __attribute__((noreturn)) os_lib_end(void)
{
    longjmp(fuzz_exit_jump_ctx.jmp_buf, 1);
}

void nvm_write(void *dst_adr, void *src_adr, unsigned int src_len)
{
    if (dst_adr == NULL || src_len == 0) {
        return;
    }

    if (src_adr == NULL) {
        memset(dst_adr, 0, src_len);
    }
    else {
        memcpy(dst_adr, src_adr, src_len);
    }
}
