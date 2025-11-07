
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
#include "nbgl_debug.h"
#include "nbgl_side.h"
#ifdef NBGL_QRCODE
#include "qrcodegen.h"
#endif  // NBGL_QRCODE
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

/****************
 *  STRUCTURES
 ****************/

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
