
/**
 * @file nbgl_front.c
 * @brief Low-Level driver for front screen, to draw elementary shapes
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_driver.h"
#include "nbgl_front.h"
#include "nbgl_debug.h"
#include "os_helpers.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Draws a plain rectangle with the given parameters
 *
 * @param area position, size and color of the rectangle to draw
 */
void nbgl_frontDrawRect(const nbgl_area_t *area)
{
    ((nbgl_area_t *) area)->x0 += FULL_SCREEN_WIDTH - SCREEN_WIDTH;
    nbgl_driver_drawRect((nbgl_area_t *) area);
    ((nbgl_area_t *) area)->x0 -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
}

/**
 * @brief Draws a horizontal line with the given parameters
 *
 * @note height is fixed to 4 pixels, and the mask provides the real thickness of the line
 *
 * @param area position, size and color of the line to draw
 * @param mask bit mask to tell which pixels (in vertical axis) are to be painted in lineColor.
 * bit[0] is the upper line on 4, and bit[3] is the bottom line
 * @param lineColor color to be applied to the line
 */
void nbgl_frontDrawHorizontalLine(const nbgl_area_t *area, uint8_t mask, color_t lineColor)
{
    ((nbgl_area_t *) area)->x0 += FULL_SCREEN_WIDTH - SCREEN_WIDTH;
    nbgl_driver_drawHorizontalLine((nbgl_area_t *) area, mask, lineColor);
    ((nbgl_area_t *) area)->x0 -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
}

/**
 * @brief Draws the given given bitmap with the given parameters. The colorMap is applied on 2BPP
 * image if not @ref INVALID_COLOR_MAP.
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in 1BPP
 * bitmap.
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 * @param transformation transformatin to be applied. This is a bit field, filled with @ref
 * VERTICAL_MIRROR for example
 * @param colorMap color map to be applied on given image, for 2 BPP image. For 1BPP, only the
 * color[0] is used, for foreground color
 */
void nbgl_frontDrawImage(const nbgl_area_t    *area,
                         const uint8_t        *buffer,
                         nbgl_transformation_t transformation,
                         nbgl_color_map_t      colorMap)
{
    ((nbgl_area_t *) area)->x0 += FULL_SCREEN_WIDTH - SCREEN_WIDTH;
    nbgl_driver_drawImage((nbgl_area_t *) area, (uint8_t *) buffer, transformation, colorMap);
    ((nbgl_area_t *) area)->x0 -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
}

/**
 * @brief Draws the given bitmap file with the given parameters. The colorMap is applied on 2BPP
 * image if not @ref INVALID_COLOR_MAP.
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in 1BPP
 * bitmap.
 *
 * @param area position and background color of the bitmap to draw
 * @param buffer bitmap file buffer, with ledger format
 * @param colorMap color map to be applied on given image, for 2 BPP image. For 1BPP, only the
 * color[0] is used, for foreground color
 * @param uzlib_chunk_buffer Work buffer of size GZLIB_UNCOMPRESSED_CHUNK
 */
void nbgl_frontDrawImageFile(const nbgl_area_t *area,
                             const uint8_t     *buffer,
                             nbgl_color_map_t   colorMap,
                             const uint8_t     *uzlib_chunk_buffer)
{
    UNUSED(uzlib_chunk_buffer);
    ((nbgl_area_t *) area)->x0 += FULL_SCREEN_WIDTH - SCREEN_WIDTH;
    nbgl_driver_drawImageFile(
        (nbgl_area_t *) area, (uint8_t *) buffer, colorMap, (uint8_t *) uzlib_chunk_buffer);
    ((nbgl_area_t *) area)->x0 -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
}
/**
 * @brief function to actually refresh the given area on screen
 *
 * @param area the area to refresh
 */
void nbgl_frontRefreshArea(const nbgl_area_t  *area,
                           nbgl_refresh_mode_t mode,
                           nbgl_post_refresh_t post_refresh)
{
    ((nbgl_area_t *) area)->x0 += FULL_SCREEN_WIDTH - SCREEN_WIDTH;
    nbgl_driver_refreshArea((nbgl_area_t *) area, mode, post_refresh);
    ((nbgl_area_t *) area)->x0 -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
}

void nbgl_frontDrawImageRle(const nbgl_area_t *area,
                            const uint8_t     *buffer,
                            uint32_t           buffer_len,
                            color_t            fore_color,
                            uint8_t            nb_skipped_bytes)
{
    ((nbgl_area_t *) area)->x0 += FULL_SCREEN_WIDTH - SCREEN_WIDTH;
    nbgl_driver_drawImageRle(
        (nbgl_area_t *) area, (uint8_t *) buffer, buffer_len, fore_color, nb_skipped_bytes);
    ((nbgl_area_t *) area)->x0 -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
}
