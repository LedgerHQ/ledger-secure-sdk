/**
 * @file nbgl_obj_types.h
 * @brief Types of NBGL objects also used by the use-case API
 *
 */

#ifndef NBGL_OBJ_TYPES_H
#define NBGL_OBJ_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_types.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief Mode in which to open/set the keyboard
 *
 */
typedef enum {
#ifdef HAVE_SE_TOUCH
    MODE_LETTERS = 0,  ///< letters mode
    MODE_DIGITS,       ///< digits and some special characters mode
    MODE_SPECIAL       ///< extended special characters mode
#else                  // HAVE_SE_TOUCH
    MODE_LOWER_LETTERS,        ///< lower case letters mode
    MODE_UPPER_LETTERS,        ///< upper case letters mode
    MODE_DIGITS_AND_SPECIALS,  ///< digits and some special characters mode
    MODE_NONE                  ///< no mode defined (only for Nanos)
#endif                 // HAVE_SE_TOUCH
} keyboardMode_t;

/**
 * @brief Letters casing in which to open/set the keyboard
 *
 */
typedef enum {
    LOWER_CASE = 0,    ///< lower case mode
    UPPER_CASE,        ///< upper case mode for one character
    LOCKED_UPPER_CASE  ///< locked upper case mode
} keyboardCase_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_OBJ_TYPES_H */
