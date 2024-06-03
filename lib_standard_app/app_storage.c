/**
 * @file nvram_struct.c
 * @brief helpers to access NVRAM features
 *  TODO: put in SDK
 */
#ifdef HAVE_APP_NVRAM
#include <string.h>

#include "app_storage.h"
#include "os_nvm.h"
#include "os_pic.h"

const Nvram_t N_nvram_real;

/**
 * @brief init header of NVRAM structure :
 *  - set "NVRA" tag
 *  - set size
 *  - set struct and data versions
 *  - set properties
 * @param data_version Version of the data
 */
void nvram_init(uint32_t data_version)
{
    Nvram_header_t header;

    memcpy(header.tag, (void *) "NVRA", 4);
    // NVRAM_STRUCT_VERSION and NVRAM_DATA_PROPERTIES must be defined in nvram_data.h
    header.struct_version = NVRAM_STRUCT_VERSION;
    header.data_version   = data_version;
    header.properties     = NVRAM_DATA_PROPERTIES;
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
uint16_t nvram_get_struct_version(void)
{
    return N_nvram.header.struct_version;
}

/**
 * @brief get the version of app data
 */
uint32_t nvram_get_data_version(void)
{
    return N_nvram.header.data_version;
}

/**
 * @brief get the properties of app data
 */
uint16_t nvram_get_properties(void)
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

/**
 * @brief set data version of app data
 */
void nvram_set_data_version(uint32_t data_version)
{
    nvm_write((void *) &N_nvram.header.data_version, (void *) &data_version, sizeof(uint32_t));
}

#endif  // HAVE_APP_NVRAM
