/**
 * @file fuzz_runtime.c
 * @brief Storage and weak defaults owned by the framework.
 *
 * @note fuzz_tail_ptr / fuzz_tail_len are the strong symbols that pull this
 * member out of the @c mock archive; the weak defaults below come along with
 * them. Weak symbols alone never force extraction, so keep them together.
 */

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#include "exceptions.h"
#include "os_task.h"
#include "main_std_app.h"  // NORETURN app_exit()

/* Not taken from fuzz_defs.h: that header is also compiled into Absolution's
 * object library, where the include path for exceptions.h is not guaranteed. */
extern try_context_t fuzz_exit_jump_ctx;

/** Harness input published by fuzz_harness_entry(). */
const uint8_t *fuzz_tail_ptr = NULL;
size_t         fuzz_tail_len = 0;

/**
 * @brief On device this comes from the linker script; a host build has none.
 *
 * Read by io_legacy/src/os_io_legacy.c under HAVE_BOLOS_APP_STACK_CANARY. Kept
 * out of the fuzzable prefix via invariants/sdk-zero-symbols.txt.
 */
uint32_t app_stack_canary;

/**
 * @brief BSS zeroing, neutralised: it would erase the state Absolution just
 *        restored from the prefix.
 */
void __wrap_os_explicit_zero_BSS_segment(void) {}

/**
 * @brief explicit_bzero routed to a volatile store loop.
 *
 * MSan cannot see through the SDK's version, so a SANITIZER=memory campaign
 * reports use-of-uninitialised-value on every buffer it zeroed
 * (google/sanitizers#1507). The barrier keeps the store from being elided.
 */
void *__wrap_explicit_bzero(void *dst, size_t len)
{
    if (dst != NULL && len != 0) {
        for (size_t i = 0; i < len; i++) {
            ((volatile unsigned char *) dst)[i] = 0;
        }
    }
    __asm__ volatile("" ::: "memory");
    return dst;
}

/**
 * @brief Default app exit: unwind to the harness instead of leaving the process.
 *
 * Pairs with the sigsetjmp in fuzz_harness_entry(). Terminating here would end
 * the campaign with status 0 and libFuzzer would report nothing.
 */
__attribute__((weak)) NORETURN void app_exit(void)
{
    siglongjmp(fuzz_exit_jump_ctx.jmp_buf, 1);
    /* siglongjmp does not return; this satisfies NORETURN if it ever did. */
    __builtin_unreachable();
}

/**
 * @brief No-op PRINTF, weak so an app can supply a real logger.
 *
 * macros/exclude_macros.txt strips the SDK's PRINTF macro, leaving the calls to
 * resolve here.
 */
__attribute__((weak)) int PRINTF(const char *format, ...)
{
    (void) format;
    return 0;
}

/**
 * @brief Weak defaults for the optional app callbacks.
 *
 * They cannot live in fuzz_harness.h: a weak definition in a header lands in the
 * same TU as an app's own, which is a redefinition error rather than weak
 * resolution.
 */
__attribute__((weak)) void fuzz_app_cleanup(void) {}

/** @brief Nothing to re-establish per iteration; most stateless targets want this. */
__attribute__((weak)) void fuzz_app_reset(void) {}
