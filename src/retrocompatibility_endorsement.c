/**
 * @file retrocompatibility_endorsement.c
 * @brief Implementation of deprecated endorsement syscall wrappers.
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include "exceptions.h"
#include "os_types.h"
#include "os_endorsement.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FONCTIONS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Retrocompatibility version of `os_endorsement_get_code_hash`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_get_code_hash instead.
 */
unsigned int os_endorsement_get_code_hash(unsigned char *buffer PLENGTH(32))
{
    bolos_err_t error = ENDORSEMENT_get_code_hash((uint8_t *) buffer);

    if (error) {
        THROW(error);
    }

    return ENDORSEMENT_HASH_LENGTH;
}

/**
 * @brief Retrocompatibility version of `os_endorsement_get_public_key`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_get_public_key instead.
 */
unsigned int os_endorsement_get_public_key(unsigned char  index,
                                           unsigned char *buffer,
                                           unsigned char *length)
{
    return ENDORSEMENT_get_public_key((ENDORSEMENT_slot_t) index, buffer, length);
}

/**
 * @brief Retrocompatibility version of `os_endorsement_get_public_key_certificate`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_get_public_key_certificate instead.
 */
unsigned int os_endorsement_get_public_key_certificate(unsigned char  index,
                                                       unsigned char *buffer,
                                                       unsigned char *length)
{
    return ENDORSEMENT_get_public_key_certificate(index, buffer, length);
}

/**
 * @brief Retrocompatibility version of `os_endorsement_key1_get_app_secret`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_key1_get_app_secret instead.
 */
unsigned int os_endorsement_key1_get_app_secret(unsigned char *buffer)
{
    bolos_err_t error = ENDORSEMENT_key1_get_app_secret(buffer);

    if (error) {
        THROW(error);
    }

    return ENDORSEMENT_APP_SECRET_LENGTH;
}

/**
 * @brief Retrocompatibility version of `os_endorsement_key1_get_app_secret`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_key1_get_app_secret instead.
 */
unsigned int os_endorsement_key1_sign_data(unsigned char *src,
                                           unsigned int   srcLength,
                                           unsigned char *signature)
{
    uint32_t    out_signature_length = 0;
    bolos_err_t error
        = ENDORSEMENT_key1_sign_data(src, srcLength, signature, &out_signature_length);

    if (error) {
        THROW(error);
    }

    return out_signature_length;
}

/**
 * @brief Retrocompatibility version of `os_endorsement_key1_sign_without_code_hash`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_key1_sign_without_code_hash instead.
 */
unsigned int os_endorsement_key1_sign_without_code_hash(unsigned char *src,
                                                        unsigned int   srcLength,
                                                        unsigned char *signature)
{
    uint32_t    out_signature_length = 0;
    bolos_err_t error
        = ENDORSEMENT_key1_sign_without_code_hash(src, srcLength, signature, &out_signature_length);

    if (error) {
        THROW(error);
    }

    return out_signature_length;
}

/**
 * @brief Retrocompatibility version of `os_endorsement_key2_derive_sign_data`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_key2_derive_sign_data instead.
 */
unsigned int os_endorsement_key2_derive_sign_data(unsigned char *src,
                                                  unsigned int   srcLength,
                                                  unsigned char *signature)
{
    uint32_t    out_signature_length = 0;
    bolos_err_t error
        = ENDORSEMENT_key2_derive_sign_data(src, srcLength, signature, &out_signature_length);

    if (error) {
        THROW(error);
    }

    return out_signature_length;
}

/**
 * @brief Retrocompatibility version of `os_endorsement_get_metadata`.
 * @warning Deprecated function. Please use \ref ENDORSEMENT_get_metadata instead.
 */
unsigned int os_endorsement_get_metadata(unsigned char index, unsigned char *buffer)
{
    uint8_t     out_metadata_length = 0;
    bolos_err_t error
        = ENDORSEMENT_get_metadata((ENDORSEMENT_slot_t) index, buffer, &out_metadata_length);

    if (error) {
        THROW(error);
    }

    return out_metadata_length;
}
