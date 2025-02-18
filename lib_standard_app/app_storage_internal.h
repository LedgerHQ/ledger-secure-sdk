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
#ifdef HAVE_APP_STORAGE

#pragma once

#include "app_storage.h"

/* The storage consists of the system and the app parts */
typedef struct __attribute__((packed)) app_storage_s {
    app_storage_header_t header;
    uint8_t              data[APP_STORAGE_SIZE];
} app_storage_t;

void app_storage_init(void);

#endif /* #ifdef HAVE_APP_STORAGE */
