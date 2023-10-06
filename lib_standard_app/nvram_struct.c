/**
 * @file nvram_struct.c
 * @brief helpers to access NVRAM features
 *  TODO: put in SDK
 */
#ifdef HAVE_APP_NVRAM
#include <string.h>
#include "nvram_struct.h"
#include "os_nvm.h"
#include "os_pic.h"

const Nvram_t N_nvram_real;

/**
 * @brief init header of NVRAM structure (set "NVRA" tag)
 */
void nvram_init(void)
{
    Nvram_header_t header;

    memcpy(header.tag, (void *) "NVRA", 4);
    // NVRAM_STRUCT_VERSION and NVRAM_DATA_VERSION must be defined in nvram_data.h
    header.struct_version = NVRAM_STRUCT_VERSION;
    header.data_version   = NVRAM_DATA_VERSION;
    header.size           = sizeof(Nvram_data_t);
    nvm_write((void *) &N_nvram.header, (void *) &header, sizeof(Nvram_header_t));
}

/**
 * @brief get the size of app data
 */
uint32_t nvram_get_size(void)
{
    return N_nvram.header.size;
    ;
}

/**
 * @brief get the version of app data structure
 */
uint8_t nvram_get_struct_version(void)
{
    return N_nvram.header.struct_version;
}

/**
 * @brief get the version of app data data
 */
uint8_t nvram_get_data_version(void)
{
    return N_nvram.header.data_version;
}

/**
 * @brief ensure NVRAM struct is initialized
 */
bool nvram_is_initalized(void)
{
    if (memcmp((const void *) N_nvram.header.tag, "NVRA", 4)) {
        return false;
    }
    if (N_nvram.header.size == 0) {
        return false;
    }
    return true;
}

#endif  // HAVE_APP_NVRAM
