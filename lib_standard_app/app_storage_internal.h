/*****************************************************************************
 *   (c) 2025 Ledger SAS.
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
#if defined(HAVE_APP_STORAGE) || defined(HAVE_BOLOS)

#pragma once

#include "app_storage.h"

/**
 * @brief The storage consists of the system and the app parts
 */
typedef struct app_storage_s {
    uint32_t             crc;  ///< CRC32 of the app storage header and data
    app_storage_header_t header;
    uint8_t              data[APP_STORAGE_SIZE];
} app_storage_t;

/**
 * @brief initializes the application storage.
 *
 * @returns int32_t Initialization status.
 *
 * @retval APP_STORAGE_SUCCESS The storage was found valid.
 * @retval APP_STORAGE_ERR_INVALID_HEADER Invalid tag, CRC and rest of the header zero: most likely
 *         a first start. The storage is reinitialized.
 * @retval APP_STORAGE_ERR_CORRUPTED Invalid tag, size or CRC: the storage existed and is damaged.
 *         The storage is reinitialized, any data it held is lost.
 *
 */
int32_t app_storage_init(void);

#endif /* HAVE_APP_STORAGE || HAVE_BOLOS */
