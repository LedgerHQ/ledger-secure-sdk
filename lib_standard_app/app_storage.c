/*****************************************************************************
 *   (c) 2024-2025 Ledger SAS.
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
#include "os_nvm.h"
#include "os_pic.h"

/* The storage consists of the system and the app parts */
const uint8_t app_storage_real[sizeof(app_storage_header_t) + APP_STORAGE_SIZE]
    __attribute__((section(".storage_section")));
#define app_storage_pic ((volatile uint8_t *) PIC(&app_storage_real))

/**
 * @brief checks if the app storage struct is initialized
 */
static bool app_storage_is_initalized(void)
{
    if (memcmp((void *) &((app_storage_header_t *) app_storage_pic)->tag,
               APP_STORAGE_TAG,
               APP_STORAGE_TAG_LEN)) {
        return false;
    }
    if (((app_storage_header_t *) app_storage_pic)->size == 0) {
        return false;
    }
    return true;
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

    app_storage_header_t header = {0};

    // TODO: should we erase all the area ?
    // In any case erase must be page aligned for the length and on the app side we do not know the
    // page size nvm_erase((void *) app_storage_pic, sizeof(app_storage_header_t) +
    // APP_STORAGE_SIZE);

    memcpy(header.tag, (void *) APP_STORAGE_TAG, APP_STORAGE_TAG_LEN);
    // APP_STORAGE_DATA_STRUCT_VERSION and APP_STORAGE_PROPERTIES must be defined in
    // app_storage_data.h
    header.struct_version = APP_STORAGE_HEADER_STRUCT_VERSION;
    header.data_version   = 1;
    header.properties = ((HAVE_APP_STORAGE_PROP_SETTINGS << 0) | (HAVE_APP_STORAGE_PROP_DATA << 1));
    header.size       = 0;
    nvm_write((void *) app_storage_pic, &header, sizeof(header));
}

/**
 * @brief returns the size of app data
 */
uint32_t app_storage_get_size(void)
{
    return ((app_storage_header_t *) app_storage_pic)->size;
}

/**
 * @brief returns the version of app data
 */
uint32_t app_storage_get_data_version(void)
{
    return ((app_storage_header_t *) app_storage_pic)->data_version;
}

/**
 * @brief returns the properties of app data
 */
uint16_t app_storage_get_properties(void)
{
    return ((app_storage_header_t *) app_storage_pic)->properties;
}

/**
 * @brief increments by 1 the data_version field
 */
void app_storage_increment_data_version(void)
{
    uint32_t data_version = ((app_storage_header_t *) app_storage_pic)->data_version;
    data_version++;
    nvm_write((void *) &app_storage_pic[offsetof(app_storage_header_t, data_version)],
              (void *) &data_version,
              sizeof(data_version));
}

/**
 * @brief sets the data_version field
 */
void app_storage_set_data_version(uint32_t data_version)
{
    nvm_write((void *) &app_storage_pic[offsetof(app_storage_header_t, data_version)],
              (void *) &data_version,
              sizeof(((app_storage_header_t *) app_storage_pic)->tag));
}

/**
 * @brief writes application storage data with length and offset
 */
int32_t app_storage_pwrite(const void *buf, uint32_t nbyte, uint32_t offset)
{
    /* Input parameters verification */
    if (buf == NULL) {
        return -1;
    }

    uint32_t size = offset + nbyte;
    if (size >= APP_STORAGE_SIZE) {
        return -1;
    }

    /* Updating data */
    nvm_write(
        (void *) &app_storage_pic[sizeof(app_storage_header_t) + offset], (void *) buf, nbyte);

    /* Updating size if it increased */
    if (((app_storage_header_t *) app_storage_pic)->size < size) {
        nvm_write((void *) &app_storage_pic[offsetof(app_storage_header_t, size)],
                  (void *) &size,
                  sizeof(((app_storage_header_t *) app_storage_pic)->size));
    }
    return nbyte;
}

/**
 * @brief returns the base address of the application storage
 */
const void *app_storage_get(void)
{
    return (const void *) &app_storage_pic[sizeof(app_storage_header_t)];
}

#endif  // HAVE_APP_STORAGE
