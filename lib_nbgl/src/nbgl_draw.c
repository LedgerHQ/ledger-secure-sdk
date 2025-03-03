
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

/*********************
 *      DEFINES
 *********************/
typedef enum {
    BAGL_FILL_CIRCLE_0_PI2,
    BAGL_FILL_CIRCLE_PI2_PI,
    BAGL_FILL_CIRCLE_PI_3PI2,
    BAGL_FILL_CIRCLE_3PI2_2PI
} quarter_t;

#define QR_PIXEL_WIDTH_HEIGHT 4

#ifdef SCREEN_SIZE_WALLET
#define MAX_FONT_HEIGHT    54
#define AVERAGE_CHAR_WIDTH 40
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
typedef struct {
    const uint8_t *topLeftDisc;
    const uint8_t *bottomLeftDisc;
    const uint8_t *topLeftCircle;
    const uint8_t *bottomLeftCircle;
} radiusIcons_t;

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
    8,
    32,
    40,
    44
#else   // SCREEN_SIZE_WALLET
    1,
    3
#endif  // SCREEN_SIZE_WALLET
};

#ifdef SCREEN_SIZE_WALLET

#if COMMON_RADIUS == 40
static const radiusIcons_t radiusIcons40px = {
    C_quarter_disc_top_left_40px_1bpp_bitmap,
    C_quarter_disc_bottom_left_40px_1bpp_bitmap,
    C_quarter_circle_top_left_40px_1bpp_bitmap,
    C_quarter_circle_bottom_left_40px_1bpp_bitmap,
};
#elif COMMON_RADIUS == 44
static const radiusIcons_t radiusIcons44px = {
    C_quarter_disc_top_left_44px_1bpp_bitmap,
    C_quarter_disc_bottom_left_44px_1bpp_bitmap,
    C_quarter_circle_top_left_44px_1bpp_bitmap,
    C_quarter_circle_bottom_left_44px_1bpp_bitmap,
};
#endif
#if SMALL_BUTTON_RADIUS == 32
static const radiusIcons_t radiusIcons32px = {
    C_quarter_disc_top_left_32px_1bpp_bitmap,
    C_quarter_disc_bottom_left_32px_1bpp_bitmap,
    C_quarter_circle_top_left_32px_1bpp_bitmap,
    C_quarter_circle_bottom_left_32px_1bpp_bitmap,
};
#endif

// indexed by nbgl_radius_t (except RADIUS_0_PIXELS)
static const radiusIcons_t *radiusIcons[RADIUS_MAX + 1] = {NULL,
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
#ifdef SCREEN_SIZE_WALLET
    if (borderColor == innerColor) {
        if (quarter < BAGL_FILL_CIRCLE_PI_3PI2) {
            quarter_buffer = (const uint8_t *) PIC(radiusIcons[radiusIndex]->topLeftDisc);
        }
        else {
            quarter_buffer = (const uint8_t *) PIC(radiusIcons[radiusIndex]->bottomLeftDisc);
        }
    }
    else {
        if (quarter < BAGL_FILL_CIRCLE_PI_3PI2) {
            quarter_buffer = (const uint8_t *) PIC(radiusIcons[radiusIndex]->topLeftCircle);
        }
        else {
            quarter_buffer = (const uint8_t *) PIC(radiusIcons[radiusIndex]->bottomLeftCircle);
        }
    }
    switch (quarter) {
        case BAGL_FILL_CIRCLE_3PI2_2PI:  // bottom right
            area.x0 = x_center;
            area.y0 = y_center;
            nbgl_frontDrawImage(&area, quarter_buffer, VERTICAL_MIRROR, borderColor);
            break;
        case BAGL_FILL_CIRCLE_PI_3PI2:  // bottom left
            area.x0 = x_center - area.width;
            area.y0 = y_center;
            nbgl_frontDrawImage(&area, quarter_buffer, NO_TRANSFORMATION, borderColor);
            break;
        case BAGL_FILL_CIRCLE_0_PI2:  // top right
            area.x0 = x_center;
            area.y0 = y_center - area.width;
            nbgl_frontDrawImage(&area, quarter_buffer, VERTICAL_MIRROR, borderColor);
            break;
        case BAGL_FILL_CIRCLE_PI2_PI:  // top left
            area.x0 = x_center - area.width;
            area.y0 = y_center - area.width;
            nbgl_frontDrawImage(&area, quarter_buffer, NO_TRANSFORMATION, borderColor);
            break;
    }
#else   // SCREEN_SIZE_WALLET
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
#endif  // SCREEN_SIZE_WALLET
}

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

#ifdef SCREEN_SIZE_NANO
    if (radiusIndex == RADIUS_1_PIXEL) {
        return;
    }
#endif  // SCREEN_SIZE_NANO
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

    // special case, when border_color == inner_color == background_color, just draw a rectangle
    if ((innerColor == borderColor) && (borderColor == area->backgroundColor)) {
        rectArea.x0     = area->x0;
        rectArea.y0     = area->y0;
        rectArea.width  = area->width;
        rectArea.height = area->height;
        nbgl_frontDrawRect(&rectArea);
        return;
    }
    // Draw 3 rectangles
    if ((2 * radius) < area->width) {
        rectArea.x0     = area->x0 + radius;
        rectArea.y0     = area->y0;
        rectArea.width  = area->width - (2 * radius);
        rectArea.height = area->height;
        nbgl_frontDrawRect(&rectArea);
    }
    // special case when radius is null, left and right rectangles are not necessary
    if (radiusIndex <= RADIUS_MAX) {
        if ((2 * radius) < area->height) {
            rectArea.x0     = area->x0;
            rectArea.y0     = area->y0 + radius;
            rectArea.width  = radius;
            rectArea.height = area->height - (2 * radius);
            nbgl_frontDrawRect(&rectArea);
            rectArea.x0 = area->x0 + area->width - radius;
            rectArea.y0 = area->y0 + radius;
            nbgl_frontDrawRect(&rectArea);
        }
    }
    // border
    // 4 rectangles (with last pixel of each corner not set)
#ifdef SCREEN_SIZE_WALLET
    uint8_t maskTop, maskBottom;
    if (stroke == 1) {
        maskTop    = 0x1;
        maskBottom = 0x8;
    }
    else if (stroke == 2) {
        maskTop    = 0x3;
        maskBottom = 0xC;
    }
    else if (stroke == 3) {
        maskTop    = 0x7;
        maskBottom = 0xE;
    }
    else if (stroke == 4) {
        maskTop    = 0xF;
        maskBottom = 0xF;
    }
    else {
        LOG_WARN(DRAW_LOGGER, "nbgl_drawRoundedBorderedRect forbidden stroke=%d\n", stroke);
        return;
    }
    rectArea.x0     = area->x0 + radius;
    rectArea.y0     = area->y0;
    rectArea.width  = area->width - 2 * radius;
    rectArea.height = 4;
    nbgl_frontDrawHorizontalLine(&rectArea, maskTop, borderColor);  // top
    rectArea.x0 = area->x0 + radius;
    rectArea.y0 = area->y0 + area->height - 4;
    nbgl_frontDrawHorizontalLine(&rectArea, maskBottom, borderColor);  // bottom
#else                                                                  // SCREEN_SIZE_WALLET
    rectArea.x0              = area->x0 + radius;
    rectArea.y0              = area->y0;
    rectArea.width           = area->width - 2 * radius;
    rectArea.height          = stroke;
    rectArea.backgroundColor = borderColor;
    nbgl_frontDrawRect(&rectArea);  // top
    rectArea.y0 = area->y0 + area->height - stroke;
    nbgl_frontDrawRect(&rectArea);  // bottom
#endif                                                                 // SCREEN_SIZE_WALLET
    if ((2 * radius) < area->height) {
        rectArea.x0              = area->x0;
        rectArea.y0              = area->y0 + radius;
        rectArea.width           = stroke;
        rectArea.height          = area->height - 2 * radius;
        rectArea.backgroundColor = borderColor;
        nbgl_frontDrawRect(&rectArea);  // left
        rectArea.x0 = area->x0 + area->width - stroke;
        nbgl_frontDrawRect(&rectArea);  // right
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
    sprintf(filename, "toto_0x%X.png", unicode);
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
    sprintf(filename, "roro_0x%X.png", unicode);
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
    size_t  dst_index = 0;
    uint8_t pixels;
    uint8_t white_pixel;
    int16_t y, y_min, y_max;

#ifdef BUILD_SCREENSHOTS
    assert((buf_area->width & 7) == 0);
    assert((buf_area->height & 7) == 0);
#endif  // BUILD_SCREENSHOTS

    dst += buf_area->y0 * buf_area->width / 8;
    dst += buf_area->x0 / 8;
    white_pixel = 0x80 >> (buf_area->x0 & 7);
    pixels      = 0;

#ifdef SCREEN_SIZE_WALLET
    // Width & Height are rotated 90° on Flex/Stax
    y     = buf_area->x0;
    y_max = y;
    y_min = y + buf_area->width;
#else   // SCREEN_SIZE_WALLET
    y                       = buf_area->y0;
    y_max                   = y;
    y_min                   = y + buf_area->height;
#endif  // SCREEN_SIZE_WALLET

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
            // Update real y_min & y_max (y & height was adjusted modulo 4)
            if (pixels) {
                if (y < y_min) {
                    y_min = y;
                }
                if (y > y_max) {
                    y_max = y;
                }
            }
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
            dst += buf_area->width / 8;
            dst_index   = 0;
            pixels      = 0;
            white_pixel = 0x80 >> (buf_area->x0 & 7);

            --remaining_height;
            ++y;
        }
    }
    // Store remaining pixels
    if (pixels) {
        dst[dst_index] |= pixels;  // OR because we handle transparency
    }
    // Update real y_min & y_max (y & height was adjusted modulo 4)
    /*#ifdef SCREEN_SIZE_WALLET
        // Width & Height are rotated 90° on Flex/Stax
        area->real_y0 = y_min;
        area->real_height = y_max - y_min;
    #else   // SCREEN_SIZE_WALLET
        area->real_y0 = y_min;
        area->real_height = y_max - y_min;
        #endif  // SCREEN_SIZE_WALLET*/
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
    size_t        dst_index = 0;
    uint8_t       dst_shift = 4;  // Next pixel must be shift left 4 bit
    Rle_context_t context   = {0};
    uint8_t       dst_pixel;
    int16_t       y, y_min, y_max;

    if (!buffer_len) {
        return;
    }

#ifdef BUILD_SCREENSHOTS
    assert((buf_area->width & 7) == 0);
    assert((buf_area->height & 7) == 0);
#endif  // BUILD_SCREENSHOTS

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
#ifdef SCREEN_SIZE_WALLET
    // Width & Height are rotated 90° on Flex/Stax
    y     = buf_area->x0;
    y_max = y;
    y_min = y + buf_area->width;
#else   // SCREEN_SIZE_WALLET
    y                       = buf_area->y0;
    y_max                   = y;
    y_min                   = y + buf_area->height;
#endif  // SCREEN_SIZE_WALLET
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
                // Update real y_min & y_max (y & height was adjusted modulo 4)
                if (y < y_min) {
                    y_min = y;
                }
                if (y > y_max) {
                    y_max = y;
                }
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

            // We can consider there is at least 1 used pixel, otherwise it would be a FILL!
            // Update real y_min & y_max (y & height was adjusted modulo 4)
            if (y < y_min) {
                y_min = y;
            }
            if (y > y_max) {
                y_max = y;
            }
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
            ++y;
        }
    }
    // Update real y_min & y_max (y & height was adjusted modulo 4)
    /*#ifdef SCREEN_SIZE_WALLET
        // Width & Height are rotated 90° on Flex/Stax
        area->real_y0 = y_min;
        area->real_height = y_max - y_min;
    #else   // SCREEN_SIZE_WALLET
        area->real_y0 = y_min;
        area->real_height = y_max - y_min;
        #endif  // SCREEN_SIZE_WALLET*/
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

        src += area->y0 * area->width / 2;
        src += area->x0 / 2;
        bytes_per_line = (width + 1) / 2;
        skip           = (area->width + 1) / 2;

        // Do we need to copy quartet by quartet, or can we copy byte by byte?
        if (area->x0 & 1) {
            // We need to copy quartet by quartet
            skip -= bytes_per_line;
            for (uint16_t h = 0; h < height; h++) {
                for (uint16_t w = 0; w < bytes_per_line; w++) {
                    uint8_t byte = *src++ & 0x0F;  // 1st quartet
                    byte <<= 4;
                    byte |= *src >> 4;
                    *dst++ = byte;
                }
                src += skip;
            }
        }
        else {
            for (uint16_t y = 0; y < height; y++) {
                memmove(dst, src, bytes_per_line);
                dst += bytes_per_line;
                src += skip;
            }
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

#define COMBINED_HEIGHT 9
// #define COMBINED_HEIGHT 0

static void display_ram_buffer(int16_t      x_min,
                               int16_t      y_min,
                               int16_t      x_max,
                               int16_t      y_max,
                               nbgl_area_t *area,
                               nbgl_area_t *buf_area,
                               nbgl_area_t *char_area,
                               color_t      fontColor,
                               // Debug Info
                               char    *text,
                               uint32_t previous,
                               uint32_t current,
                               uint32_t next)
{
    static uint8_t ultrasaved = 0;
    current                   = current;
    //  Move the data at the beginning of RAM buffer, in a packed way
    buf_area->x0      = x_min;
    buf_area->y0      = y_min;
    char_area->height = ((x_max - x_min) + 3) & 0xFFFC;
    char_area->width  = y_max - y_min;

    // if (!strcmp(text, "แบตเตอรี่ต่ำจนวิกฤต")) {
    if (!strcmp(text, "Réinitialisation")) {
        fprintf(stdout,
                "Calling pack_ram_buffer '%c'(0x%X) with x0=%d, y0=%d, w=%d, h=%d, width=%d, "
                "height=%d\n",
                previous,
                previous,
                buf_area->x0,
                buf_area->y0,
                buf_area->width,
                buf_area->height,
                char_area->height,
                char_area->width);
    }
    pack_ram_buffer(buf_area, char_area->height, char_area->width, ramBuffer);

    /*if (!strcmp(text, "แบตเตอรี่ต่ำจนวิกฤต")
        && ((previous == 0x000E48 && current == 0x000E15)
            || previous == 0x000E33)) {
            ultrasaved = 1;*/
    if (!strcmp(text, "Réinitialisation") && !ultrasaved) {
        if (next == 't') {
            ultrasaved = 1;
        }
        save_ultrapacked_picture(
            ramBuffer, char_area->height, char_area->width, char_area->bpp, previous);
    }
    char_area->y0 = area->y0 + x_min - COMBINED_HEIGHT;

    /*if (!strcmp(text, "แบตเตอรี่ต่ำจนวิกฤต")
      && ((previous == 0x000E48 && current == 0x000E15) || previous == 0x000E33)) {*/
    if (!strcmp(text, "Réinitialisation")) {
        fprintf(stdout,
                "Displaying RAM buffer '%c'(0x%X) at x0=%d, y0=%d, w=%d, h=%d\n",
                previous,
                previous,
                char_area->x0,
                char_area->y0,
                char_area->height,
                char_area->width);
    }
    nbgl_frontDrawImage(char_area, ramBuffer, NO_TRANSFORMATION, fontColor);
}

// Structure used to hold character information

typedef struct character_info {
    uint32_t       unicode;
    const uint8_t *buffer;
    uint16_t       byte_cnt;
    int16_t        x_min;
    int16_t        y_min;
    int16_t        x_max;
    int16_t        y_max;
    uint16_t       width;
    uint16_t       height;
    int16_t        real_y_min;
    uint16_t       real_height;
    uint8_t        encoding;
    uint8_t        nb_skipped_bytes;
    bool           is_unicode;
    bool           over_previous;

} CHARACTER_INFO;

static void update_char_info(CHARACTER_INFO     *char_info,
                             const uint8_t     **text,
                             uint16_t           *textLen,
                             nbgl_unicode_ctx_t *unicode_ctx,
                             const nbgl_font_t  *font)
{
    char_info->unicode = nbgl_popUnicodeChar(text, textLen, &char_info->is_unicode);

    // Do we still have some characters to read?
    if (!char_info->unicode) {
        return;
    }

    // Retrieves information depending on whether it is an ASCII character or not.
    if (char_info->is_unicode) {
        const nbgl_font_unicode_character_t *unicodeCharacter
            = nbgl_getUnicodeFontCharacter(char_info->unicode);
        // if not supported char, go to next one (this should never happen!!)
        if (unicodeCharacter == NULL) {
#ifdef BUILD_SCREENSHOTS
            fprintf(stdout,
                    "Inside update_char_info, unicode (%c)[0x%X] is not supported!\n",
                    char_info->unicode,
                    char_info->unicode);
#endif  // BUILD_SCREENSHOTS
            update_char_info(char_info, text, textLen, unicode_ctx, font);
            return;
        }
        char_info->width = unicodeCharacter->width;
#if defined(HAVE_LANGUAGE_PACK)
        char_info->buffer = unicode_ctx->bitmap;
        char_info->buffer += unicodeCharacter->bitmap_offset;

        char_info->x_max         = char_info->width;
        char_info->y_max         = unicode_ctx->font->height;
        char_info->over_previous = unicodeCharacter->over_previous;

        if (!unicode_ctx->font->crop) {
            // Take in account the skipped bytes, if any
            char_info->nb_skipped_bytes = (unicodeCharacter->x_min_offset & 7) << 3;
            char_info->nb_skipped_bytes |= unicodeCharacter->y_min_offset & 7;
            char_info->x_min = 0;
            char_info->y_min = 0;
        }
        else {
            char_info->nb_skipped_bytes = 0;
            char_info->y_min            = unicode_ctx->font->y_min;
            char_info->y_min += (uint16_t) unicodeCharacter->y_min_offset;
            char_info->y_max -= (uint16_t) unicodeCharacter->y_max_offset;

            if (char_info->over_previous) {
                // That character will be displayed over the previous one: get correct X coords
                char_info->x_min = -(int16_t) unicodeCharacter->width;
                char_info->width = 16 * (uint16_t) unicodeCharacter->x_min_offset;
                char_info->width += (uint16_t) unicodeCharacter->x_max_offset;
                char_info->x_max = char_info->x_min + char_info->width;
            }
            else {
                char_info->x_min = (uint16_t) unicodeCharacter->x_min_offset;
                char_info->x_max -= (uint16_t) unicodeCharacter->x_max_offset;
            }
        }
        char_info->byte_cnt = nbgl_getUnicodeFontCharacterByteCount();
        char_info->encoding = unicodeCharacter->encoding;
        char_info->height   = char_info->y_max - char_info->y_min;
#endif  // defined(HAVE_LANGUAGE_PACK)
    }
    else {
        // Special character: nothing special to do here
        if (char_info->unicode == '\f' || char_info->unicode == '\b') {
            return;
        }
        // if not supported char, go to next one (this should never happen!!)
        if ((char_info->unicode < font->first_char) || (char_info->unicode > font->last_char)) {
#ifdef BUILD_SCREENSHOTS
            fprintf(stdout,
                    "Inside update_char_info, unicode (%c)[0x%X] is not supported!\n",
                    char_info->unicode,
                    char_info->unicode);
#endif  // BUILD_SCREENSHOTS
            update_char_info(char_info, text, textLen, unicode_ctx, font);
            return;
        }
        const nbgl_font_character_t *character = (const nbgl_font_character_t *) PIC(
            &font->characters[char_info->unicode - font->first_char]);

        char_info->buffer        = (const uint8_t *) PIC(&font->bitmap[character->bitmap_offset]);
        char_info->width         = character->width;
        char_info->encoding      = character->encoding;
        char_info->over_previous = 0;

        char_info->x_max = char_info->width;
        char_info->y_max = font->height;

        if (!font->crop) {
            // Take in account the skipped bytes, if any
            char_info->nb_skipped_bytes = (character->x_min_offset & 7) << 3;
            char_info->nb_skipped_bytes |= character->y_min_offset & 7;
            char_info->x_min = 0;
            char_info->y_min = 0;
        }
        else {
            char_info->nb_skipped_bytes = 0;
            char_info->x_min            = (uint16_t) character->x_min_offset;
            char_info->y_min            = font->y_min;
            char_info->y_min += (uint16_t) character->y_min_offset;
            char_info->x_max -= (uint16_t) character->x_max_offset;
            char_info->y_max -= (uint16_t) character->y_max_offset;
        }

        char_info->byte_cnt = get_bitmap_byte_cnt(font, char_info->unicode);
        char_info->height   = char_info->y_max - char_info->y_min;
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
    int16_t            x      = area->x0;
    int16_t            next_x = x;
    nbgl_area_t        current_area, previous_area;
    int16_t            buf_x_min;
    int16_t            buf_y_min;
    int16_t            buf_x_max;
    int16_t            buf_y_max;
    CHARACTER_INFO     previous_char = {0};
    CHARACTER_INFO     current_char  = {0};
    CHARACTER_INFO     next_char     = {0};
    const nbgl_font_t *font          = nbgl_getFont(fontId);
#ifdef HAVE_UNICODE_SUPPORT
    nbgl_unicode_ctx_t *unicode_ctx = nbgl_getUnicodeFont(fontId);
#endif  // HAVE_UNICODE_SUPPORT
    // Flag set to 1 when there is data to be drawn in RAM buffer
    uint8_t redraw_buf_area = 0;

    //  Area representing the RAM buffer and in which the glyphs will be drawn
    nbgl_area_t buf_area = {0};
    // ensure that the ramBuffer also used for image file decompression is big enough
    // 4bpp: size of ram_buffer is (font->height * 3*AVERAGE_CHAR_WIDTH/2) / 2
    // 1bpp: size of ram_buffer is (font->height * 3*AVERAGE_CHAR_WIDTH/2) / 8
    CCASSERT(ram_buffer,
             (MAX_FONT_HEIGHT * 3 * AVERAGE_CHAR_WIDTH / 4) <= GZLIB_UNCOMPRESSED_CHUNK);
    // TODO Investigate why area->bpp is not always initialized correctly
    buf_area.bpp = (nbgl_bpp_t) font->bpp;
#ifdef SCREEN_SIZE_WALLET
    // Width & Height are rotated 90° on Flex/Stax
    buf_area.width  = (unicode_ctx->font->line_height + 7) & 0xFFF8;  // Modulo 8 is better for 1BPP
    buf_area.height = ((3 * AVERAGE_CHAR_WIDTH / 2) + 7) & 0xFFF8;
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
    buf_area.width           = 3 * AVERAGE_CHAR_WIDTH / 2;
    buf_area.height          = unicode_ctx->font->line_height;
    buf_area.backgroundColor = 0;  // This will be the transparent color
#ifdef BUILD_SCREENSHOTS
    assert((buf_area.height * buf_area.width / 8) <= GZLIB_UNCOMPRESSED_CHUNK);
#endif  // BUILD_SCREENSHOTS
#endif  // SCREEN_SIZE_WALLET

    // Those variables will be updated with current_char dimension
    buf_x_min = AVERAGE_CHAR_WIDTH;
    buf_y_min = MAX_FONT_HEIGHT;
    buf_x_max = 0;
    buf_y_max = 0;

#define THE_STRING "ป๋าพื่กรอกรหัส PIN ของคุณ"
    // TMP Wip Stuff
    const char *original_text = text;
    if (!strcmp(original_text, THE_STRING)) {
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

    current_area.backgroundColor = area->backgroundColor;
    current_area.bpp             = (nbgl_bpp_t) font->bpp;

    // Get the first character, that will go into 'current_char'
    // (to correctly handle languages using 'combined characters', we need to
    // know current, previous & next characters)
    update_char_info(&next_char, (const uint8_t **) &text, &textLen, unicode_ctx, font);

    while (textLen > 0 || next_char.unicode) {
        // Get the character we already read
        current_char = next_char;
        // Get info for next character
        update_char_info(&next_char, (const uint8_t **) &text, &textLen, unicode_ctx, font);

#ifndef SCREEN_SIZE_WALLET
        // Handle special characters (for LNX & LNS+ devices)
        if (!current_char.is_unicode) {
            // If '\f', that string si completed
            if (current_char.unicode == '\f') {
                break;
            }
            // if \b, switch fontId
            else if (current_char.unicode == '\b') {
                if (fontId == BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp) {  // switch to bold
                    fontId      = BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
                    unicode_ctx = nbgl_getUnicodeFont(fontId);
                    font        = (const nbgl_font_t *) nbgl_getFont(fontId);
                }
                else if (fontId == BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp) {  // switch to regular
                    fontId      = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
                    unicode_ctx = nbgl_getUnicodeFont(fontId);
                    font        = (const nbgl_font_t *) nbgl_getFont(fontId);
                }
                continue;
            }
        }
#endif  // SCREEN_SIZE_WALLET

        // Render current character in current_area
        if (!current_char.over_previous) {
            // This character will not be displayed over previous one => update x
            if (next_x > x) {
                x = next_x;
            }
            current_area.x0 = x + current_char.x_min;
        }
        current_area.y0     = area->y0 + current_char.y_min;
        current_area.height = (current_char.y_max - current_char.y_min);
        current_area.width  = (current_char.x_max - current_char.x_min);

        if (!strcmp(original_text, THE_STRING)) {
            fprintf(stdout,
                    "Will render '%c'(0x0%X) (x=%d, width=%d, next_x=%d) at x0=%d, y0=%d, W=%d, "
                    "H=%d\n",
                    current_char.unicode,
                    current_char.unicode,
                    x,
                    current_char.width,
                    next_x,
                    current_area.x0,
                    current_area.y0,
                    current_area.width,
                    current_area.height);
            fprintf(stdout,
                    "OP=%d, x_min=%d, x_max=%d, y_min=%d, y_max=%d\n",
                    current_char.over_previous,
                    current_char.x_min,
                    current_char.x_max,
                    current_char.y_min,
                    current_char.y_max);
        }
        // If char_byte_cnt = 0, call nbgl_frontDrawImageRle to let speculos notice
        // a space character was 'displayed'
        if (!current_char.byte_cnt || current_char.encoding == 1) {
            // TMP Wip Stuff
            static uint8_t saved      = 0;
            static uint8_t ultrasaved = 0;

            if (!strcmp(original_text, THE_STRING)) {
                fprintf(stdout,
                        "Rendering '%c'(0x%X) at X=%d, Y=%d, W=%d, H=%d, nb_skipped_bytes=%d\n",
                        current_char.unicode,
                        current_char.unicode,
                        current_area.x0,
                        current_area.y0,
                        current_area.width,
                        current_area.height,
                        current_char.nb_skipped_bytes);
                fprintf(stdout,
                        "OP=%d, x_min=%d, x_max=%d, y_min=%d, y_max=%d\n",
                        current_char.over_previous,
                        current_char.x_min,
                        current_char.x_max,
                        current_char.y_min,
                        current_char.y_max);
            }
            // if current character should not be displayed over previous one,
            // send RAM buffer to display
            if (!current_char.over_previous) {
                // Send RAM buffer content (previous character) to display
                if (redraw_buf_area) {
                    display_ram_buffer(buf_x_min,
                                       buf_y_min,
                                       buf_x_max,
                                       buf_y_max,
                                       area,
                                       &buf_area,
                                       &previous_area,
                                       fontColor,
                                       // Debug Info
                                       original_text,
                                       previous_char.unicode,
                                       current_char.unicode,
                                       next_char.unicode);
                    // Reset that flag
                    redraw_buf_area = 0;
                    buf_x_min       = AVERAGE_CHAR_WIDTH;
                    buf_y_min       = MAX_FONT_HEIGHT;
                    buf_x_max       = 0;
                    buf_y_max       = 0;
                }
#ifdef SCREEN_SIZE_WALLET
                // To handle transparency, ramBuffer must be filled with background color
                // => Fill ramBuffer with background color (0x0F for 4bpp & 0 for 1bpp)
                if (buf_area.bpp == NBGL_BPP_4) {
                    memset(ramBuffer, 0xFF, (buf_area.height * buf_area.width / 2));
                }
                else {
                    memset(ramBuffer, 0x0, (buf_area.height * buf_area.width / 8));
                }

                // Update display coordinates of current char inside the RAM buffer
                //(X & Y are rotated 90° on Stax/Flex)
                buf_area.x0 = COMBINED_HEIGHT + current_char.y_min;
                buf_area.y0 = (AVERAGE_CHAR_WIDTH / 2) + current_char.x_min;

                buf_x_min = buf_area.x0;
                buf_x_max = buf_x_min + current_area.height;
                buf_y_min = buf_area.y0;
                buf_y_max = buf_y_min + current_area.width;
#else   // SCREEN_SIZE_WALLET
        // To handle transparency, ramBuffer must be filled with background color
        // => Fill ramBuffer with background color (0 for 1bpp)
                memset(ramBuffer, 0, (buf_area.height * buf_area.width / 8));
                // Update display coordinates of current char inside the RAM buffer
                buf_area.x0 = AVERAGE_CHAR_WIDTH / 2;
                buf_area.y0 = COMBINED_HEIGHT + current_char.y_min;
                buf_x_min   = buf_area.x0;
                buf_x_max   = buf_x_min + current_area.width;
                buf_y_min   = buf_area.y0;
                buf_y_max   = buf_y_min + current_area.height;
#endif  // SCREEN_SIZE_WALLET
        // Compute next x with current char width, to be ready to display next one
                x += current_char.width - font->char_kerning;
                if (!strcmp(original_text, THE_STRING)) {
                    fprintf(stdout,
                            "Increasing next_x from %d to %d (char_width=%d, kerning=%d, "
                            "unicode='%c'(0x0%X))\n",
                            next_x,
                            x,
                            current_char.width,
                            font->char_kerning,
                            current_char.unicode,
                            current_char.unicode);
                }
                next_x = x;
            }
            else {
                // Security check: first character can't be a composed one!
                if (previous_char.unicode == 0) {
#ifdef BUILD_SCREENSHOTS
                    fprintf(stdout,
                            "ERROR: First character '%c'(0x%X) is a composed one! (text=>%s<=)\n",
                            current_char.unicode,
                            current_char.unicode,
                            original_text);
#endif  // BUILD_SCREENSHOTS
                    continue;
                }
                // current_area.x0 = x + current_char.x_min;
                // previous_area.x0 = x + current_char.x_min;

                // Update next_x if this character is larger than original one
                // (except for vowel 0xE31 which is overflowing on purpose on right side)
                if (current_char.unicode != 0x00E31
                    && (x + current_char.x_min + current_char.width) > next_x) {
                    if (!strcmp(original_text, THE_STRING)) {
                        fprintf(stdout,
                                "Increasing(2) next_x from %d to %d (char_width=%d, char_x_min=%d, "
                                "unicode='%c'(0x0%X))\n",
                                next_x,
                                x + current_char.width + current_char.x_min,
                                current_char.width,
                                current_char.x_min,
                                current_char.unicode,
                                current_char.unicode);
                    }
                    next_x = x + current_char.x_min + current_char.width;
                    // That character is a special one, as it is displayed on top of previous
                    // character and also on its right => give space for next character
                    if (current_char.unicode == 0x00E33) {
                        next_x += 1;
                    }
                }
                // Take in account current x_min (which is < 0)
#ifdef SCREEN_SIZE_WALLET
                buf_area.x0 = COMBINED_HEIGHT + current_char.y_min;
                buf_area.y0 = (AVERAGE_CHAR_WIDTH / 2) - current_char.x_min - current_char.width;

                // Thai rules for displaying characters on top of each others
                // Order priority, from bottom to top
                // 1 - consonnant
                // 2 - vowel or sign 0x0E4D and 0x0E4E
                // 3 - tone 0x0E48, 0x0E49, 0x0E4A and 0x0E4B or sign 0x0E4C
                // => 0x0E48, 0x0E49, 0x0E4A, 0x0E4B & 0x0E4C MUST ALWAYS be displayed on top of
                // other characters! WARNING: some vowels, signs or tone may have to be displayed
                // left with some consonnant (0E1B, 0E1F etc)

                if (current_char.unicode >= 0x0E48 && current_char.unicode <= 0x0E4C) {
                    if (next_char.unicode == 0x00E33) {
                        // Display current character up to next one
                        buf_area.x0 = COMBINED_HEIGHT + 0;  // TODO should be next_area.ymin
                        buf_area.x0 -= (current_char.y_max - current_char.y_min)
                                       - 2;  // minus height of cur char
                        // TMP
                        /*if (current_char.unicode == 0x0E48) {
                            buf_area.x0 += 2;
                            }*/
                    }
                    else if (previous_char.unicode >= 0x0E31 && previous_char.unicode <= 0x0E37) {
                        // Take in account the height of previous character
                        // buf_area.x0 -= previous_area.height - 2;
                        buf_area.x0 = buf_x_min - (current_char.y_max - current_char.y_min) - 1;
                        // TMP
                        /*if (current_char.unicode == 0x0E48) {
                            buf_area.x0 += 4;
                            }*/
                    }
                    else if (current_char.unicode == 0x0E4B && previous_char.unicode == 0x0E1B) {
                        // We must shift 0x0E4B to the left or we will overwrite 0x0E1B
                        buf_area.y0 -= current_char.x_min / 2;
                    }
                }
                // Update RAM buffer x/y min/max
                if (buf_area.x0 < buf_x_min) {
                    buf_x_min = buf_area.x0;
                }
                if ((buf_area.x0 + current_area.height) > buf_x_max) {
                    buf_x_max = buf_area.x0 + current_area.height;
                }
                if (buf_area.y0 < buf_y_min) {
                    buf_y_min = buf_area.y0;
                }
                if ((buf_area.y0 + current_area.width) > buf_y_max) {
                    buf_y_max = buf_area.y0 + current_area.width;
                }
#else   // SCREEN_SIZE_WALLET
                buf_area.x0 += previous_char.width - current_char.x_min;

                // Thai rules for displaying characters on top of each others
                // Order priority, from bottom to top
                // 1 - consonnant
                // 2 - vowel or sign 0x0E4D and 0x0E4E
                // 3 - tone 0x0E48, 0x0E49, 0x0E4A and 0x0E4B or sign 0x0E4C
                // => 0x0E48, 0x0E49, 0x0E4A, 0x0E4B & 0x0E4C MUST ALWAYS be displayed on top of
                // other characters! WARNING: some vowels, signs or tone may have to be displayed
                // left with some consonnant (0E1B, 0E1F etc)

                if (current_char.unicode >= 0x0E48 && current_char.unicode <= 0x0E4C
                    && previous_char.unicode >= 0x0E31 && previous_char.unicode <= 0x0E37) {
                    // Take in account the height of previous character
                    buf_area.x0 -= previous_area.height - 2;
                }
                // Update RAM buffer x/y min/max
                if (buf_area.x0 < buf_x_min) {
                    buf_x_min = buf_area.x0;
                }
                if ((buf_area.x0 + current_area.width) > buf_x_max) {
                    buf_x_max = buf_area.x0 + current_area.width;
                }
                if (buf_area.y0 < buf_y_min) {
                    buf_y_min = buf_area.y0;
                }
                if ((buf_area.y0 + current_area.height) > buf_y_max) {
                    buf_y_max = buf_area.y0 + current_area.height;
                }
#endif  // SCREEN_SIZE_WALLET

                // Update current_area if needed
                /*
                if ((area->y0 + current_char.y_min) < current_area.y0) {
                    current_area.y0 = area->y0 + current_char.y_min;
                }
                if ((current_char.y_max - current_char.y_min) > current_area.height) {
                    current_area.height = (current_char.y_max - current_char.y_min);
                }
                if ((current_char.x_max - current_char.x_min) > current_area.width) {
                    current_area.width = (current_char.x_max - current_char.x_min);
                    }*/
            }
            if (!strcmp(original_text, THE_STRING) && current_char.unicode >= 0x000E00
                && current_char.unicode < 0x000E80) {
                fprintf(stdout,
                        "Drawing in RAM buffer '%c'(0x%X) at X=%d, Y=%d, W=%d, H=%d\n",
                        current_char.unicode,
                        current_char.unicode,
                        current_area.x0,
                        current_area.y0,
                        current_area.width,
                        current_area.height);
                fprintf(stdout,
                        "RAM buffer area: X=%d, Y=%d, W=%d, H=%d\n",
                        buf_area.x0,
                        buf_area.y0,
                        buf_area.width,
                        buf_area.height);
                fprintf(stdout,
                        "buf_x/y_min/max: buf_x_min=%d, buf_x_max=%d, buf_y_min=%d, buf_y_max=%d\n",
                        buf_x_min,
                        buf_x_max,
                        buf_y_min,
                        buf_y_max);
            }
#if 0
            nbgl_frontDrawImageRle(
                &current_area, current_char.buffer, current_char.byte_cnt, fontColor, nb_skipped_bytes);
#else
            // Draw that character in the RAM buffer, with transparency
            nbgl_drawImageRle(&current_area,
                              current_char.buffer,
                              current_char.byte_cnt,
                              &buf_area,
                              ramBuffer,
                              current_char.nb_skipped_bytes);
            // WARNING: current_area was adjusted to height of what was really displayed!

            // Set the flag telling that RAM buffer need to be displayed, if needed
            if (current_char.byte_cnt) {
                redraw_buf_area = 1;
                previous_area   = current_area;
            }

            if (!strcmp(original_text, THE_STRING) && current_char.over_previous) {
                // if (!strcmp(original_text, "Réinitialisation")) {
                fprintf(stdout,
                        "Real coords for '%c' (0x%X) : y0=%d, height=%d\n",
                        current_char.unicode,
                        current_char.unicode,
                        current_char.real_y_min,
                        current_char.real_height);
            }
            if (!strcmp(original_text, THE_STRING) && current_char.unicode == 0x000E4B
                && next_char.unicode == 0x000E32 && previous_char.unicode == 0x000E1B) {
                /*if (current_char.unicode == 0x000E14) {
                    saved = 1;
                    }*/
                saved = 1;
                fprintf(stdout,
                        "Saving packed picture '%c' (0x%X) at X=%d, Y=%d, W=%d, H=%d, "
                        "buf_x_min=%d, buf_y_min=%d, nb_skipped_bytes=%d\n",
                        current_char.unicode,
                        current_char.unicode,
                        buf_area.x0,
                        buf_area.y0,
                        buf_area.width,
                        buf_area.height,
                        buf_x_min,
                        buf_y_min,
                        current_char.nb_skipped_bytes);
                fprintf(
                    stdout, "ramBuffer: width=%d, height=%d\n", buf_area.width, buf_area.height);
                fflush(stdout);
                save_packed_picture(ramBuffer,
                                    buf_area.width,
                                    buf_area.height,
                                    current_area.bpp,
                                    current_char.unicode);
            }
#endif  // 0
        }
        else {
            nbgl_frontDrawImage(&current_area, current_char.buffer, NO_TRANSFORMATION, fontColor);
        }
        previous_char = current_char;
    }
    // Do we need to send RAM buffer content (previous character) to display?
    if (redraw_buf_area) {
        // Move the data at the beginning of RAM buffer, in a packed way
        display_ram_buffer(buf_x_min,
                           buf_y_min,
                           buf_x_max,
                           buf_y_max,
                           area,
                           &buf_area,
                           &previous_area,
                           fontColor,
                           // Debug Info
                           original_text,
                           previous_char.unicode,
                           current_char.unicode,
                           next_char.unicode);
    }
    return fontId;
}

#ifdef NBGL_QRCODE
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
    }
    else {  // V4 small or V10
        // for each point of the V10 QR code, paint 16 pixels in image (4 in width, 4 in height)
        qrArea.width  = QR_PIXEL_WIDTH_HEIGHT * size;
        qrArea.height = QR_PIXEL_WIDTH_HEIGHT;
        // paint a line of 4*size pixels in width by 4 pixels in height
        for (int y = 0; y < size; y++) {
            idx = 0;
            for (int x = 0; x < size; x++) {
                memset(&QrDrawBuffer[idx], qrcodegen_getModule(qrcode, x, y) ? 0xFF : 0x00, 2);
                idx += 2;
            }
            nbgl_frontDrawImage(&qrArea, QrDrawBuffer, NO_TRANSFORMATION, foregroundColor);
            qrArea.y0 += QR_PIXEL_WIDTH_HEIGHT;
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
