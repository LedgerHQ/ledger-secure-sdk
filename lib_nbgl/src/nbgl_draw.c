
/**
 * @file nbgl_draw.c
 * @brief Implementation of middle-level API to draw rich forms like
 * rounded rectangle
 */

/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include "nbgl_front.h"
#include "nbgl_draw.h"
#include "nbgl_fonts.h"
#include "nbgl_debug.h"
#include "nbgl_side.h"
#ifdef NBGL_QRCODE
#include "qrcodegen.h"
#endif  // NBGL_QRCODE
#ifdef BUILD_SCREENSHOTS
#include <assert.h>
#endif  // BUILD_SCREENSHOTS
#include "glyphs.h"
#include "os_pic.h"
#include "os_utils.h"
#include "os_helpers.h"

/*********************
 *      DEFINES
 *********************/
#ifdef SCREEN_SIZE_WALLET
typedef enum {
    LEFT_HALF,
    RIGHT_HALF
} half_t;
#else   // SCREEN_SIZE_WALLET
typedef enum {
    BAGL_FILL_CIRCLE_0_PI2,
    BAGL_FILL_CIRCLE_PI2_PI,
    BAGL_FILL_CIRCLE_PI_3PI2,
    BAGL_FILL_CIRCLE_3PI2_2PI
} quarter_t;
#endif  // SCREEN_SIZE_WALLET

#define QR_PIXEL_WIDTH_HEIGHT 4

#ifdef SCREEN_SIZE_WALLET
#define MAX_FONT_HEIGHT    48
#define AVERAGE_CHAR_WIDTH 32
#else  // SCREEN_SIZE_WALLET
#define MAX_FONT_HEIGHT    24
#define AVERAGE_CHAR_WIDTH 24
#endif  // SCREEN_SIZE_WALLET

/**********************
 *      TYPEDEFS
 **********************/
#ifdef NBGL_QRCODE
typedef struct {
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    uint8_t QrDrawBuffer[QR_PIXEL_WIDTH_HEIGHT * QR_PIXEL_WIDTH_HEIGHT * QR_MAX_PIX_SIZE / 8];
} QrCodeBuffer_t;

#define qrcode       ((QrCodeBuffer_t *) ramBuffer)->qrcode
#define tempBuffer   ((QrCodeBuffer_t *) ramBuffer)->tempBuffer
#define QrDrawBuffer ((QrCodeBuffer_t *) ramBuffer)->QrDrawBuffer
#endif  // NBGL_QRCODE

// icons to be used to draw circles or discs for a given radius
#ifdef SCREEN_SIZE_WALLET
typedef struct {
    const nbgl_icon_details_t *leftDisc;
    const nbgl_icon_details_t *leftCircle;
} radiusIcons_t;
#else   // SCREEN_SIZE_WALLET
typedef struct {
    const uint8_t *topLeftDisc;
    const uint8_t *bottomLeftDisc;
    const uint8_t *topLeftCircle;
    const uint8_t *bottomLeftCircle;
} radiusIcons_t;
#endif  // SCREEN_SIZE_WALLET

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#ifndef SCREEN_SIZE_WALLET
static const uint8_t quarter_disc_3px_1bpp[]     = {0xEC, 0xFF};
static const uint8_t quarter_disc_3px_90_1bpp[]  = {0x2F, 0xFF};
static const uint8_t quarter_disc_3px_180_1bpp[] = {0x9B, 0xFF};
static const uint8_t quarter_disc_3px_270_1bpp[] = {0xFA, 0x00};

static const uint8_t quarter_circle_3px_1bpp[]     = {0x4C, 0x00};
static const uint8_t quarter_circle_3px_90_1bpp[]  = {0x0D, 0x00};
static const uint8_t quarter_circle_3px_180_1bpp[] = {0x19, 0x00};
static const uint8_t quarter_circle_3px_270_1bpp[] = {0x58, 0x00};
#endif  // SCREEN_SIZE_WALLET

// indexed by nbgl_radius_t (except RADIUS_0_PIXELS)
static const uint8_t radiusValues[RADIUS_MAX + 1] = {
#ifdef SCREEN_SIZE_WALLET
    20,
    28,
    32,
    40,
    44
#else   // SCREEN_SIZE_WALLET
    1,
    3
#endif  // SCREEN_SIZE_WALLET
};

#ifdef SCREEN_SIZE_WALLET

#if COMMON_RADIUS == 28
static const radiusIcons_t radiusIcons28px
    = {&C_half_disc_left_56px_1bpp, &C_half_circle_left_56px_1bpp};
#elif COMMON_RADIUS == 40
static const radiusIcons_t radiusIcons40px
    = {&C_half_disc_left_80px_1bpp, &C_half_circle_left_80px_1bpp};
#elif COMMON_RADIUS == 44
static const radiusIcons_t radiusIcons44px
    = {&C_half_disc_left_88px_1bpp, &C_half_circle_left_88px_1bpp};
#endif
#if SMALL_BUTTON_RADIUS == 20
static const radiusIcons_t radiusIcons20px
    = {&C_half_disc_left_40px_1bpp, &C_half_circle_left_40px_1bpp};
#elif SMALL_BUTTON_RADIUS == 32
static const radiusIcons_t radiusIcons32px
    = {&C_half_disc_left_64px_1bpp, &C_half_circle_left_64px_1bpp};
#endif

// indexed by nbgl_radius_t (except RADIUS_0_PIXELS)
static const radiusIcons_t *radiusIcons[RADIUS_MAX + 1] = {
#if SMALL_BUTTON_RADIUS == 20
    &radiusIcons20px,
#else
    NULL,
#endif
#if COMMON_RADIUS == 28
    &radiusIcons28px,
#else
    NULL,
#endif
#if SMALL_BUTTON_RADIUS == 32
    &radiusIcons32px,
#else
    NULL,
#endif
#if COMMON_RADIUS == 40
    &radiusIcons40px,
#else
    NULL,
#endif
#if COMMON_RADIUS == 44
    &radiusIcons44px
#else
    NULL
#endif  // COMMON_RADIUS
};
#endif  // SCREEN_SIZE_WALLET

#ifdef NBGL_QRCODE
// ensure that the ramBuffer also used for image file decompression is big enough for QR code
CCASSERT(qr_code_buffer, sizeof(QrCodeBuffer_t) <= GZLIB_UNCOMPRESSED_CHUNK);
#endif  // NBGL_QRCODE

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

#ifdef SCREEN_SIZE_WALLET
static void draw_circle_helper(int           x0,
                               int           y0,
                               nbgl_radius_t radiusIndex,
                               half_t        half,
                               color_t       borderColor,
                               color_t       innerColor,
                               color_t       backgroundColor)
{
    const nbgl_icon_details_t *half_icon = NULL;
    nbgl_area_t                area      = {.bpp = NBGL_BPP_1, .backgroundColor = backgroundColor};

    // radius is not supported
    if (radiusIndex > RADIUS_MAX) {
        return;
    }
    if (borderColor == innerColor) {
        half_icon = PIC(radiusIcons[radiusIndex]->leftDisc);
    }
    else {
#if NB_COLOR_BITS == 1
        borderColor = BLACK;
#endif
        half_icon = PIC(radiusIcons[radiusIndex]->leftCircle);
    }
    area.width  = half_icon->width;
    area.height = half_icon->height;
    switch (half) {
        case LEFT_HALF:  // left
            area.x0 = x0;
            area.y0 = y0;
            nbgl_frontDrawImage(&area, half_icon->bitmap, NO_TRANSFORMATION, borderColor);
            break;
        case RIGHT_HALF:  // right
            area.x0 = x0 - half_icon->width;
            area.y0 = y0;
            nbgl_frontDrawImage(&area, half_icon->bitmap, VERTICAL_MIRROR, borderColor);
            break;
    }
}
#else   // SCREEN_SIZE_WALLET
static void draw_circle_helper(int           x_center,
                               int           y_center,
                               nbgl_radius_t radiusIndex,
                               quarter_t     quarter,
                               color_t       borderColor,
                               color_t       innerColor,
                               color_t       backgroundColor)
{
    const uint8_t *quarter_buffer = NULL;
    nbgl_area_t    area           = {.bpp = NBGL_BPP_1, .backgroundColor = backgroundColor};

    // radius is not supported
    if (radiusIndex > RADIUS_MAX) {
        return;
    }
    area.width = area.height = radiusValues[radiusIndex];
    switch (quarter) {
        case BAGL_FILL_CIRCLE_3PI2_2PI:  // bottom right
            area.x0        = x_center;
            area.y0        = y_center;
            quarter_buffer = (borderColor == innerColor) ? quarter_disc_3px_180_1bpp
                                                         : quarter_circle_3px_180_1bpp;
            break;
        case BAGL_FILL_CIRCLE_PI_3PI2:  // bottom left
            area.x0        = x_center - area.width;
            area.y0        = y_center;
            quarter_buffer = (borderColor == innerColor) ? quarter_disc_3px_270_1bpp
                                                         : quarter_circle_3px_270_1bpp;
            break;
        case BAGL_FILL_CIRCLE_0_PI2:  // top right
            area.x0        = x_center;
            area.y0        = y_center - area.width;
            quarter_buffer = (borderColor == innerColor) ? quarter_disc_3px_90_1bpp
                                                         : quarter_circle_3px_90_1bpp;
            break;
        case BAGL_FILL_CIRCLE_PI2_PI:  // top left
            area.x0 = x_center - area.width;
            area.y0 = y_center - area.width;
            quarter_buffer
                = (borderColor == innerColor) ? quarter_disc_3px_1bpp : quarter_circle_3px_1bpp;
            break;
    }
    nbgl_frontDrawImage(&area, quarter_buffer, NO_TRANSFORMATION, borderColor);
}
#endif  // SCREEN_SIZE_WALLET

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief This functions draws a rounded corners rectangle (without border), with the given
 * parameters.
 *
 * @param area position, size and background color (outside of the rectangle) to use for the
 * rectangle
 * @param radiusIndex radius size
 * @param innerColor color to use for inside the rectangle
 */
void nbgl_drawRoundedRect(const nbgl_area_t *area, nbgl_radius_t radiusIndex, color_t innerColor)
{
    nbgl_area_t rectArea;
    uint8_t     radius;

    LOG_DEBUG(DRAW_LOGGER,
              "nbgl_drawRoundedRect x0 = %d, y0 = %d, width =%d, height =%d\n",
              area->x0,
              area->y0,
              area->width,
              area->height);

    if (radiusIndex <= RADIUS_MAX) {
        radius = radiusValues[radiusIndex];
    }
    else if (radiusIndex == RADIUS_0_PIXELS) {
        radius = 0;
    }
    else {
        // radius not supported
        LOG_WARN(DRAW_LOGGER, "nbgl_drawRoundedRect forbidden radius index =%d\n", radiusIndex);
        return;
    }

    // Draw full rectangle
    rectArea.x0              = area->x0;
    rectArea.y0              = area->y0;
    rectArea.width           = area->width;
    rectArea.height          = area->height;
    rectArea.backgroundColor = innerColor;
    nbgl_frontDrawRect(&rectArea);
    // special case when radius is null, just draw a rectangle
    if (radiusIndex == RADIUS_0_PIXELS) {
        return;
    }

#ifdef SCREEN_SIZE_WALLET
    UNUSED(radius);
    // Draw 2 halves of disc
    draw_circle_helper(area->x0,
                       area->y0,
                       radiusIndex,
                       LEFT_HALF,
                       innerColor,  // unused
                       innerColor,
                       area->backgroundColor);
    draw_circle_helper(area->x0 + area->width,
                       area->y0,
                       radiusIndex,
                       RIGHT_HALF,
                       innerColor,  // unused
                       innerColor,
                       area->backgroundColor);
#else   // SCREEN_SIZE_WALLET
    if (radiusIndex == RADIUS_1_PIXEL) {
        return;
    }
    // Draw 4 quarters of disc
    draw_circle_helper(area->x0 + radius,
                       area->y0 + radius,
                       radiusIndex,
                       BAGL_FILL_CIRCLE_PI2_PI,
                       innerColor,  // unused
                       innerColor,
                       area->backgroundColor);
    draw_circle_helper(area->x0 + area->width - radius,
                       area->y0 + radius,
                       radiusIndex,
                       BAGL_FILL_CIRCLE_0_PI2,
                       innerColor,  // unused
                       innerColor,
                       area->backgroundColor);
    draw_circle_helper(area->x0 + radius,
                       area->y0 + area->height - radius,
                       radiusIndex,
                       BAGL_FILL_CIRCLE_PI_3PI2,
                       innerColor,  // unused
                       innerColor,
                       area->backgroundColor);
    draw_circle_helper(area->x0 + area->width - radius,
                       area->y0 + area->height - radius,
                       radiusIndex,
                       BAGL_FILL_CIRCLE_3PI2_2PI,
                       innerColor,  // unused
                       innerColor,
                       area->backgroundColor);
#endif  // SCREEN_SIZE_WALLET
}

/**
 * @brief This functions draws a rounded corners rectangle with a border, with the given parameters.
 *
 * @param area position, size and background color (outside of the rectangle) to use for the
 * rectangle
 * @param radiusIndex radius size
 * @param stroke thickness of border (fixed to 2)
 * @param innerColor color to use for inside the rectangle
 * @param borderColor color to use for the border
 */
void nbgl_drawRoundedBorderedRect(const nbgl_area_t *area,
                                  nbgl_radius_t      radiusIndex,
                                  uint8_t            stroke,
                                  color_t            innerColor,
                                  color_t            borderColor)
{
    uint8_t     radius;
    nbgl_area_t rectArea;

    LOG_DEBUG(
        DRAW_LOGGER,
        "nbgl_drawRoundedBorderedRect: innerColor = %d, borderColor = %d, backgroundColor=%d\n",
        innerColor,
        borderColor,
        area->backgroundColor);

    if (radiusIndex <= RADIUS_MAX) {
        radius = radiusValues[radiusIndex];
    }
    else if (radiusIndex == RADIUS_0_PIXELS) {
        radius = 0;
    }
    else {
        // radius not supported
        LOG_WARN(
            DRAW_LOGGER, "nbgl_drawRoundedBorderedRect forbidden radius index =%d\n", radiusIndex);
        return;
    }
    rectArea.backgroundColor = innerColor;

    // Draw 1 rectangle in inner rectangle
    rectArea.x0     = area->x0;
    rectArea.y0     = area->y0;
    rectArea.width  = area->width;
    rectArea.height = area->height;
    nbgl_frontDrawRect(&rectArea);
    // special case, when border_color == inner_color == background_color, return
    if ((innerColor == borderColor) && (borderColor == area->backgroundColor)) {
        return;
    }
    // border
    // 4 rectangles (with last pixel of each corner not set)
#ifdef SCREEN_SIZE_WALLET
    uint16_t circle_width = 0;
    if (radiusIndex <= RADIUS_MAX) {
        const nbgl_icon_details_t *half_icon = PIC(radiusIcons[radiusIndex]->leftDisc);
        circle_width                         = half_icon->width;
    }
    if ((2 * circle_width) < area->width) {
        if ((area->height - stroke) > VERTICAL_ALIGNMENT) {
            // draw the 2 horizontal lines
            rectArea.height = stroke;
            rectArea.width  = area->width - 2 * circle_width;
            rectArea.x0 += circle_width;
            nbgl_frontDrawLine(&rectArea, 1, borderColor);  // top
            rectArea.y0 = area->y0 + area->height - stroke;
            nbgl_frontDrawLine(&rectArea, 1, borderColor);  // bottom
        }
        else {
            uint8_t  pattern = 0;
            uint32_t i;
            for (i = 0; i < stroke; i++) {
                pattern |= 1 << (7 - i);
            }
            for (i = area->height - stroke; i < area->height; i++) {
                pattern |= 1 << (7 - i);
            }
            memset(ramBuffer, pattern, area->width);
            rectArea.height = 8;
            rectArea.bpp    = NBGL_BPP_1;
            nbgl_frontDrawImage(&rectArea, ramBuffer, NO_TRANSFORMATION, borderColor);
        }
    }
    if ((2 * radius) < area->height) {
        rectArea.x0              = area->x0;
        rectArea.y0              = area->y0;
        rectArea.width           = stroke;
        rectArea.height          = area->height;
        rectArea.backgroundColor = area->backgroundColor;
        nbgl_frontDrawLine(&rectArea, 0, borderColor);  // left
        rectArea.x0 = area->x0 + area->width - stroke;
        nbgl_frontDrawLine(&rectArea, 0, borderColor);  // right
    }
    if (radiusIndex <= RADIUS_MAX) {
        // Draw 4 quarters of circles
        draw_circle_helper(area->x0,
                           area->y0,
                           radiusIndex,
                           LEFT_HALF,
                           borderColor,
                           innerColor,
                           area->backgroundColor);
        draw_circle_helper(area->x0 + area->width,
                           area->y0,
                           radiusIndex,
                           RIGHT_HALF,
                           borderColor,
                           innerColor,
                           area->backgroundColor);
    }
#else   // SCREEN_SIZE_WALLET
    rectArea.x0              = area->x0 + radius;
    rectArea.y0              = area->y0;
    rectArea.width           = area->width - 2 * radius;
    rectArea.height          = stroke;
    rectArea.backgroundColor = borderColor;
    nbgl_frontDrawRect(&rectArea);  // top
    rectArea.y0 = area->y0 + area->height - stroke;
    nbgl_frontDrawRect(&rectArea);  // bottom
    if ((2 * radius) < area->height) {
        rectArea.x0              = area->x0;
        rectArea.y0              = area->y0 + radius;
        rectArea.width           = stroke;
        rectArea.height          = area->height - 2 * radius;
        rectArea.backgroundColor = area->backgroundColor;
        nbgl_frontDrawLine(&rectArea, 0, borderColor);  // left
        rectArea.x0 = area->x0 + area->width - stroke;
        nbgl_frontDrawLine(&rectArea, 0, borderColor);  // right
    }

    if (radiusIndex <= RADIUS_MAX) {
        // Draw 4 quarters of circles
        draw_circle_helper(area->x0 + radius,
                           area->y0 + radius,
                           radiusIndex,
                           BAGL_FILL_CIRCLE_PI2_PI,
                           borderColor,
                           innerColor,
                           area->backgroundColor);
        draw_circle_helper(area->x0 + area->width - radius,
                           area->y0 + radius,
                           radiusIndex,
                           BAGL_FILL_CIRCLE_0_PI2,
                           borderColor,
                           innerColor,
                           area->backgroundColor);
        draw_circle_helper(area->x0 + radius,
                           area->y0 + area->height - radius,
                           radiusIndex,
                           BAGL_FILL_CIRCLE_PI_3PI2,
                           borderColor,
                           innerColor,
                           area->backgroundColor);
        draw_circle_helper(area->x0 + area->width - radius,
                           area->y0 + area->height - radius,
                           radiusIndex,
                           BAGL_FILL_CIRCLE_3PI2_2PI,
                           borderColor,
                           innerColor,
                           area->backgroundColor);
    }
#endif  // SCREEN_SIZE_WALLET
}

/**
 * @brief Helper function to render an icon directly from its `nbgl_icon_details_t` structure.
 *
 * The icon is rendered whether it's an image file or not.
 * No transformation is applied to the icon.
 *
 * @param area Area of drawing
 * @param transformation Transformation to apply to this icon (only available for raw image, not
 * image file)
 * @param color_map Color map applied to icon
 * @param icon Icon details structure to draw
 */
void nbgl_drawIcon(nbgl_area_t               *area,
                   nbgl_transformation_t      transformation,
                   nbgl_color_map_t           color_map,
                   const nbgl_icon_details_t *icon)
{
    if (icon->isFile) {
        nbgl_frontDrawImageFile(area, icon->bitmap, color_map, ramBuffer);
    }
    else {
        nbgl_frontDrawImage(area, icon->bitmap, transformation, color_map);
    }
}

/**
 * @brief Return the size of the bitmap associated to the input font and character
 *
 * @param font pointer to the font infos
 * @param charId id of the character
 */

static uint16_t get_bitmap_byte_cnt(const nbgl_font_t *font, uint8_t charId)
{
    if ((charId < font->first_char) || (charId > font->last_char)) {
        return 0;
    }

    uint16_t baseId = charId - font->first_char;
    if (charId < font->last_char) {
        const nbgl_font_character_t *character
            = (const nbgl_font_character_t *) PIC(&font->characters[baseId]);
        const nbgl_font_character_t *nextCharacter
            = (const nbgl_font_character_t *) PIC(&font->characters[baseId + 1]);
        return (nextCharacter->bitmap_offset - character->bitmap_offset);
    }
    else if (charId == font->last_char) {
        return (font->bitmap_len - font->characters[baseId].bitmap_offset);
    }
    return 0;
}

//=============================================================================
#if 1  // TMP Wip stuff
int save_png(char *framebuffer, char *fullPath, uint16_t width, uint16_t height);

static void save_packed_picture(uint8_t *packed_buffer,
                                uint16_t width,
                                uint16_t height,
                                uint16_t bpp,
                                uint32_t unicode)
{
    char     filename[32];
    uint8_t *buffer;
    sprintf(filename, "toto_0x%X", unicode);
    if ((buffer = calloc(width * height, 1)) == NULL) {
        fprintf(stderr, "Can't allocate buffer!\n");
        return;
    }
    if (bpp == NBGL_BPP_4) {
        for (uint16_t y = 0; y < height; y++) {
            for (uint16_t x = 0; x < width; x += 2) {
                uint8_t double_pixel        = *packed_buffer++;
                buffer[(y * width) + x + 0] = double_pixel >> 4;
                buffer[(y * width) + x + 1] = double_pixel & 0x0F;
            }
        }
    }
    else {
        for (uint16_t y = 0; y < height; y++) {
            for (uint16_t x = 0; x < width; x += 8) {
                uint8_t pixels = *packed_buffer++;
                uint8_t msk    = 0x80;
                for (uint16_t bit = 0; bit < 8; bit++, msk >>= 1) {
                    uint8_t pixel = 0;
                    if (pixels & msk) {
                        pixel = 0xFF;
                    }
                    buffer[(y * width) + x + bit] = pixel;
                }
            }
        }
    }
    save_png((char *) buffer, filename, width, height);
    free(buffer);
}

static void save_ultrapacked_picture(uint8_t *packed_buffer,
                                     uint16_t width,
                                     uint16_t height,
                                     uint16_t bpp,
                                     uint32_t unicode)
{
    char     filename[32];
    uint8_t *buffer;
    sprintf(filename, "roro_0x%X", unicode);
    if ((buffer = calloc(width * height, 1)) == NULL) {
        fprintf(stderr, "Can't allocate buffer!\n");
        return;
    }
    if (bpp == NBGL_BPP_4) {
        for (uint16_t y = 0; y < height; y++) {
            for (uint16_t x = 0; x < width; x += 2) {
                uint8_t double_pixel        = *packed_buffer++;
                buffer[(y * width) + x + 0] = double_pixel >> 4;
                buffer[(y * width) + x + 1] = double_pixel & 0x0F;
            }
        }
    }
    else {
        uint8_t pixels = *packed_buffer++;
        uint8_t msk    = 0x80;
        for (uint16_t y = 0; y < height; y++) {
            for (uint16_t x = 0; x < width; x++) {
                uint8_t pixel = 0;
                if (pixels & msk) {
                    pixel = 0xFF;
                }
                buffer[(y * width) + x] = pixel;
                msk >>= 1;
                if (!msk) {
                    pixels = *packed_buffer++;
                    msk    = 0x80;
                }
            }
        }
    }
    save_png((char *) buffer, filename, width, height);
    free(buffer);
}
#endif  // 1 TMP Wip stuff
//=============================================================================

/**
 * @brief Uncompress a 1BPP RLE-encoded glyph and draw it in a RAM buffer
 * (we handle transparency, meaning when resulting pixel is 0 we don't store it)
 *
 * 1BPP RLE Decoder:
 *
 * compressed bytes contains ZZZZOOOO nibbles, with
 * - ZZZZ: number of consecutives zeros (from 0 to 15)
 * - OOOO: number of consecutives ones (from 0 to 15)
 *
 * @param area area information about where to display the text
 * @param buffer buffer of RLE-encoded data
 * @param buffer_len length of buffer
 * @param buf_area of the RAM buffer
 * @param dst RAM buffer on which the glyph will be drawn
 * @param nb_skipped_bytes number of bytes that was cropped
 */
static void nbgl_draw1BPPImageRle(nbgl_area_t *area,
                                  uint8_t     *buffer,
                                  uint32_t     buffer_len,
                                  nbgl_area_t *buf_area,
                                  uint8_t     *dst,
                                  uint8_t      nb_skipped_bytes)
{
    size_t index = 0;
    // Set the initial number of transparent pixels
    size_t nb_zeros = (size_t) nb_skipped_bytes * 8;
    size_t nb_ones  = 0;
#ifdef SCREEN_SIZE_WALLET
    // Width & Height are rotated 90° on Flex/Stax
    size_t remaining_height = area->width;
    size_t remaining_width  = area->height;
#else   // SCREEN_SIZE_WALLET
    size_t remaining_height = area->height;
    size_t remaining_width  = area->width;
#endif  // SCREEN_SIZE_WALLET
    size_t  dst_offset = (buf_area->width + 7) / 8;
    size_t  dst_index  = 0;
    uint8_t pixels;
    uint8_t white_pixel;

    dst += buf_area->y0 * buf_area->width / 8;
    dst += buf_area->x0 / 8;
    white_pixel = 0x80 >> (buf_area->x0 & 7);
    pixels      = 0;

    while (remaining_height && (index < buffer_len || nb_zeros || nb_ones)) {
        // Reload nb_zeros & nb_ones if needed
        while (!nb_zeros && !nb_ones && index < buffer_len) {
            uint8_t byte = buffer[index++];
            nb_ones      = byte & 0x0F;
            nb_zeros     = byte >> 4;
        }
        // Get next pixel
        if (nb_zeros) {
            --nb_zeros;
            // Useless, but kept for clarity
            pixels |= 0;
        }
        else if (nb_ones) {
            --nb_ones;
            pixels |= white_pixel;
        }
        white_pixel >>= 1;
        if (!white_pixel) {
            white_pixel = 0x80;
            dst[dst_index++] |= pixels;  // OR because we handle transparency
            pixels = 0;
        }
        --remaining_width;

        // Have we reached the end of the line?
        if (!remaining_width) {
#ifdef SCREEN_SIZE_WALLET
            // Width & Height are rotated 90° on Flex/Stax
            remaining_width = area->height;
#else   // SCREEN_SIZE_WALLET
            remaining_width = area->width;
#endif  // SCREEN_SIZE_WALLET

            // Store current pixel content
            dst[dst_index] |= pixels;  // OR because we handle transparency

            // Start next line
            dst += dst_offset;
            dst_index   = 0;
            pixels      = 0;
            white_pixel = 0x80 >> (buf_area->x0 & 7);

            --remaining_height;
        }
    }
    // Store remaining pixels
    if (pixels) {
        dst[dst_index] |= pixels;  // OR because we handle transparency
    }
}

typedef struct {
    // Part containing the data to be decoded
    uint32_t read_cnt;
    uint32_t buffer_len;
    uint8_t *buffer;
    uint8_t  byte;
    // Part containing the decoded pixels
    uint8_t nb_pix;
    uint8_t color;      // if color <= 15 it is a FILL, else a COPY
    uint8_t pixels[6];  // Maximum 6 pixels (COPY)

} Rle_context_t;

// Get next pixel(s) and update Rle_context content
static inline void get_next_pixels(Rle_context_t *context, size_t remaining_width)
{
    // Is there still remaining data to read?
    if (context->read_cnt >= context->buffer_len) {
        // Just return the number of pixels to skip
        context->nb_pix = remaining_width;
        context->color  = 0xF;  // Background color, which is considered as transparent
        return;
    }

    // Uncompress next data
    uint8_t byte = context->buffer[context->read_cnt++];

    if (byte & 0x80) {
        if (byte & 0x40) {
            // CMD=11 + RRRRRR => FILL White (max=63+1)
            context->nb_pix = (byte & 0x3F) + 1;
            context->color  = 0x0F;
        }
        else {
            // CMD=10 + RR + VVVV + WWWWXXXX + YYYYZZZZ + QQQQ0000 : COPY Quartets x Repeat+1
            // - RR is repeat count - 3 of quartets (max=3+3 => 6 quartets)
            // - VVVV: value of 1st 4BPP pixel
            // - WWWW: value of 2nd 4BPP pixel
            // - XXXX: value of 3rd 4BPP pixel
            // - YYYY: value of 4th 4BPP pixel
            // - ZZZZ: value of 5th 4BPP pixel
            // - QQQQ: value of 6th 4BPP pixel
            context->nb_pix = ((byte & 0x30) >> 4);
            context->nb_pix += 3;
            context->pixels[0] = byte & 0x0F;  // Store VVVV
            byte               = context->buffer[context->read_cnt++];
            context->pixels[1] = byte >> 4;    // Store WWWW
            context->pixels[2] = byte & 0x0F;  // Store XXXX
            if (context->nb_pix >= 4) {
                byte               = context->buffer[context->read_cnt++];
                context->pixels[3] = byte >> 4;    // Store YYYY
                context->pixels[4] = byte & 0x0F;  // Store ZZZZ
                if (context->nb_pix >= 6) {
                    byte               = context->buffer[context->read_cnt++];
                    context->pixels[5] = byte >> 4;  // Store QQQQ
                }
            }
            context->color = 0x10;  // COPY command + pixels offset=0
        }
    }
    else {
        // CMD=0 + RRR + VVVV : FILL Value x Repeat+1 (max=7+1)
        context->nb_pix = (byte & 0x70) >> 4;
        context->nb_pix += 1;
        context->color = byte & 0x0F;
    }
}

/**
 * @brief Uncompress a 4BPP RLE-encoded glyph and draw it in a RAM buffer
 * (we handle transparency, meaning when resulting pixel is 0xF we don't store it)
 *
 * 4BPP RLE Decoder - The provided bytes contains:
 *
 *   11RRRRRR
 *   10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
 *   0RRRVVVV
 *
 *   With:
 *   * 11RRRRRR
 *       - RRRRRRR is repeat count - 1 of White (0xF) quartets (max=63+1)
 *   * 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
 *       - RR is repeat count - 3 of quartets (max=3+3 => 6 quartets)
 *       - VVVV: value of 1st 4BPP pixel
 *       - WWWW: value of 2nd 4BPP pixel
 *       - XXXX: value of 3rd 4BPP pixel
 *       - YYYY: value of 4th 4BPP pixel
 *       - ZZZZ: value of 5th 4BPP pixel
 *       - QQQQ: value of 6th 4BPP pixel
 *   * 0RRRVVVV
 *       - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
 *       - VVVV: value of the 4BPP pixel
 *
 * @param area area information about where to display the text
 * @param buffer buffer of RLE-encoded data
 * @param buffer_len length of buffer
 * @param buf_area of the RAM buffer
 * @param dst RAM buffer on which the glyph will be drawn
 * @param nb_skipped_bytes number of bytes that was cropped
 */
static void nbgl_draw4BPPImageRle(nbgl_area_t *area,
                                  uint8_t     *buffer,
                                  uint32_t     buffer_len,
                                  nbgl_area_t *buf_area,
                                  uint8_t     *dst,
                                  uint8_t      nb_skipped_bytes)
{
#ifdef SCREEN_SIZE_WALLET
    // Width & Height are rotated 90° on Flex/Stax
    size_t remaining_height = area->width;
    size_t remaining_width  = area->height;
#else   // SCREEN_SIZE_WALLET
    size_t remaining_height = area->height;
    size_t remaining_width  = area->width;
#endif  // SCREEN_SIZE_WALLET
    // size_t        dst_offset= (buf_area->width - area->width + 1) / 2;
    size_t        dst_index = 0;
    uint8_t       dst_shift = 4;  // Next pixel must be shift left 4 bit
    Rle_context_t context   = {0};
    uint8_t       dst_pixel;

    if (!buffer_len) {
        return;
    }

    context.buffer     = buffer;
    context.buffer_len = buffer_len;

    // Handle 'transparent' pixels
    if (nb_skipped_bytes) {
        context.nb_pix = nb_skipped_bytes * 2;
        context.color  = 0xF;  // Background color, which is considered as transparent
    }

    dst += buf_area->y0 * buf_area->width / 2;
    dst += buf_area->x0 / 2;
    dst_pixel = *dst;

    if (buf_area->x0 & 1) {
        dst_shift = 0;
    }
    while (remaining_height) {
        uint8_t nb_pix;

        // if the context is empty, let's fill it
        if (context.nb_pix == 0) {
            get_next_pixels(&context, remaining_width);
        }
        // Write those pixels in the RAM buffer
        nb_pix = context.nb_pix;
        if (nb_pix > remaining_width) {
            nb_pix = remaining_width;
        }

        // if color <= 0x0F it is a FILL command, else it is a COPY
        if (context.color <= 0x0F) {
            // Do we need to just skip transparent pixels?
            if (context.color == 0x0F) {
                dst[dst_index] = dst_pixel;
                dst_index += nb_pix / 2;
                if (nb_pix & 1) {
                    dst_shift ^= 4;
                    if (dst_shift) {
                        ++dst_index;
                    }
                }
                dst_pixel = dst[dst_index];
                // FILL nb_pix pixels with context.color
            }
            else {
                for (uint8_t i = 0; i < nb_pix; i++) {
                    dst_pixel &= ~(0x0F << dst_shift);
                    dst_pixel |= context.color << dst_shift;
                    dst_shift ^= 4;
                    // Do we need to go to next byte?
                    if (dst_shift) {
                        dst[dst_index] = dst_pixel;
                        ++dst_index;
                        dst_pixel = dst[dst_index];
                    }
                }
            }
        }
        else {
            uint8_t *pixels = context.pixels;
            // LSB of context.color contains the offset of the pixels to copy
            pixels += context.color & 0x0F;

            // COPY nb_pix pixels from &context.pixels[i]
            for (uint8_t i = 0; i < nb_pix; i++) {
                uint8_t color = pixels[i];
                // Handle transparency
                if (color != 0x0F) {
                    dst_pixel &= ~(0x0F << dst_shift);
                    dst_pixel |= color << dst_shift;
                }
                dst_shift ^= 4;
                // Do we need to go to next byte?
                if (dst_shift) {
                    dst[dst_index] = dst_pixel;
                    ++dst_index;
                    dst_pixel = dst[dst_index];
                }
            }
            // Update offset of the pixels to copy
            context.color += nb_pix;
        }

        // Take in account displayed pixels
        context.nb_pix -= nb_pix;
        remaining_width -= nb_pix;

        // Have we reached the end of the line?
        if (remaining_width == 0) {
#ifdef SCREEN_SIZE_WALLET
            // Width & Height are rotated 90° on Flex/Stax
            remaining_width = area->height;
#else   // SCREEN_SIZE_WALLET
            remaining_width = area->width;
#endif  // SCREEN_SIZE_WALLET

            // Store last pixels
            dst[dst_index] = dst_pixel;
            // Start next line
            dst_index = 0;
            dst += buf_area->width / 2;
            dst_pixel = dst[dst_index];
            if (buf_area->x0 & 1) {
                dst_shift = 0;
            }
            else {
                dst_shift = 4;
            }
            --remaining_height;
        }
    }
}

/**
 * @brief Uncompress a RLE-encoded glyph and draw it in a RAM buffer
 *
 * @param text_area area information about where to display the text
 * @param buffer buffer of RLE-encoded data
 * @param buffer_len length of buffer
 * @param buf_area of the RAM buffer
 * @param dst RAM buffer on which the glyph will be drawn
 * @param nb_skipped_bytes number of bytes that was cropped
 */

static void nbgl_drawImageRle(nbgl_area_t *text_area,
                              uint8_t     *buffer,
                              uint32_t     buffer_len,
                              nbgl_area_t *buf_area,
                              uint8_t     *dst,
                              uint8_t      nb_skipped_bytes)
{
    if (text_area->bpp == NBGL_BPP_4) {
        nbgl_draw4BPPImageRle(text_area, buffer, buffer_len, buf_area, dst, nb_skipped_bytes);
    }
    else if (text_area->bpp == NBGL_BPP_1) {
        nbgl_draw1BPPImageRle(text_area, buffer, buffer_len, buf_area, dst, nb_skipped_bytes);
    }
}

static void pack_ram_buffer(nbgl_area_t *area, uint16_t width, uint16_t height, uint8_t *dst)
{
    uint8_t *src = dst;

    if (area->bpp == NBGL_BPP_4) {
        uint16_t bytes_per_line;
        uint16_t skip;

        area->x0 &= 0xFFFE;  // Make sure we are on a byte boundary
        src += area->y0 * area->width / 2;
        src += area->x0 / 2;
        bytes_per_line = (width + 1) / 2;
        skip           = (area->width + 1) / 2;

        for (uint16_t y = 0; y < height; y++) {
            memmove(dst, src, bytes_per_line);
            dst += bytes_per_line;
            src += skip;
        }
    }
    else {
        uint8_t src_pixel;
        uint8_t src_shift;
        uint8_t src_index;
        uint8_t dst_pixel = 0;
        uint8_t dst_shift = 7;

        src += area->y0 * area->width / 8;
        src += area->x0 / 8;
        for (uint16_t y = 0; y < height; y++) {
            src_shift = (7 - (area->x0 & 7));
            src_pixel = *src;
            src_index = 0;
            for (uint16_t x = 0; x < width; x++) {
                dst_pixel |= ((src_pixel >> src_shift) & 1) << dst_shift;
                if (dst_shift == 0) {
                    dst_shift = 8;
                    *dst++    = dst_pixel;
                    dst_pixel = 0;
                }
                --dst_shift;

                if (src_shift == 0) {
                    src_shift = 8;
                    ++src_index;
                    src_pixel = src[src_index];
                }
                --src_shift;
            }
            src += area->width / 8;
        }
        // Write last byte, if any
        if (dst_shift != 7) {
            *dst++ = dst_pixel;
        }
    }
}

/**
 * @brief This function draws the given single-line text, with the given parameters.
 *
 * @param area position, size and background color to use for text
 * @param text array of characters (UTF-8)
 * @param textLen number of chars to draw
 * @param fontId font to be used
 * @param fontColor color to use for font
 */
nbgl_font_id_e nbgl_drawText(const nbgl_area_t *area,
                             const char        *text,
                             uint16_t           textLen,
                             nbgl_font_id_e     fontId,
                             color_t            fontColor)
{
    // text is a series of characters, each character being a bitmap
    // we need to align bitmaps on width multiple of 4 limitation.
    int16_t            x = area->x0;
    nbgl_area_t        rectArea;
    const nbgl_font_t *font = nbgl_getFont(fontId);
#ifdef HAVE_UNICODE_SUPPORT
    nbgl_unicode_ctx_t *unicode_ctx = NULL;
#endif  // HAVE_UNICODE_SUPPORT
    // Area representing the RAM buffer and in which the glyphs will be drawn
    nbgl_area_t buf_area = {0};
    // ensure that the ramBuffer also used for image file decompression is big enough
    // 4bpp: size of ram_buffer is (font->height * 2 * AVERAGE_CHAR_WIDTH) / 2
    // 1bpp: size of ram_buffer is (font->height * 2 * AVERAGE_CHAR_WIDTH) / 8
    CCASSERT(ram_buffer, (MAX_FONT_HEIGHT * AVERAGE_CHAR_WIDTH) <= GZLIB_UNCOMPRESSED_CHUNK);
    // TODO Investigate why area->bpp is not always initialized correctly
    buf_area.bpp = (nbgl_bpp_t) font->bpp;
#ifdef SCREEN_SIZE_WALLET
    // Width & Height are rotated 90° on Flex/Stax
    buf_area.width  = (font->line_height + 7) & 0xFFF8;  // Modulo 8 is better for 1BPP
    buf_area.height = 2 * AVERAGE_CHAR_WIDTH;
    if (buf_area.bpp == NBGL_BPP_4) {
        buf_area.backgroundColor = 0xF;  // This will be the transparent color
    }
    else {
        buf_area.backgroundColor = 0;  // This will be the transparent color
    }
#ifdef BUILD_SCREENSHOTS
    assert((buf_area.height * buf_area.width / 2) <= GZLIB_UNCOMPRESSED_CHUNK);
#endif  // BUILD_SCREENSHOTS
#else   // SCREEN_SIZE_WALLET
    buf_area.width           = 2 * AVERAGE_CHAR_WIDTH;
    buf_area.height          = font->line_height;
    buf_area.backgroundColor = 0;  // This will be the transparent color
#ifdef BUILD_SCREENSHOTS
    assert((buf_area.height * buf_area.width / 8) <= GZLIB_UNCOMPRESSED_CHUNK);
#endif  // BUILD_SCREENSHOTS
#endif  // SCREEN_SIZE_WALLET

    // TMP Wip Stuff
    const char *original_text = text;
    // if (!strcmp(text, "Continue setting up using the Ledger Live app")) {
    if (!strcmp(text, "Wrong word")) {
        fprintf(stdout,
                "nbgl_drawText: x0 = %d, y0 = %d, w = %d, h = %d, fontColor = %d, "
                "backgroundColor=%d, fontId=%d, text = %s\n",
                area->x0,
                area->y0,
                area->width,
                area->height,
                fontColor,
                area->backgroundColor,
                fontId,
                text);
    }

    LOG_DEBUG(DRAW_LOGGER,
              "nbgl_drawText: x0 = %d, y0 = %d, w = %d, h = %d, fontColor = %d, "
              "backgroundColor=%d, text = %s\n",
              area->x0,
              area->y0,
              area->width,
              area->height,
              fontColor,
              area->backgroundColor,
              text);

    rectArea.backgroundColor = area->backgroundColor;
    rectArea.bpp             = (nbgl_bpp_t) font->bpp;

    while (textLen > 0) {
        const nbgl_font_character_t *character;
        uint8_t                      char_width;
        uint32_t                     unicode;
        bool                         is_unicode;
        const uint8_t               *char_buffer;
        int16_t                      char_x_min;
        int16_t                      char_y_min;
        int16_t                      char_x_max;
        int16_t                      char_y_max;
        uint16_t                     char_byte_cnt;
        uint8_t                      encoding;
        uint8_t                      nb_skipped_bytes;

        unicode = nbgl_popUnicodeChar((const uint8_t **) &text, &textLen, &is_unicode);

        if (is_unicode) {
#ifdef HAVE_UNICODE_SUPPORT
            if (unicode_ctx == NULL) {
                unicode_ctx = nbgl_getUnicodeFont(fontId);
            }
            const nbgl_font_unicode_character_t *unicodeCharacter
                = nbgl_getUnicodeFontCharacter(unicode);
            // if not supported char, go to next one
            if (unicodeCharacter == NULL) {
                continue;
            }
            char_width = unicodeCharacter->width;
#if defined(HAVE_LANGUAGE_PACK)
            char_buffer = unicode_ctx->bitmap;
            char_buffer += unicodeCharacter->bitmap_offset;

            char_x_max = char_width;
            char_y_max = unicode_ctx->font->height;

            if (!unicode_ctx->font->crop) {
                // Take in account the skipped bytes, if any
                nb_skipped_bytes = (unicodeCharacter->x_min_offset & 7) << 3;
                nb_skipped_bytes |= unicodeCharacter->y_min_offset & 7;
                char_x_min = 0;
                char_y_min = 0;
            }
            else {
                nb_skipped_bytes = 0;
                char_x_min       = (uint16_t) unicodeCharacter->x_min_offset;
                char_y_min       = unicode_ctx->font->y_min;
                char_y_min += (uint16_t) unicodeCharacter->y_min_offset;
                char_x_max -= (uint16_t) unicodeCharacter->x_max_offset;
                char_y_max -= (uint16_t) unicodeCharacter->y_max_offset;
            }

            char_byte_cnt = nbgl_getUnicodeFontCharacterByteCount();
            encoding      = unicodeCharacter->encoding;
#endif  // defined(HAVE_LANGUAGE_PACK)
#else   // HAVE_UNICODE_SUPPORT
            continue;
#endif  // HAVE_UNICODE_SUPPORT
        }
        else {
            if (unicode == '\f') {
                break;
            }
            // if \b, switch fontId
            else if (unicode == '\b') {
                if (fontId == BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp) {  // switch to bold
                    fontId = BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
#ifdef HAVE_UNICODE_SUPPORT
                    unicode_ctx = NULL;
#endif  // HAVE_UNICODE_SUPPORT
                    font = (const nbgl_font_t *) nbgl_getFont(fontId);
                }
                else if (fontId == BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp) {  // switch to regular
                    fontId = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
#ifdef HAVE_UNICODE_SUPPORT
                    unicode_ctx = NULL;
#endif  // HAVE_UNICODE_SUPPORT
                    font = (const nbgl_font_t *) nbgl_getFont(fontId);
                }
                continue;
            }
            // if not supported char, go to next one
            if ((unicode < font->first_char) || (unicode > font->last_char)) {
                continue;
            }
            character = (const nbgl_font_character_t *) PIC(
                &font->characters[unicode - font->first_char]);
            char_buffer = (const uint8_t *) PIC(&font->bitmap[character->bitmap_offset]);
            char_width  = character->width;
            encoding    = character->encoding;

            char_x_max = char_width;
            char_y_max = font->height;

            if (!font->crop) {
                // Take in account the skipped bytes, if any
                nb_skipped_bytes = (character->x_min_offset & 7) << 3;
                nb_skipped_bytes |= character->y_min_offset & 7;
                char_x_min = 0;
                char_y_min = 0;
            }
            else {
                nb_skipped_bytes = 0;
                char_x_min       = (uint16_t) character->x_min_offset;
                char_y_min       = font->y_min;
                char_y_min += (uint16_t) character->y_min_offset;
                char_x_max -= (uint16_t) character->x_max_offset;
                char_y_max -= (uint16_t) character->y_max_offset;
            }

            char_byte_cnt = get_bitmap_byte_cnt(font, unicode);
        }

        // Render character
        rectArea.x0     = x + char_x_min;
        rectArea.y0     = area->y0 + char_y_min;
        rectArea.height = (char_y_max - char_y_min);
        rectArea.width  = (char_x_max - char_x_min);

        // If char_byte_cnt = 0, call nbgl_frontDrawImageRle to let speculos notice
        // a space character was 'displayed'
        if (!char_byte_cnt || encoding == 1) {
            // if (!strcmp(original_text, "clock") && fontId == BAGL_FONT_INTER_SEMIBOLD_28px_1bpp)
            // {
            if (!strcmp(original_text, "Wrong word")) {
                fprintf(stdout,
                        "Rendering '%c' at X=%d, Y=%d, W=%d, H=%d, nb_skipped_bytes=%d\n",
                        unicode,
                        rectArea.x0,
                        rectArea.y0,
                        rectArea.width,
                        rectArea.height,
                        nb_skipped_bytes);
                fprintf(stdout,
                        "x_min=%d, x_max=%d, y_min=%d, y_max=%d\n",
                        char_x_min,
                        char_x_max,
                        char_y_min,
                        char_y_max);
            }
            // To be able to handle transparency, ramBuffer must be filled with background color
            // => Fill ramBuffer with background color (0x0F for 4bpp & 0 for 1bpp)
#ifdef SCREEN_SIZE_WALLET
            if (buf_area.bpp == NBGL_BPP_4) {
                memset(ramBuffer, 0xFF, (buf_area.height * buf_area.width / 2));
            }
            else {
                memset(ramBuffer, 0x0, (buf_area.height * buf_area.width / 8));
            }
            //(X & Y are rotated 90° on Stax/Flex)
            buf_area.x0 = char_y_min;
            buf_area.y0 = AVERAGE_CHAR_WIDTH / 2;
#else   // SCREEN_SIZE_WALLET
            memset(ramBuffer, 0, (buf_area.height * buf_area.width / 8));
            buf_area.x0 = AVERAGE_CHAR_WIDTH / 2;
            buf_area.y0 = char_y_min;
#endif  // SCREEN_SIZE_WALLET
#if 0
            nbgl_frontDrawImageRle(
                &rectArea, char_buffer, char_byte_cnt, fontColor, nb_skipped_bytes);
#else
            // Draw that character in a RAM buffer
            nbgl_drawImageRle(
                &rectArea, char_buffer, char_byte_cnt, &buf_area, ramBuffer, nb_skipped_bytes);
            // TMP Wip Stuff
            static int saved      = 0;
            static int ultrasaved = 0;
            if (!strcmp(original_text, "Wrong word") && unicode == 'W' && rectArea.bpp == NBGL_BPP_4
                && !saved) {
                saved = 1;
                fprintf(stdout,
                        "Rendering '%c' at X=%d, Y=%d, W=%d, H=%d, nb_skipped_bytes=%d\n",
                        unicode,
                        rectArea.x0,
                        rectArea.y0,
                        rectArea.width,
                        rectArea.height,
                        nb_skipped_bytes);
                fprintf(
                    stdout, "ramBuffer: width=%d, height=%d\n", buf_area.width, buf_area.height);
                fflush(stdout);
                save_packed_picture(
                    ramBuffer, buf_area.width, buf_area.height, rectArea.bpp, unicode);
            }
            // Now, draw this block into the real screen
            if (char_byte_cnt) {
                // Move the data at the beginning of RAM buffer, in a packed way
                pack_ram_buffer(&buf_area, rectArea.height, rectArea.width, ramBuffer);
                // TMP Wip Stuff
                if (!strcmp(original_text, "Wrong word") && unicode == 'W'
                    && rectArea.bpp == NBGL_BPP_4 && !ultrasaved) {
                    ultrasaved = 1;
                    save_ultrapacked_picture(
                        ramBuffer, rectArea.height, rectArea.width, rectArea.bpp, unicode);
                }
                nbgl_frontDrawImage(&rectArea, ramBuffer, NO_TRANSFORMATION, fontColor);
            }
#endif  // 0
        }
        else {
            nbgl_frontDrawImage(&rectArea, char_buffer, NO_TRANSFORMATION, fontColor);
        }
        x += char_width - font->char_kerning;
    }
    return fontId;
}

#ifdef NBGL_QRCODE
#ifdef TARGET_APEX
static void push_bits(uint8_t *buffer, uint16_t current_bits, uint8_t bits, uint8_t nb_bits)
{
    uint8_t byte           = current_bits / 8;
    uint8_t remaining_bits = 8 - current_bits % 8;

    if (remaining_bits >= nb_bits) {
        // put bits in possible MSB
        buffer[byte] |= bits << (remaining_bits - nb_bits);
    }
    else {
        // manage MSB
        buffer[byte] |= bits >> (nb_bits - remaining_bits);
        nb_bits -= remaining_bits;
        // then LSB
        buffer[byte + 1] |= bits << (8 - nb_bits);
    }
}
#endif  // TARGET_APEX

static void nbgl_frontDrawQrInternal(const nbgl_area_t    *area,
                                     color_t               foregroundColor,
                                     nbgl_qrcode_version_t version)
{
    int      size = qrcodegen_getSize(qrcode);
    uint16_t idx  = 0;

    nbgl_area_t qrArea = {.x0              = area->x0,
                          .y0              = area->y0,
                          .backgroundColor = area->backgroundColor,
                          // QR codes are 1 BPP only
                          .bpp = NBGL_BPP_1};
    if (version == QRCODE_V4) {
#ifndef TARGET_APEX
        // for each point of the V4 QR code, paint 64 pixels in image (8 in width, 8 in height)
        qrArea.width  = 2;
        qrArea.height = QR_PIXEL_WIDTH_HEIGHT * 2 * size;
        // paint a column of 2*size pixels in width by 8 pixels in height
        for (int x = 0; x < size; x++) {
            idx = 0;
            for (int y = 0; y < size; y++) {
                // draw 2 columns at once
                QrDrawBuffer[idx] = qrcodegen_getModule(qrcode, x, y) ? 0xFF : 0x00;
                QrDrawBuffer[idx + QR_V4_NB_PIX_SIZE] = QrDrawBuffer[idx];
                idx += 1;
            }
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0 += 2;
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0 += 2;
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0 += 2;
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0 += 2;
        }
#else   // TARGET_APEX
        // for each point of the V4 QR code, paint 5*5 pixels in image
        qrArea.width  = 1;
        qrArea.height = 5 * size;
        for (int x = 0; x < size; x++) {
            idx = 0;
            memset(QrDrawBuffer, 0, (size * 5 + 7) / 8);
            // paint a column of 5*size pixels in width by 5 pixels in height
            for (int y = 0; y < size; y++) {
                push_bits(QrDrawBuffer, 5 * y, qrcodegen_getModule(qrcode, x, y) ? 0x1F : 0x00, 5);
            }
            for (int z = 0; z < 5; z++) {
                nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
                qrArea.x0 += 1;
            }
        }
#endif  // TARGET_APEX
    }
    else {  // V4 small or V10
        // for each point of the V10 QR code, paint 16 pixels in image (4 in width, 4 in height)
        qrArea.width  = 1;
        qrArea.height = QR_PIXEL_WIDTH_HEIGHT * size;
        // paint a line of 4*size pixels in width by 4 pixels in height
        for (int x = 0; x < size; x++) {
            idx = 0;
            memset(QrDrawBuffer, 0, (size + 1) / 2);
            for (int y = 0; y < size; y++) {
                QrDrawBuffer[idx] |= qrcodegen_getModule(qrcode, x, y) ? 0xF0 >> ((y & 1) * 4) : 0;
                if (y & 1) {
                    idx++;
                }
            }
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0++;
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0++;
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0++;
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.x0++;
        }
    }
}

/**
 * @brief Draws the given text into a V10 QR code (QR code version is fixed using
 * qrcodegen_VERSION_MIN/qrcodegen_VERSION_MAX in qrcodegen.h)
 *
 * @note y0 and height must be multiple 4 pixels, and background color is applied to 0's in 1BPP
 * bitmap.
 *
 * @param area position, size and color of the QR code to draw
 * @param version version of QR Code
 * @param text text to encode
 * @param foregroundColor color to be applied to the 1's in QR code
 */
void nbgl_drawQrCode(const nbgl_area_t    *area,
                     nbgl_qrcode_version_t version,
                     const char           *text,
                     color_t               foregroundColor)
{
    uint8_t versionNum = (version == QRCODE_V10) ? 10 : 4;
    bool    ok         = qrcodegen_encodeText(text,
                                   tempBuffer,
                                   qrcode,
                                   qrcodegen_Ecc_LOW,
                                   versionNum,
                                   versionNum,
                                   qrcodegen_Mask_AUTO,
                                   true);

    if (ok) {
        nbgl_frontDrawQrInternal(area, foregroundColor, version);
    }
    else {
        LOG_WARN(
            DRAW_LOGGER, "Impossible to draw QRCode text %s with version %d\n", text, versionNum);
    }
}
#endif  // NBGL_QRCODE
