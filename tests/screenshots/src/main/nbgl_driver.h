/**
 * @file nbgl_driver.h
 * @brief Low-Level driver API, to draw elementary forms
 *
 */

#ifndef NBGL_DRIVER_H
#define NBGL_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "bolos_target.h"
#include "nbgl_types.h"

/*********************
 *      DEFINES
 *********************/
/**
 * Width of the full screen in pixels, including side, front and unusable edge
 */
#if defined(TARGET_STAX)
#define FULL_SCREEN_WIDTH 496
#elif defined(TARGET_FLEX)
#define FULL_SCREEN_WIDTH 480
#elif defined(TARGET_APEX)
#define FULL_SCREEN_WIDTH 300
#elif defined(SCREEN_SIZE_NANO)
#define FULL_SCREEN_WIDTH 128
#endif  // TARGET_APEX

/**
 * @brief macro to get the color from color_map for the __code__ input
 * @param color_map u8 representing the color map
 * @param color input code (from @ref color_t)
 */
#define GET_COLOR_MAP(color_map, color) ((color_map >> (color * 2)) & 0x3)

/**
 * Value on 4BPP of WHITE
 *
 */
#define WHITE_4BPP 0xF

/**
 * Value on 4BPP of LIGHT_GRAY
 *
 */
#define LIGHT_GRAY_4BPP 0xA

/**
 * Value on 4BPP of DARK_GRAY
 *
 */
#define DARK_GRAY_4BPP 0x5

/**
 * Value on 4BPP of BLACK
 *
 */
#define BLACK_4BPP 0x0

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void nbgl_driver_drawRect(nbgl_area_t *area);
void nbgl_driver_drawLine(nbgl_area_t *area, uint8_t dotStartIdx, color_t lineColor);
void nbgl_driver_drawImage(nbgl_area_t          *area,
                           uint8_t              *buffer,
                           nbgl_transformation_t transformation,
                           nbgl_color_map_t      colorMap);
void nbgl_driver_drawImageFile(nbgl_area_t     *area,
                               uint8_t         *buffer,
                               nbgl_color_map_t colorMap,
                               uint8_t         *uzlib_chunk_buffer);
void nbgl_driver_drawImageRle(nbgl_area_t *area,
                              uint8_t     *buffer,
                              uint32_t     buffer_len,
                              color_t      fore_color,
                              uint8_t      nb_skipped_bytes);
void nbgl_driver_refreshArea(nbgl_area_t        *area,
                             nbgl_refresh_mode_t mode,
                             nbgl_post_refresh_t post_refresh);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_DRIVER_H */
