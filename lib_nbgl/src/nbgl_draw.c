
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
                    unicode_ctx = nbgl_getUnicodeFont(fontId);
#endif  // HAVE_UNICODE_SUPPORT
                    font = (const nbgl_font_t *) nbgl_getFont(fontId);
                }
                else if (fontId == BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp) {  // switch to regular
                    fontId = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
#ifdef HAVE_UNICODE_SUPPORT
                    unicode_ctx = nbgl_getUnicodeFont(fontId);
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
        // Draw that character in a RAM buffer
            nbgl_drawImageRle(
                &rectArea, char_buffer, char_byte_cnt, &buf_area, ramBuffer, nb_skipped_bytes);

            // Now, draw this block into the real screen
            if (char_byte_cnt) {
                // Move the data at the beginning of RAM buffer, in a packed way
                pack_ram_buffer(&buf_area, rectArea.height, rectArea.width, ramBuffer);
                nbgl_frontDrawImage(&rectArea, ramBuffer, NO_TRANSFORMATION, fontColor);
            }
        }
        else {
            nbgl_frontDrawImage(&rectArea, char_buffer, NO_TRANSFORMATION, fontColor);
        }
        x += char_width - font->char_kerning;
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
