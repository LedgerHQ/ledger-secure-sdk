/**
 * @file nbgl_draw_text.h
 * @brief Middle Level API of the new BOLOS Graphical Library
 *
 */

#ifndef NBGL_DRAW_TEXT_H
#define NBGL_DRAW_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_types.h"
#include "nbgl_fonts.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

nbgl_font_id_e nbgl_drawText(const nbgl_area_t *area,
                             const char        *text,
                             uint16_t           textLen,
                             nbgl_font_id_e     fontId,
                             color_t            fontColor);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_DRAW_TEXT_H */
