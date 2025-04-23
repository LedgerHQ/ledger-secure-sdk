
/**
 * @file nbgl_low.c
 * @brief Low-Level driver, to draw elementary forms
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "bolos_target.h"
#include "nbgl_driver.h"
#include "nbgl_debug.h"
#include "nbgl_image_utils.h"
#include "uzlib.h"
#include "os_helpers.h"
#include "os_screen.h"
#include "json_scenario.h"

/*********************
 *      DEFINES
 *********************/
#define NB_LAST_AREAS 32

/**********************
 *      TYPEDEFS
 **********************/
typedef struct KeepOutArea_s {
    uint16_t x_start;
    uint16_t x_end;
    uint16_t y_start;
    uint16_t y_end;
} KeepOutArea_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#ifdef SCREEN_SIZE_NANO
static KeepOutArea_t keepOutArea;
#endif
static nbgl_area_t lastAreas[NB_LAST_AREAS];
static uint32_t    lastAreaIndex = 0, nbValidAreas = 0;

/**********************
 *      MACROS
 **********************/
#ifdef SCREEN_SIZE_WALLET
#define CHECK_PARAMS()
#define CHECK_PARAMS_ROTATED()                                                      \
    {                                                                               \
        if (area->width & 0x3) {                                                    \
            LOG_FATAL(LOW_LOGGER, "%s: Bad width %d\n", __FUNCTION__, area->width); \
        }                                                                           \
    }

#define IS_NOT_KEEP_OUT(x, y) (true)
#define IS_IN_SCREEN(x, y)              \
    (((y * FULL_SCREEN_WIDTH + x) >= 0) \
     && ((y * FULL_SCREEN_WIDTH + x) < (FULL_SCREEN_WIDTH * SCREEN_HEIGHT)))
#else  // SCREEN_SIZE_WALLET
#define CHECK_PARAMS()
#define CHECK_PARAMS_ROTATED()

// ensure the pixel is not in the keepout area, and not above height limit
#define IS_NOT_KEEP_OUT(x, y)                                                            \
    (((x < keepOutArea.x_start) || (x >= keepOutArea.x_end) || (y < keepOutArea.y_start) \
      || (y >= keepOutArea.y_end)))

#define IS_IN_SCREEN(x, y)              \
    (((y * FULL_SCREEN_WIDTH + x) >= 0) \
     && ((y * FULL_SCREEN_WIDTH + x) < (FULL_SCREEN_WIDTH * SCREEN_HEIGHT)))

#endif  // SCREEN_SIZE_WALLET

// expands a 2BPP color into a 4BPP color
#define EXPAND_TO_4BPP(__color2bpp) (((__color2bpp) << 2) | (__color2bpp))

/**********************
 *      VARIABLES
 **********************/
static uint8_t framebuffer[FULL_SCREEN_WIDTH * SCREEN_HEIGHT];
static color_t backgroundColor;

/**********************
 *  STATIC FUNCTIONS
 **********************/

static color_t getBackgroundColor(int16_t x, int16_t y)
{
    uint32_t i = 0;
    while (i < nbValidAreas) {
        nbgl_area_t *paintedArea;

        // lastAreas is a circular buffer and we are going from last to first
        if (i < lastAreaIndex) {
            paintedArea = &lastAreas[lastAreaIndex - i - 1];
        }
        else {
            paintedArea = &lastAreas[NB_LAST_AREAS + lastAreaIndex - i - 1];
        }
        // Check if given area top-left position belongs to last painted area
        if ((x >= paintedArea->x0) && (x < (paintedArea->x0 + paintedArea->width))
            && (y >= paintedArea->y0) && (y < (paintedArea->y0 + paintedArea->height))) {
            return paintedArea->backgroundColor;
        }
        i++;
    }
    return backgroundColor;
}

static void draw_alignedBackground(nbgl_area_t *area)
{
    color_t curBackgroundColorTop, curBackgroundColorBottom;
    int16_t x, y, y0, y1;

    y0 = NBGL_LOWER_ALIGN(MAX(0, area->y0));
    y1 = NBGL_UPPER_ALIGN(MIN(SCREEN_HEIGHT, area->y0 + area->height));
    if (y0 != MAX(0, area->y0)) {
        curBackgroundColorTop = getBackgroundColor(area->x0, y0);
    }
    if (y1 != MIN(SCREEN_HEIGHT, area->y0 + area->height)) {
        curBackgroundColorBottom = getBackgroundColor(area->x0, y1);
    }
    // draw from y0 to area->y0 in background color
    for (y = y0; y < MAX(0, area->y0); y++) {
        for (x = area->x0; x < (area->x0 + area->width); x++) {
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(curBackgroundColorTop);
            }
        }
    }
    // draw from area->y0 + area->height to y1 in background color
    for (y = MIN(SCREEN_HEIGHT, area->y0 + area->height); y < y1; y++) {
        for (x = area->x0; x < (area->x0 + area->width); x++) {
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(curBackgroundColorBottom);
            }
        }
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Draws a plain rectangle with the given parameters
 *
 * @param area position, size and color of the rectangle to draw
 */
void nbgl_driver_drawRect(nbgl_area_t *area)
{
    int16_t x, y, y0, y1;
    color_t curBackgroundColorTop, curBackgroundColorBottom;
    LOG_DEBUG(LOW_LOGGER,
              "nbgl_driver_drawRect: x0 = %d, y0=%d, width=%d, height=%d, color=%d\n",
              area->x0,
              area->y0,
              area->width,
              area->height,
              area->backgroundColor);
    CHECK_PARAMS();
    y0 = NBGL_LOWER_ALIGN(MAX(0, area->y0));
    y1 = NBGL_UPPER_ALIGN(MIN(SCREEN_HEIGHT, area->y0 + area->height));

    // memorize screen background color if full screen paint
    if ((area->width == SCREEN_WIDTH) && (area->height == SCREEN_HEIGHT)) {
        backgroundColor          = area->backgroundColor;
        curBackgroundColorTop    = backgroundColor;
        curBackgroundColorBottom = backgroundColor;
        // reset last painted areas
        lastAreaIndex = 0;
        nbValidAreas  = 0;
    }
    else {
        if (y0 != MAX(0, area->y0)) {
            curBackgroundColorTop = getBackgroundColor(area->x0, y0);
        }
        if (y1 != MIN(SCREEN_HEIGHT, area->y0 + area->height)) {
            curBackgroundColorBottom = getBackgroundColor(area->x0, y1);
        }
        // add in circular buffer
        if (nbValidAreas < NB_LAST_AREAS) {
            nbValidAreas++;
        }
        memcpy(&lastAreas[lastAreaIndex], area, sizeof(nbgl_area_t));
        if (lastAreaIndex < (NB_LAST_AREAS - 1)) {
            lastAreaIndex++;
        }
        else {
            lastAreaIndex = 0;
        }
    }
    // draw from y0 to area->y0 in background color
    for (y = MAX(0, area->y0); y < y0; y++) {
        for (x = area->x0; x < (area->x0 + area->width); x++) {
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(curBackgroundColorTop);
            }
        }
    }

    // draw from area->y0 to area->y0 + area->height
    for (; y < MIN(SCREEN_HEIGHT, area->y0 + area->height); y++) {
        for (x = area->x0; x < (area->x0 + area->width); x++) {
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(area->backgroundColor);
            }
        }
    }

    // draw from area->y0 + area->height to y1 in background color
    for (; y < y1; y++) {
        for (x = area->x0; x < (area->x0 + area->width); x++) {
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(curBackgroundColorBottom);
            }
        }
    }
}

/**
 * @brief Draws a line with the given parameters
 *
 * @note if height <= VERTICAL_ALIGNMENT, it's considered as a horizontal line
 *       if width <= VERTICAL_ALIGNMENT, it's considered as a vertical line
 *
 * @param area position, size and color of the line to draw
 * @param dotStartIdx start index for dotted lines (index in x)
 * @param lineColor color to be applied to the line
 */
void nbgl_driver_drawLine(nbgl_area_t *area, uint8_t startIndex, color_t lineColor)
{
    CHECK_PARAMS();

    UNUSED(startIndex);
    // draw an aligned background
    draw_alignedBackground(area);

    // LOG_DEBUG(LOW_LOGGER,"nbgl_ll_drawHorizontalLine: area->x0 = %d\n",area->x0);
    for (int16_t y = 0; y < area->height; y++) {
        for (int16_t x = 0; x < area->width; x++) {
#if NB_COLOR_BITS == 4
            framebuffer[(y + area->y0) * FULL_SCREEN_WIDTH + x + area->x0]
                = EXPAND_TO_4BPP(lineColor);
#else
            if ((lineColor == BLACK) || (lineColor == WHITE)) {
                framebuffer[(y + area->y0) * FULL_SCREEN_WIDTH + x + area->x0]
                    = EXPAND_TO_4BPP(lineColor);
            }
            else {
                if (area->height == 1) {  // horizontal line
                    if ((x % 3) == startIndex) {
                        framebuffer[(y + area->y0) * FULL_SCREEN_WIDTH + x + area->x0]
                            = EXPAND_TO_4BPP(BLACK);
                    }
                }
                else if (area->width == 1) {  // vertical line
                    if ((y % 3) == startIndex) {
                        framebuffer[(y + area->y0) * FULL_SCREEN_WIDTH + x + area->x0]
                            = EXPAND_TO_4BPP(BLACK);
                    }
                }
                else {
                    framebuffer[(y + area->y0) * FULL_SCREEN_WIDTH + x + area->x0]
                        = (((x + y) & 0x1) == 1) ? EXPAND_TO_4BPP(BLACK) : EXPAND_TO_4BPP(WHITE);
                }
            }

#endif
        }
    }
}

/**
 * @brief Draws the given 1 BPP bitmap with the given parameters
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 * @param foreColor color to be applied to the 1's in bitmap
 */
static void nbgl_driver_draw1BPPImage(nbgl_area_t *area, uint8_t *buffer, color_t foreColor)
{
    int16_t  x, y;
    uint8_t  shift = 0;
    uint8_t *end   = buffer + ((area->width * area->height + 7) / 8);
    CHECK_PARAMS();
    // draw an aligned background
    draw_alignedBackground(area);
    x = area->x0 + area->width - 1;
    y = area->y0;
    while (buffer < end) {
        uint8_t pixel_val = (*buffer >> (7 - shift)) & 0x1;
        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            framebuffer[y * FULL_SCREEN_WIDTH + x]
                = EXPAND_TO_4BPP(pixel_val ? foreColor : area->backgroundColor);
        }

        shift++;
        if (shift == 8) {
            shift = 0;
            buffer++;
        }
        if (y < (area->y0 + area->height - 1)) {
            y++;
        }
        else {
            y = area->y0;
            if (x == area->x0) {
                return;
            }
            x--;
        }
    }
}

/**
 * @brief Draws the given 1 BPP bitmap with the given parameters, but reading pixels from right to
 * left
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 * @param foreColor color to be applied to the 1's in bitmap
 */
static void nbgl_driver_draw1BPPImageVerticalMirror(nbgl_area_t *area,
                                                    uint8_t     *buffer,
                                                    color_t      foreColor)
{
    int16_t  x, y;
    uint8_t  shift = 0;
    uint8_t *end   = buffer + (area->width * area->height / 8);
    CHECK_PARAMS();
    // draw an aligned background
    draw_alignedBackground(area);
    x = area->x0;
    y = area->y0;
    while (buffer < end) {
        uint8_t pixel_val = (*buffer >> (7 - shift)) & 0x1;
        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            framebuffer[y * FULL_SCREEN_WIDTH + x]
                = EXPAND_TO_4BPP(pixel_val ? foreColor : area->backgroundColor);
        }
        shift++;
        if (shift == 8) {
            shift = 0;
            buffer++;
        }
        if (y < (area->y0 + area->height - 1)) {
            y++;
        }
        else {
            y = area->y0;
            x++;
        }
    }
}

/**
 * @brief Draws the given 1 BPP bitmap with the given parameters, but reading pixels from top right
 * to bottom left (rotation of 90 degrees, clockwise)
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 * @param foreColor color to be applied to the 1's in bitmap
 */
static void nbgl_driver_draw1BPPImageRotate90(nbgl_area_t *area, uint8_t *buffer, color_t foreColor)
{
    int16_t  x, y;
    uint8_t  shift = 0;
    uint8_t *end   = buffer + (area->width * area->height / 8);
    // draw an aligned background
    draw_alignedBackground(area);
    CHECK_PARAMS_ROTATED();

    x = area->x0 + area->height - 1;
    y = area->y0 + area->width - 1;
    while (buffer < end) {
        uint8_t pixel_val = (*buffer >> (7 - shift)) & 0x1;
        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            framebuffer[y * FULL_SCREEN_WIDTH + x]
                = EXPAND_TO_4BPP(pixel_val ? foreColor : area->backgroundColor);
        }

        shift++;
        if (shift == 8) {
            shift = 0;
            buffer++;
        }
        if (x > area->x0) {
            x--;
        }
        else {
            x = area->x0 + area->height - 1;
            y--;
        }
    }
}

/**
 * @brief Uncompress a 1BPP RLE-encoded glyph
 *
 * 1BPP RLE Encoder:
 *
 * compressed bytes contains ZZZZOOOO nibbles, with
 * - ZZZZ: number of consecutives zeros (from 0 to 15)
 * - OOOO: number of consecutives ones (from 0 to 15)
 *
 * @param area
 * @param buffer buffer of RLE-encoded data
 * @param buffer_len length of buffer
 */
static void nbgl_driver_draw1BPPImageRle(nbgl_area_t *area,
                                         uint8_t     *buffer,
                                         uint32_t     buffer_len,
                                         color_t      foreColor,
                                         uint8_t      nb_skipped_bytes)
{
    int16_t x, y;
    uint8_t pixel;
    size_t  index = 0;
    // Set the initial number of transparent pixels
    size_t nb_zeros         = (size_t) nb_skipped_bytes * 8;
    size_t nb_ones          = 0;
    size_t remaining_pixels = area->width * area->height;

    // draw an aligned background
    draw_alignedBackground(area);
    CHECK_PARAMS();
    x = area->x0 + area->width - 1;
    y = area->y0;
    while (remaining_pixels && (index < buffer_len || nb_zeros || nb_ones)) {
        // Reload nb_zeros & nb_ones if needed
        while (!nb_zeros && !nb_ones && index < buffer_len) {
            uint8_t byte = buffer[index++];
            nb_ones      = byte & 0x0F;
            nb_zeros     = byte >> 4;
        }
        // Get next pixel
        if (nb_zeros) {
            --nb_zeros;
            pixel = 0;
        }
        else if (nb_ones) {
            --nb_ones;
            pixel = 1;
        }
        --remaining_pixels;

        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            framebuffer[y * FULL_SCREEN_WIDTH + x]
                = EXPAND_TO_4BPP(pixel ? foreColor : area->backgroundColor);
        }
        if (y < (area->y0 + area->height - 1)) {
            y++;
        }
        else {
            y = area->y0;
            x--;
        }
    }
}

/**
 * @brief Draws the given 2 BPP bitmap with the given parameters. The colorMap is applied if not
 * null.
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap.
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 * @param colorMap color map to be applied on given 2BPP pixels, that may be different from desired
 * colors
 */
static void nbgl_driver_draw2BPPImage(nbgl_area_t *area, uint8_t *buffer, nbgl_color_map_t colorMap)
{
    int16_t  x, y;
    uint8_t  shift = 0;
    uint8_t *end   = buffer + (area->width * area->height / 4);
    CHECK_PARAMS();
    x = area->x0 + area->width - 1;
    y = area->y0;
    while (buffer < end) {
        uint8_t pixel_val = (*buffer >> (6 - shift)) & 0x3;
        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            if (colorMap == INVALID_COLOR_MAP) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(pixel_val);
            }
            else {
                framebuffer[y * FULL_SCREEN_WIDTH + x]
                    = EXPAND_TO_4BPP(GET_COLOR_MAP(colorMap, pixel_val));
            }
        }
        shift += 2;
        if (shift == 8) {
            shift = 0;
            buffer++;
        }
        if (y < (area->y0 + area->height - 1)) {
            y++;
        }
        else {
            y = area->y0;
            x--;
        }
    }
}

/**
 * @brief Draws the given 2 BPP bitmap with the given parameters, but from right to left. The
 * colorMap is applied if not null.
 *
 * @note x0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap.
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 * @param colorMap color map to be applied on given 2BPP pixels, that may be different from desired
 * colors
 */
static void nbgl_driver_draw2BPPImageVerticalMirror(nbgl_area_t     *area,
                                                    uint8_t         *buffer,
                                                    nbgl_color_map_t colorMap)
{
    int16_t  x, y;
    uint8_t  shift = 0;
    uint8_t *end   = buffer + (area->width * area->height / 4);
    CHECK_PARAMS();
    x = area->x0;
    y = area->y0;
    while (buffer < end) {
        uint8_t pixel_val = (*buffer >> (6 - shift)) & 0x3;
        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            if (colorMap == INVALID_COLOR_MAP) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = EXPAND_TO_4BPP(pixel_val);
            }
            else {
                framebuffer[y * FULL_SCREEN_WIDTH + x]
                    = EXPAND_TO_4BPP(GET_COLOR_MAP(colorMap, pixel_val));
            }
        }
        shift += 2;
        if (shift == 8) {
            shift = 0;
            buffer++;
        }
        if (y < (area->y0 + area->height - 1)) {
            y++;
        }
        else {
            y = area->y0;
            x++;
        }
    }
}

// 4BPP Color maps

// Extract a color from color_map, if color_map is not NULL
#define GET_MAPPED_COLOR(color_map, color_index) \
    ((color_map == NULL) ? (color_index) : color_map[(color_index)])

// Return the 4BPP value corresponding to input color
static uint8_t get_4bpp_value(color_t color)
{
    switch (color) {
        case BLACK:
            return BLACK_4BPP;
        case WHITE:
            return WHITE_4BPP;
        case LIGHT_GRAY:
            return LIGHT_GRAY_4BPP;
        case DARK_GRAY:
            return DARK_GRAY_4BPP;
        default:
            return WHITE_4BPP;
    }
}

// Size of 4BPP color maps
#define COLOR_MAP_SIZE_4BPP 16

// Compute a default color map, starting from input color to area background color.
// The values are evenly distributed whenever possible.
// The map is written to output_map.
static void compute_default_color_map(uint8_t      output_map[COLOR_MAP_SIZE_4BPP],
                                      color_t      color,
                                      nbgl_area_t *area)
{
    uint8_t start_val = get_4bpp_value(color);
    uint8_t end_val   = get_4bpp_value(area->backgroundColor);

    int    nb_values;  // Number of different values to be written in the output map
    int8_t incr;
    if (end_val >= start_val) {
        nb_values = (end_val - start_val + 1);
        incr      = -1;
    }
    else {
        nb_values = (start_val - end_val + 1);
        incr      = 1;
    }

    // Whenever (cnt << 8) reaches next_step value
    // we increment the value written in the output map.
    uint32_t cnt_step  = ((COLOR_MAP_SIZE_4BPP << 8) / nb_values);
    uint32_t next_step = cnt_step;

    // The output map is filled from the end to the start
    output_map[COLOR_MAP_SIZE_4BPP - 1] = end_val;
    for (uint32_t cnt = 1; cnt < COLOR_MAP_SIZE_4BPP; cnt++) {
        uint8_t arr_index = COLOR_MAP_SIZE_4BPP - 1 - cnt;
        if ((cnt << 8) >= next_step) {
            next_step += cnt_step;
            output_map[arr_index] = output_map[arr_index + 1] + incr;
        }
        else {
            output_map[arr_index] = output_map[arr_index + 1];
        }
    }
}

// These hardcoded color maps have been manually tuned with the designer.

static const uint8_t WHITE_ON_BLACK[]
    = {WHITE_4BPP, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, BLACK_4BPP};

static const uint8_t DARK_GRAY_ON_WHITE[COLOR_MAP_SIZE_4BPP]
    = {DARK_GRAY_4BPP, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12, 13, 14, WHITE_4BPP};

static const uint8_t LIGHT_GRAY_ON_WHITE[COLOR_MAP_SIZE_4BPP]
    = {LIGHT_GRAY_4BPP, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, WHITE_4BPP};

static const uint8_t LIGHT_GRAY_ON_BLACK[COLOR_MAP_SIZE_4BPP]
    = {LIGHT_GRAY_4BPP, 9, 8, 7, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, BLACK_4BPP};

static const uint8_t *DARK_GRAY_ON_BLACK = LIGHT_GRAY_ON_BLACK;

// Buffer where non-hardcoded color maps are computed

static uint8_t default_color_map[16];

// Return a pointer to the color map corresponding to input area and color
static const uint8_t *get_color_map_array(nbgl_area_t *area, color_t color)
{
    if ((color == BLACK) && (area->backgroundColor == WHITE)) {
        return NULL;  // No color map to apply
    }
    else if ((color == WHITE) && (area->backgroundColor == BLACK)) {
        return WHITE_ON_BLACK;
    }
    else if ((color == DARK_GRAY) && (area->backgroundColor == WHITE)) {
        return DARK_GRAY_ON_WHITE;
    }
    else if ((color == LIGHT_GRAY) && (area->backgroundColor == WHITE)) {
        return LIGHT_GRAY_ON_WHITE;
    }
    else if ((color == LIGHT_GRAY) && (area->backgroundColor == BLACK)) {
        return LIGHT_GRAY_ON_BLACK;
    }
    else if ((color == DARK_GRAY) && (area->backgroundColor == BLACK)) {
        return DARK_GRAY_ON_BLACK;
    }

    compute_default_color_map(default_color_map, color, area);
    return default_color_map;
}

/**
 * @brief Draws the given 4 BPP bitmap with the given parameters. The colorMap is applied if not
 * null.
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap.
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 */
static void nbgl_driver_draw4BPPImage(nbgl_area_t *area, uint8_t *buffer, color_t color)
{
    int16_t  x, y;
    uint8_t  shift = 0;
    uint8_t *end   = buffer + (area->width * area->height / 2);
    CHECK_PARAMS();
    x                        = area->x0 + area->width - 1;
    y                        = area->y0;
    const uint8_t *color_map = get_color_map_array(area, color);
    while (buffer < end) {
        uint8_t color_index = ((*buffer) >> (4 - shift)) & 0x0F;
        uint8_t pixel_val   = GET_MAPPED_COLOR(color_map, color_index);

        if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
            framebuffer[y * FULL_SCREEN_WIDTH + x] = pixel_val;
        }

        shift += 4;
        if (shift == 8) {
            shift = 0;
            buffer++;
        }
        if (y < (area->y0 + area->height - 1)) {
            y++;
        }
        else {
            y = area->y0;
            x--;
        }
    }
}

/**
 * @brief Draws the given 1 BPP compressed bitmap with the given parameters. The colorMap is applied
 * if not null.
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap.
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 */
static void nbgl_ll_draw1BPPCompressedImage(nbgl_area_t *area,
                                            uint8_t     *buffer,
                                            uint32_t     bufferLen,
                                            uint8_t     *uzlibChunkBuffer,
                                            color_t      foreColor)
{
    int16_t             x, y;
    uint8_t             shift = 0;
    unsigned int        dlen;
    struct uzlib_uncomp d;
    int                 res;
    CHECK_PARAMS();
    x = area->x0 + area->width - 1;
    y = area->y0;

    // draw an aligned background
    draw_alignedBackground(area);
    uzlib_init();
    while (bufferLen > 0) {
        uint16_t compressed_chunk_len;

        // read length of compressed chunk on 2 bytes
        compressed_chunk_len = 256 * buffer[1] + buffer[0];
        buffer += 2;
        /* -- get decompressed length -- */
        dlen = buffer[compressed_chunk_len - 1];
        dlen = 256 * dlen + buffer[compressed_chunk_len - 2];
        dlen = 256 * dlen + buffer[compressed_chunk_len - 3];
        dlen = 256 * dlen + buffer[compressed_chunk_len - 4];
        bufferLen -= compressed_chunk_len + 2;

        /* there can be mismatch between length in the trailer and actual
            data stream; to avoid buffer overruns on overlong streams, reserve
            one extra byte */
        dlen++;

        /* -- decompress data -- */
        uzlib_uncompress_init(&d, NULL, 0);

        /* all 3 fields below must be initialized by user */
        d.source         = buffer;
        d.source_limit   = buffer + compressed_chunk_len - 4;
        d.source_read_cb = NULL;

        // position buffer to the end of the current compressed chunk
        buffer += compressed_chunk_len;

        res = uzlib_gzip_parse_header(&d);
        if (res != TINF_OK) {
            printf("Error parsing header: %d\n", res);
            exit(1);
        }

        d.dest_start = d.dest = uzlibChunkBuffer;

        d.dest_limit = d.dest + dlen;
        res          = uzlib_uncompress_chksum(&d);
        if (res != TINF_DONE) {
            printf("error: %d\n", res);
            break;
        }
        unsigned int i = 0;

        // write pixels
        while (i < (dlen - 1)) {
            uint8_t pixel_val = (uzlibChunkBuffer[i] >> (7 - shift)) & 0x1;
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x]
                    = EXPAND_TO_4BPP(pixel_val ? foreColor : area->backgroundColor);
            }
            shift++;
            if (shift == 8) {
                shift = 0;
                i++;
            }
            if (y < (area->y0 + area->height - 1)) {
                y++;
            }
            else {
                y = area->y0;
                x--;
            }
        }
    }
}

/**
 * @brief Draws the given 4 BPP compressed bitmap with the given parameters. The colorMap is applied
 * if not null.
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in bitmap.
 *
 * @param area position, size and color of the bitmap to draw
 * @param buffer bitmap buffer
 */
static void nbgl_ll_draw4BPPCompressedImage(nbgl_area_t *area,
                                            uint8_t     *buffer,
                                            uint32_t     bufferLen,
                                            uint8_t     *uzlibChunkBuffer,
                                            color_t      foreColor)
{
    int16_t             x, y;
    uint8_t             shift = 0;
    unsigned int        dlen;
    struct uzlib_uncomp d;
    int                 res;
    CHECK_PARAMS();
    x                        = area->x0 + area->width - 1;
    y                        = area->y0;
    const uint8_t *color_map = get_color_map_array(area, foreColor);

    uzlib_init();
    while (bufferLen > 0) {
        uint16_t compressed_chunk_len;

        // read length of compressed chunk on 2 bytes
        compressed_chunk_len = 256 * buffer[1] + buffer[0];
        buffer += 2;
        /* -- get decompressed length -- */
        dlen = buffer[compressed_chunk_len - 1];
        dlen = 256 * dlen + buffer[compressed_chunk_len - 2];
        dlen = 256 * dlen + buffer[compressed_chunk_len - 3];
        dlen = 256 * dlen + buffer[compressed_chunk_len - 4];
        bufferLen -= compressed_chunk_len + 2;

        /* there can be mismatch between length in the trailer and actual
            data stream; to avoid buffer overruns on overlong streams, reserve
            one extra byte */
        dlen++;

        /* -- decompress data -- */
        uzlib_uncompress_init(&d, NULL, 0);

        /* all 3 fields below must be initialized by user */
        d.source         = buffer;
        d.source_limit   = buffer + compressed_chunk_len - 4;
        d.source_read_cb = NULL;

        // position buffer to the end of the current compressed chunk
        buffer += compressed_chunk_len;

        res = uzlib_gzip_parse_header(&d);
        if (res != TINF_OK) {
            printf("Error parsing header: %d\n", res);
            exit(1);
        }

        d.dest_start = d.dest = uzlibChunkBuffer;

        d.dest_limit = d.dest + dlen;
        res          = uzlib_uncompress_chksum(&d);
        if (res != TINF_DONE) {
            printf("error: %d\n", res);
            break;
        }
        unsigned int i = 0;
        // write pixels
        while (i < (dlen - 1)) {
            uint8_t pixel_val = (uzlibChunkBuffer[i] >> (4 - shift)) & 0xF;
            if (IS_IN_SCREEN(x, y) && IS_NOT_KEEP_OUT(x, y)) {
                framebuffer[y * FULL_SCREEN_WIDTH + x] = GET_MAPPED_COLOR(color_map, pixel_val);
            }
            shift += 4;
            if (shift == 8) {
                shift = 0;
                i++;
            }
            if (y < (area->y0 + area->height - 1)) {
                y++;
            }
            else {
                y = area->y0;
                x--;
            }
        }
    }
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
void nbgl_driver_drawImage(nbgl_area_t          *area,
                           uint8_t              *buffer,
                           nbgl_transformation_t transformation,
                           nbgl_color_map_t      colorMap)
{
    if (area->bpp == NBGL_BPP_1) {
        if (transformation == NO_TRANSFORMATION) {
            nbgl_driver_draw1BPPImage(area, buffer, GET_COLOR_MAP(colorMap, 0));
        }
        else if (transformation == VERTICAL_MIRROR) {
            nbgl_driver_draw1BPPImageVerticalMirror(area, buffer, GET_COLOR_MAP(colorMap, 0));
        }
        else if (transformation == ROTATE_90_CLOCKWISE) {
            nbgl_driver_draw1BPPImageRotate90(area, buffer, GET_COLOR_MAP(colorMap, 0));
        }
    }
    else if (area->bpp == NBGL_BPP_2) {
        if (transformation == NO_TRANSFORMATION) {
            nbgl_driver_draw2BPPImage(area, buffer, colorMap);
        }
        else if (transformation == VERTICAL_MIRROR) {
            nbgl_driver_draw2BPPImageVerticalMirror(area, buffer, colorMap);
        }
    }
    else if (area->bpp == NBGL_BPP_4) {
        nbgl_driver_draw4BPPImage(area, buffer, colorMap);
    }
    else {
        LOG_FATAL(LOW_LOGGER, "Forbidden bpp: %d\n", area->bpp);
    }
}

/////// RLE functions

uint8_t rle_4bpp_uncompress_buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 2];

// Write nb_pix 4BPP pixels of the same color to the display
static inline void fill_4bpp_pixels_color(uint8_t   color,
                                          uint8_t   nb_pix,
                                          uint32_t *pix_cnt,
                                          uint8_t  *remaining,
                                          uint32_t  max_pix_cnt)
{
    // Check max
    if ((*pix_cnt + nb_pix) > max_pix_cnt) {
        return;
    }

    uint8_t double_pix        = (color << 4) | color;
    bool    first_non_aligned = (*pix_cnt % 2);
    bool    last_non_aligned  = (*pix_cnt + nb_pix) % 2;

    // If first pixel to write is non aligned
    if (first_non_aligned) {
        *remaining |= (0x0F & color);
        // Remaining byte is now full: send it
        // spi_queue_byte(*remaining);
        rle_4bpp_uncompress_buffer[(*pix_cnt) / 2] = *remaining;
        *remaining                                 = 0;
        (*pix_cnt)++;
        nb_pix--;
    }

    // Write pixels 2 by 2
    for (uint32_t i = 0; i < nb_pix / 2; i++) {
        rle_4bpp_uncompress_buffer[(*pix_cnt) / 2] = double_pix;
        (*pix_cnt) += 2;
    }

    // If last pixel to write is non aligned
    if (last_non_aligned) {
        // Save remaining pixel
        *remaining = color << 4;
        (*pix_cnt)++;
    }
}

// Handle 'Copy white' RLE instruction
static uint32_t handle_4bpp_repeat_white(uint8_t        byte_in,
                                         uint32_t      *pix_cnt,
                                         const uint8_t *color_map,
                                         uint8_t       *remaining_byte,
                                         uint32_t       max_pix_cnt)
{
    uint8_t nb_pix = (byte_in & 0x3F) + 1;
    uint8_t color  = GET_MAPPED_COLOR(color_map, 0x0F);
    fill_4bpp_pixels_color(color, nb_pix, pix_cnt, remaining_byte, max_pix_cnt);
    return 1;
}

// Handle 'Repeat color' RLE instruction
static uint32_t handle_4bpp_repeat_color(uint8_t        byte_in,
                                         uint32_t      *pix_cnt,
                                         const uint8_t *color_map,
                                         uint8_t       *remaining_byte,
                                         uint32_t       max_pix_cnt)
{
    uint8_t nb_pix = ((byte_in & 0x70) >> 4) + 1;
    uint8_t color  = GET_MAPPED_COLOR(color_map, byte_in & 0x0F);
    fill_4bpp_pixels_color(color, nb_pix, pix_cnt, remaining_byte, max_pix_cnt);
    return 1;
}

// Handle 'Copy' RLE instruction
static uint32_t handle_4bpp_copy(uint8_t       *bytes_in,
                                 uint32_t       bytes_in_len,
                                 uint32_t      *pix_cnt,
                                 const uint8_t *color_map,
                                 uint8_t       *remaining_byte,
                                 uint32_t       max_pix_cnt)
{
    uint8_t nb_pix        = ((bytes_in[0] & 0x30) >> 4) + 3;
    uint8_t nb_bytes_read = (nb_pix / 2) + 1;

    // Do not read outside of bytes_in
    if (nb_bytes_read > bytes_in_len) {
        return nb_bytes_read;
    }

    for (uint8_t i = 0; i < nb_pix; i++) {
        // Write pix by pix
        uint8_t index = (i + 1) / 2;
        uint8_t color_index;
        if ((i % 2) == 0) {
            color_index = bytes_in[index] & 0x0F;
        }
        else {
            color_index = bytes_in[index] >> 4;
        }
        uint8_t color = GET_MAPPED_COLOR(color_map, color_index);
        fill_4bpp_pixels_color(color, 1, pix_cnt, remaining_byte, max_pix_cnt);
    }

    return nb_bytes_read;
}

/**
 * @brief Uncompress a 4BPP RLE-encoded glyph
 *
 * 4BPP RLE Encoder:
 *
 * 'Repeat white' byte
 * - [11][number of whites to write - 1 (6 bits)]
 *
 * 'Copy' byte
 * - [10][number of nibbles to copy - 3 (2)][nib0 (4 bits)][nib1 (4 bits)]...
 *
 * 'Repeat color' byte
 * - [0][number of bytes to write - 1 (3 bits)][color index (4 bits)]
 *
 * @param area
 * @param buffer buffer of RLE-encoded data
 * @param len length of buffer
 */
static void nbgl_driver_draw4BPPImageRle(nbgl_area_t *area,
                                         uint8_t     *buffer,
                                         uint32_t     buffer_len,
                                         color_t      fore_color,
                                         uint8_t      nb_skipped_bytes)
{
    CHECK_PARAMS();

    if (area->bpp != NBGL_BPP_4) {
        return;
    }

    const uint8_t *color_map = get_color_map_array(area, fore_color);
    // Fill buffer with background color
    uint8_t background = GET_MAPPED_COLOR(color_map, 0xF);
    background |= background << 4;
    memset(rle_4bpp_uncompress_buffer, background, sizeof(rle_4bpp_uncompress_buffer));
    uint32_t max_pix_cnt = (area->width * area->height);
    // 'transparent' bytes was set (with memset), not need to write them again
    uint32_t pix_cnt   = nb_skipped_bytes * 2;
    uint32_t read_cnt  = 0;
    uint8_t  remaining = 0;

    while (read_cnt < buffer_len) {
        uint8_t byte = buffer[read_cnt];
        if (byte & 0x80) {
            if (byte & 0x40) {
                read_cnt
                    += handle_4bpp_repeat_white(byte, &pix_cnt, color_map, &remaining, max_pix_cnt);
            }
            else {
                uint8_t *bytes_in = buffer + read_cnt;
                uint32_t max_len  = buffer_len - read_cnt;
                read_cnt += handle_4bpp_copy(
                    bytes_in, max_len, &pix_cnt, color_map, &remaining, max_pix_cnt);
            }
        }
        else {
            read_cnt
                += handle_4bpp_repeat_color(byte, &pix_cnt, color_map, &remaining, max_pix_cnt);
        }
    }

    if ((pix_cnt % 2) != 0) {
        rle_4bpp_uncompress_buffer[(pix_cnt) / 2] = remaining;
    }

    // Now send it as if it was a 4BPP uncompressed image
    color_t background_color = area->backgroundColor;
    area->backgroundColor    = WHITE;
    nbgl_driver_draw4BPPImage(area, rle_4bpp_uncompress_buffer, BLACK);
    area->backgroundColor = background_color;
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
void nbgl_driver_drawImageFile(nbgl_area_t     *area,
                               uint8_t         *buffer,
                               nbgl_color_map_t colorMap,
                               uint8_t         *uzlib_chunk_buffer)
{
    // uzlib_chunk_buffer is ignored in simulation
    uint8_t compression = GET_IMAGE_FILE_COMPRESSION(buffer);

    area->width  = GET_IMAGE_FILE_WIDTH(buffer);
    area->height = GET_IMAGE_FILE_HEIGHT(buffer);
    area->bpp    = GET_IMAGE_FILE_BPP(buffer);
    if (compression == NBGL_NO_COMPRESSION) {
        nbgl_driver_drawImage(area, GET_IMAGE_FILE_BUFFER(buffer), NO_TRANSFORMATION, colorMap);
    }
    else if (compression == NBGL_GZLIB_COMPRESSION) {
        // TODO: add support of compression at least for 2BPP
        if (area->bpp == NBGL_BPP_1) {
            nbgl_ll_draw1BPPCompressedImage(area,
                                            GET_IMAGE_FILE_BUFFER(buffer),
                                            GET_IMAGE_FILE_BUFFER_LEN(buffer),
                                            uzlib_chunk_buffer,
                                            GET_COLOR_MAP(colorMap, 0));
        }
        else if (area->bpp == NBGL_BPP_4) {
            nbgl_ll_draw4BPPCompressedImage(area,
                                            GET_IMAGE_FILE_BUFFER(buffer),
                                            GET_IMAGE_FILE_BUFFER_LEN(buffer),
                                            uzlib_chunk_buffer,
                                            (color_t) GET_COLOR_MAP(colorMap, 0));
        }
    }
    else if (compression == NBGL_RLE_COMPRESSION) {
        nbgl_driver_drawImageRle(area,
                                 GET_IMAGE_FILE_BUFFER(buffer),
                                 GET_IMAGE_FILE_BUFFER_LEN(buffer),
                                 (color_t) GET_COLOR_MAP(colorMap, 0),
                                 0);
    }
}

/**
 * @brief function to actually refresh the given area on screen
 *
 * @param area the area to refresh
 */
void nbgl_driver_refreshArea(nbgl_area_t        *area,
                             nbgl_refresh_mode_t mode,
                             nbgl_post_refresh_t post_refresh)
{
    UNUSED(mode);
    UNUSED(post_refresh);
    // LOG_WARN(LOW_LOGGER,"nbgl_driver_refreshArea(): x0:%d,y0:%d,width:%d,height:%d\n", area->x0,
    // area->y0, area->width, area->height);
    if ((area->width == 0) || (area->height == 0)) {
        return;
    }
    // LOG_DEBUG(LOW_LOGGER,"nbgl_driver_refreshArea(): \n");
    scenario_save_screen((char *) framebuffer, area);
}

void nbgl_driver_drawImageRle(nbgl_area_t *area,
                              uint8_t     *buffer,
                              uint32_t     buffer_len,
                              color_t      fore_color,
                              uint8_t      nb_skipped_bytes)
{
    if (buffer_len == 0) {
        return;
    }
    if (area->bpp == NBGL_BPP_4) {
        nbgl_driver_draw4BPPImageRle(area, buffer, buffer_len, fore_color, nb_skipped_bytes);
    }
    else if (area->bpp == NBGL_BPP_1) {
        nbgl_driver_draw1BPPImageRle(area, buffer, buffer_len, fore_color, nb_skipped_bytes);
    }
}

#ifdef HAVE_SE_SCREEN
void screen_set_keepout(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    if (!width || !height) {
        keepOutArea.x_start = 0xFFFE;
        keepOutArea.x_end   = 0xFFFF;
        keepOutArea.y_start = 0xFFFE;
        keepOutArea.y_end   = 0xFFFF;
        return;
    }

    if (x > SCREEN_WIDTH) {
        x = SCREEN_WIDTH;
    }
    if (y > SCREEN_HEIGHT) {
        y = SCREEN_HEIGHT;
    }
    if (width > SCREEN_WIDTH) {
        width = SCREEN_WIDTH;
    }
    if (height > SCREEN_HEIGHT) {
        height = SCREEN_HEIGHT;
    }

    keepOutArea.x_start = x;
    keepOutArea.x_end   = x + width;
    keepOutArea.y_start = y;
    keepOutArea.y_end   = y + height;
}
#endif  // HAVE_SE_SCREEN
