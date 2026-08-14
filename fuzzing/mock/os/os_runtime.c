/* OS and libc runtime mocks. */

#include <stdint.h>
#include <string.h>

#include "exceptions.h"
#include "io.h"
#include "nbgl_types.h"
#include "os.h"
#include "usbd_core.h"

bolos_ux_params_t G_ux_params;

struct {
    uint8_t bss[0xff];
    uint8_t ebss[0xff];
} mock_memory;

void *_bss  = &mock_memory.bss;
void *_ebss = &mock_memory.ebss;

size_t strlcat(char *dst, const char *src, size_t size)
{
    char       *d = dst;
    const char *s = src;
    size_t      n = size;

    while (n != 0 && *d != '\0') {
        d++;
        n--;
    }

    size_t d_len = (size_t) (d - dst);
    if (n == 0) {
        return d_len + strlen(s);
    }

    while (*s != '\0') {
        if (n > 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return d_len + (size_t) (s - src);
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t i = 0;

    if (size != 0) {
        while (i + 1 < size && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
        dst[i] = '\0';
    }

    while (src[i] != '\0') {
        i++;
    }

    return i;
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    (void) pdev;
    (void) ep_addr;
    return 0;
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
