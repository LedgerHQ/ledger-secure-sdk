
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
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
 ********************************************************************************/
/*
 * Internal hash function
 */
#ifndef CX_HASH_H
#define CX_HASH_H

#ifdef HAVE_HASH

#include "lcx_hash.h"
#include <stddef.h>

// #warning reduce MAX_HASH_SIZE and MAX_HASH_BLOCK_SIZE according to HAVE_xxx
#define MAX_HASH_SIZE       128
#define MAX_HASH_BLOCK_SIZE 128

/**
 * Destroy/clean the hash context.
 */
void cx_hash_destroy(cx_hash_t *hash_ctx);

#endif  // HAVE_HASH

#endif  // CX_HASH_H
