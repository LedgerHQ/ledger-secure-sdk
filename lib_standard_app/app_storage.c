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
#include "os_nvm.h"
#include "os_pic.h"

// TODO: Create new section for the linker script
/*app_storage app_storage_real __attribute__((section(".bss.app_storage")));*/
const app_storage_t N_app_storage_real;

/**
 * @brief init header of application storage structure :
 *  - set "NVRA" tag
 *  - set size
 *  - set struct and data versions
 *  - set properties
 * @param data_version Version of the data
 */
void app_storage_init(uint32_t data_version)
{
    app_storage_header_t header = {0};

    memcpy(header.tag, (void *) "NVRA", 4);
    // APP_STORAGE_DATA_STRUCT_VERSION and APP_STORAGE_PROPERTIES must be defined in
    // app_storage_data.h
    header.struct_version = APP_STORAGE_DATA_STRUCT_VERSION;
    header.data_version   = data_version;
    header.properties     = APP_STORAGE_PROPERTIES;
    // TODO: Doing this lead to have app storage bigger than needed
    header.size = sizeof(app_storage_data_t);
    nvm_write((void *) &N_app_storage.header, &header, sizeof(header));
}

/**
 * @brief get the size of app data
 */
uint32_t app_storage_get_size(void)
{
    return N_app_storage.header.size;
}

/**
 * @brief get the version of app data structure
 */
uint16_t app_storage_get_struct_version(void)
{
    return N_app_storage.header.struct_version;
}

/**
 * @brief get the version of app data
 */
uint32_t app_storage_get_data_version(void)
{
    return N_app_storage.header.data_version;
}

/**
 * @brief get the properties of app data
 */
uint16_t app_storage_get_properties(void)
{
    return N_app_storage.header.properties;
}

/**
 * @brief ensure app storage struct is initialized
 */
bool app_storage_is_initalized(void)
{
    if (memcmp((void *) N_app_storage.header.tag, "NVRA", 4)) {
        return false;
    }
    if (N_app_storage.header.size == 0) {
        return false;
    }
    return true;
}

/**
 * @brief set data version of app data
 */
void app_storage_set_data_version(uint32_t data_version)
{
    nvm_write((void *) &N_app_storage.header.data_version,
              (void *) &data_version,
              sizeof(N_app_storage.header.data_version));
}

#endif  // HAVE_APP_STORAGE
