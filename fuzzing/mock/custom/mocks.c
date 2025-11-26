
#include <string.h>
#include "usbd_core.h"

size_t strlcat(char *dst, const char *src, size_t size)
{
    char       *d = dst;
    const char *s = src;
    size_t      n = size;
    size_t      dsize;

    while (n-- != 0 && *d != '\0') {
        d++;
    }
    dsize = d - dst;
    n     = size - dsize;

    if (n == 0) {
        return (dsize + strlen(s));
    }

    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dsize + (s - src));
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t i = 0;

    if (size == 0) {
        // Can't copy anything or null-terminate
        while (src[i]) {
            i++;
        }
        return i;
    }

    for (i = 0; i < size - 1 && src[i]; i++) {
        dst[i] = src[i];
    }

    dst[i] = '\0';  // NUL-terminate
    while (src[i]) {
        i++;  // Count full length of src
    }

    return i;  // Return length of src
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev __attribute__((unused)),
                               uint8_t             ep_addr __attribute__((unused)))
{
    return 0;
}
