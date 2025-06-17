
#include <string.h>
#include "usbd_core.h"

size_t strlcpy(char *dst, const char *src, size_t size) {
    size_t i = 0;

    if (size == 0) {
        // Can't copy anything or null-terminate
        while (src[i]) i++;
        return i;
    }

    for (i = 0; i < size - 1 && src[i]; i++) {
        dst[i] = src[i];
    }

    dst[i] = '\0';  // NUL-terminate
    while (src[i]) i++;  // Count full length of src

    return i;  // Return length of src
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr){
    return 0;
}
