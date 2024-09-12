#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include "nbgl_fonts.h"
#include "ux_loc.h"
#include "os_task.h"

void fetch_language_packs(void);

const LANGUAGE_PACK *language_pack = NULL;

unsigned int os_sched_current_task(void)
{
    return TASK_BOLOS_UX;
}

void *pic(void *addr)
{
    return addr;
}

void fetch_language_packs(void)
{
    // If we are looking for a language pack:
    // - if the expected language is found then we'll use its begin/length range.
    // - else we'll use the built-in package and need to reset allowed MMU range.

    FILE *fptr = NULL;

#ifdef HAVE_SE_TOUCH
    fptr = fopen("../bolos_pack_fr_stax.bin", "rb");
#else   // HAVE_SE_TOUCH
    fptr = fopen("../bolos_pack_fr_nanos.bin", "rb");
#endif  // HAVE_SE_TOUCH

    assert_non_null(fptr);
    if (fptr != NULL) {
        fseek(fptr, 0, SEEK_END);

        uint32_t len = ftell(fptr);

        fseek(fptr, 0, SEEK_SET);

        uint8_t *source = (uint8_t *) malloc(len);

        assert_non_null(source);

        assert_int_equal(fread((unsigned char *) source, 1, len, fptr), len);

        fclose(fptr);

        language_pack = (LANGUAGE_PACK *) source;
        nbgl_refreshUnicodeFont(language_pack);
    }
}
