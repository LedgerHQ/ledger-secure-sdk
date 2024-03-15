/**
 * @file nvram_struct.h
 * @brief All definitions of the common part of NVRAM structure, and helpers to access it
 *  TODO: put in SDK
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "os_pic.h"

/* "nvram_data.h" needs to be created in all apps including this file */
#include "nvram_data.h"

/**
 * @brief Structure defining the header of NVRAM
 *
 */
typedef struct Nvram_header_s {
    char     tag[4];          ///< ['N','V','R','A'] array, when properly initialized
    uint32_t size;            ///< size in bytes of the data
    uint32_t struct_version;  ///< version of the structure of data (to be set once at first
                              ///< application start-up)
    uint32_t data_version;    ///< version of the content of data (to be updated every time data are
                              ///< updated)
} Nvram_header_t;

/**
 * @brief Structure defining the NVRAM
 *
 */
typedef struct Nvram_s {
    Nvram_header_t header;  ///< header describing the data
    Nvram_data_t   data;  ///< application data, Nvram_data_t must be defined in "nvram_data.h" file
} Nvram_t;

/**
 * @brief This variable must be defined in Application code, and never used directly,
 * except by @ref N_nvram
 *
 */
extern const Nvram_t N_nvram_real;

/**
 * @brief To be used by function accessing NVRAM data (not N_nvram_real)
 *
 */
#define N_nvram (*(volatile Nvram_t *) PIC(&N_nvram_real))

extern void     nvram_init(void);
extern uint32_t nvram_get_size(void);
extern uint8_t  nvram_get_struct_version(void);
extern uint8_t  nvram_get_data_version(void);
extern bool     nvram_is_initalized(void);
