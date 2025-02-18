/*****************************************************************************
 *   (c) 2024, 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#ifdef HAVE_APP_STORAGE
#include <string.h>
#include "app_storage.h"
#include "app_storage_internal.h"
#include "os_nvm.h"
#include "os_pic.h"

#define APP_STORAGE_ERASE_BLOCK_SIZE 256

/* In order to be used in unit testing */
#if !defined(TEST)
#define CONST const
#else
#define CONST
#endif

CONST app_storage_t app_storage_real __attribute__((section(".storage_section")));
#define as (*(volatile app_storage_t *) PIC(&app_storage_real))

/**
 * @brief checks if the app storage struct is initialized
 */
static bool app_storage_is_initalized(void)
{
    if (memcmp((const void *) &as.header.tag, APP_STORAGE_TAG, APP_STORAGE_TAG_LEN) != 0) {
        return false;
    }
    return true;
}

/**
 * @brief resets system header
 */
static inline void system_header_reset(void)
{
    /* Starting with zero application data size */
    app_storage_header_t header = {0};

    memcpy(&header.tag, (void *) APP_STORAGE_TAG, APP_STORAGE_TAG_LEN);
    header.struct_version = APP_STORAGE_HEADER_STRUCT_VERSION;
    header.data_version   = APP_STORAGE_INITIAL_APP_DATA_VERSION;
    header.properties = ((HAVE_APP_STORAGE_PROP_SETTINGS << 0) | (HAVE_APP_STORAGE_PROP_DATA << 1));
    nvm_write((void *) &as.header, &header, sizeof(header));
}

/**
 * @brief initializes the header of application storage structure:
 *  - checks if it already initialized, if not:
 *  - sets "NVRA" tag
 *  - sets initial size (0)
 *  - sets struct and data versions (1)
 *  - sets properties (from Makefile)
 */
void app_storage_init(void)
{
    if (app_storage_is_initalized()) {
        return;
    }
    system_header_reset();
}

/**
 * @brief resets and zeroes out application storage
 */
void app_storage_reset(void)
{
    system_header_reset();

    uint8_t erase_buf[APP_STORAGE_ERASE_BLOCK_SIZE] = {0};
    uint32_t offset = 0;
    if (APP_STORAGE_SIZE > APP_STORAGE_ERASE_BLOCK_SIZE) {
        for (; offset < APP_STORAGE_SIZE - APP_STORAGE_ERASE_BLOCK_SIZE; offset += APP_STORAGE_ERASE_BLOCK_SIZE) {
            nvm_write((void *) &as.data[offset], (void *) erase_buf, APP_STORAGE_ERASE_BLOCK_SIZE);
        }
    }
    if (APP_STORAGE_SIZE > offset) {
        nvm_write((void *) &as.data[offset], (void *) erase_buf, APP_STORAGE_SIZE - offset);
    }
}

/**
 * @brief returns the size of app data
 */
uint32_t app_storage_get_size(void)
{
    return as.header.size;
}

/**
 * @brief returns the version of app data
 */
uint32_t app_storage_get_data_version(void)
{
    return as.header.data_version;
}

/**
 * @brief returns the properties of app data
 */
uint16_t app_storage_get_properties(void)
{
    return as.header.properties;
}

/**
 * @brief increments by 1 the data_version field
 */
void app_storage_increment_data_version(void)
{
    uint32_t data_version = as.header.data_version;
    data_version++;
    if (data_version == 0) {
        /* wraparound  - TODO: what do we do in this case ? */
        data_version = APP_STORAGE_INITIAL_APP_DATA_VERSION;
    }
    nvm_write((void *) &as.header.data_version, (void *) &data_version, sizeof(data_version));
}

/**
 * @brief writes application data to the storage
 */
void app_storage_set_data_version(uint32_t data_version)
{
    nvm_write((void *) &as.header.data_version, (void *) &data_version, sizeof(data_version));
}

/**
 * @brief writes application storage data with length and offset
 */
int32_t app_storage_pwrite(const void *buf, uint32_t nbyte, uint32_t offset)
{
    /* Input parameters verification */
    if (buf == NULL) {
        return APP_STORAGE_EINVAL;
    }
    if (nbyte == 0) {
        return APP_STORAGE_EINVAL;
    }
    uint32_t max_offset = 0;
    if (__builtin_add_overflow(offset, nbyte, &max_offset)) {
        return APP_STORAGE_EINVAL;
    }

    /* Checking the app storage boundaries */
    if (max_offset > APP_STORAGE_SIZE) {
        return APP_STORAGE_EOVERFLOW;
    }

    /* Updating data */
    nvm_write((void *) &as.data[offset], (void *) buf, nbyte);

    /* Updating size if it increased */
    if (as.header.size < max_offset) {
        nvm_write((void *) &as.header.size, (void *) &max_offset, sizeof(max_offset));
    }
    return nbyte;
}

/**
 * @brief reads application data from the storage
 */
int32_t app_storage_pread(void *buf, uint32_t nbyte, uint32_t offset)
{
    /* Input parameters verification */
    if (buf == NULL) {
        return APP_STORAGE_EINVAL;
    }
    if (nbyte == 0) {
        return APP_STORAGE_EINVAL;
    }
    uint32_t max_offset = 0;
    if (__builtin_add_overflow(offset, nbyte, &max_offset)) {
        return APP_STORAGE_EINVAL;
    }

    /* Checking if the data has been already written */
    if (max_offset > as.header.size) {
        return APP_STORAGE_EADDRNOTAVAIL;
    }

    /* Reading data */
    memcpy((void *) buf, (void *) &as.data[offset], nbyte);

    return nbyte;
}

#endif  // HAVE_APP_STORAGE
