/*****************************************************************************
 *   (c) 2024 Ledger SAS.
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
#include "lcx_crc.h"
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
#define app_storage (*(volatile app_storage_t *) PIC(&app_storage_real))

/**
 * @brief checks if the app storage struct is initialized
 */
static bool app_storage_is_initalized(void)
{
    if (memcmp((const void *) &app_storage.header.tag, APP_STORAGE_TAG, APP_STORAGE_TAG_LEN) != 0) {
        return false;
    }
    return true;
}

static inline void update_crc(void)
{
    uint32_t crc = cx_crc32((void *) &app_storage.header,
                            sizeof(app_storage.header) + app_storage.header.size);
    nvm_write((void *) &app_storage.crc, &crc, sizeof(app_storage.crc));
}

/**
 * @brief resets system header
 */
static inline void system_header_reset(void)
{
    /* Starting with zero application data size */
    app_storage_header_t header = {0};

    memcpy(&header.tag, (void *) APP_STORAGE_TAG, APP_STORAGE_TAG_LEN);
    /* Setting up current system structure version */
    header.struct_version = APP_STORAGE_HEADER_STRUCT_VERSION;
    /* Setting up initial application datat version */
    header.data_version = APP_STORAGE_INITIAL_APP_DATA_VERSION;
    /* Setting up properties fields from the app makefile */
    header.properties = ((HAVE_APP_STORAGE_PROP_SETTINGS << 0) | (HAVE_APP_STORAGE_PROP_DATA << 1));
    nvm_write((void *) &app_storage.header, &header, sizeof(header));
    update_crc();
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

    uint8_t  erase_buf[APP_STORAGE_ERASE_BLOCK_SIZE] = {0};
    uint32_t offset                                  = 0;
    bool     need_update                             = false;

    if (APP_STORAGE_SIZE > APP_STORAGE_ERASE_BLOCK_SIZE) {
        for (; offset < APP_STORAGE_SIZE - APP_STORAGE_ERASE_BLOCK_SIZE;
             offset += APP_STORAGE_ERASE_BLOCK_SIZE) {
            nvm_write((void *) &app_storage.data[offset],
                      (void *) erase_buf,
                      APP_STORAGE_ERASE_BLOCK_SIZE);
        }
        need_update = true;
    }
    if (APP_STORAGE_SIZE > offset) {
        nvm_write(
            (void *) &app_storage.data[offset], (void *) erase_buf, APP_STORAGE_SIZE - offset);
        need_update = true;
    }
    if (need_update) {
        update_crc();
    }
}

/**
 * @brief returns the size of app data
 */
uint32_t app_storage_get_size(void)
{
    return app_storage.header.size;
}

/**
 * @brief returns the version of app data
 */
uint32_t app_storage_get_data_version(void)
{
    return app_storage.header.data_version;
}

/**
 * @brief returns the properties of app data
 */
uint16_t app_storage_get_properties(void)
{
    return app_storage.header.properties;
}

/**
 * @brief increments by 1 the data_version field
 */
void app_storage_increment_data_version(void)
{
    uint32_t data_version = app_storage.header.data_version;
    data_version++;
    if (data_version == 0) {
        /* wraparound  - we consider we'll never achieve this number,
           knowing that the the maximum number of Flash write cycles
           may be around 100 000.
         */
        data_version = APP_STORAGE_INITIAL_APP_DATA_VERSION;
    }
    nvm_write(
        (void *) &app_storage.header.data_version, (void *) &data_version, sizeof(data_version));
    update_crc();
}

/**
 * @brief writes application data to the storage
 */
void app_storage_set_data_version(uint32_t data_version)
{
    nvm_write(
        (void *) &app_storage.header.data_version, (void *) &data_version, sizeof(data_version));
    update_crc();
}

/**
 * @brief writes application storage data with length and offset
 */
int32_t app_storage_write(const void *buf, uint32_t nbyte, uint32_t offset)
{
    /* Input parameters verification */
    if (buf == NULL) {
        return APP_STORAGE_ERR_INVALID_ARGUMENT;
    }
    if (nbyte == 0) {
        return APP_STORAGE_ERR_INVALID_ARGUMENT;
    }
    uint32_t max_offset = 0;
    if (__builtin_add_overflow(offset, nbyte, &max_offset)) {
        return APP_STORAGE_ERR_INVALID_ARGUMENT;
    }

    /* Checking the app storage boundaries */
    if (max_offset > APP_STORAGE_SIZE) {
        return APP_STORAGE_ERR_OVERFLOW;
    }

    /* Updating data */
    nvm_write((void *) &app_storage.data[offset], (void *) buf, nbyte);

    /* Updating size if it increased */
    if (app_storage.header.size < max_offset) {
        nvm_write((void *) &app_storage.header.size, (void *) &max_offset, sizeof(max_offset));
    }
    update_crc();

    return nbyte;
}

/**
 * @brief reads application data from the storage
 */
int32_t app_storage_read(void *buf, uint32_t nbyte, uint32_t offset)
{
    /* Input parameters verification */
    if (buf == NULL) {
        return APP_STORAGE_ERR_INVALID_ARGUMENT;
    }
    if (nbyte == 0) {
        return APP_STORAGE_ERR_INVALID_ARGUMENT;
    }
    uint32_t max_offset = 0;
    if (__builtin_add_overflow(offset, nbyte, &max_offset)) {
        return APP_STORAGE_ERR_INVALID_ARGUMENT;
    }

    /* Checking if the data has been already written */
    if (max_offset > app_storage.header.size) {
        return APP_STORAGE_ERR_NO_DATA_AVAILABLE;
    }

    /* Reading data */
    memcpy((void *) buf, (void *) &app_storage.data[offset], nbyte);

    return nbyte;
}

#endif  // HAVE_APP_STORAGE
