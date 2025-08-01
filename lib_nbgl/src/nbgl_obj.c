/**
 * @file nbgl_obj.c
 * @brief Implementation of API to draw all basic graphic objects
 */

/*********************
 *      INCLUDES
 *********************/
#include <string.h>

#include "app_config.h"
#include "nbgl_obj.h"
#include "nbgl_draw.h"
#include "nbgl_front.h"
#include "nbgl_debug.h"
#include "nbgl_screen.h"
#include "nbgl_touch.h"
#include "os_print.h"
#include "os_helpers.h"
#include "os_pic.h"
#include "os_task.h"
#include "glyphs.h"
#include "os_io_seph_ux.h"
#ifdef HAVE_SERIALIZED_NBGL
#include "nbgl_serialize.h"
#endif
#ifdef BUILD_SCREENSHOTS
#include "json_scenario.h"
#endif  // BUILD_SCREENSHOTS

/*********************
 *      DEFINES
 *********************/
// max number of letters in TEXT_ENTRY
#define NB_MAX_LETTERS 9

#if defined(TARGET_STAX)
#define INTER_DASHES        10
#define SPINNER_DASH_WIDTH  22
#define SPINNER_DASH_HEIGHT 14
#define SPINNER_DASH_STROKE 4
#define PROGRESS_STROKE     3
#elif defined(TARGET_FLEX)
#define INTER_DASHES        8
#define SPINNER_DASH_WIDTH  24
#define SPINNER_DASH_HEIGHT 16
#define SPINNER_DASH_STROKE 4
#define PROGRESS_STROKE     3
#elif defined(TARGET_APEX)
#define INTER_DASHES        8
#define SPINNER_DASH_WIDTH  15
#define SPINNER_DASH_HEIGHT 10
#define SPINNER_DASH_STROKE 2
#define PROGRESS_STROKE     2
#endif  // TARGETS

/**********************
 *      TYPEDEFS
 **********************/
typedef void (*draw_function_t)(nbgl_obj_t *obj, nbgl_obj_t *prevObj, bool computePosition);

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void draw_screen(nbgl_container_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_container(nbgl_container_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_image(nbgl_image_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_button(nbgl_button_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#ifdef HAVE_SE_TOUCH
static void draw_line(nbgl_line_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_switch(nbgl_switch_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_radioButton(nbgl_radio_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_pageIndicator(nbgl_page_indicator_t *obj,
                               nbgl_obj_t            *prevObj,
                               bool                   computePosition);
static void draw_spinner(nbgl_spinner_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#else   // HAVE_SE_TOUCH
static void draw_textEntry(nbgl_text_entry_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#endif  // HAVE_SE_TOUCH
static void draw_progressBar(nbgl_progress_bar_t *obj, nbgl_obj_t *prevObj, bool computePosition);
static void draw_textArea(nbgl_text_area_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#ifdef NBGL_QRCODE
static void draw_qrCode(nbgl_qrcode_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#endif  // NBGL_QRCODE
#ifdef NBGL_KEYBOARD
static void draw_keyboard(nbgl_keyboard_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#endif  // NBGL_KEYBOARD
#ifdef NBGL_KEYPAD
static void draw_keypad(nbgl_keypad_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#endif  // NBGL_KEYPAD
static void draw_image_file(nbgl_image_file_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#ifdef NBGL_MASKING
static void draw_mask_control(nbgl_mask_control_t *obj, nbgl_obj_t *prevObj, bool computePosition);
#endif  // NBGL_MASKING

static void extendRefreshArea(nbgl_area_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
// area to resfresh
static nbgl_area_t refreshArea;

// boolean used to enable/forbid drawing/refresh by Application
static bool objDrawingDisabled;

// boolean used to indicate a manual refresh area extension
static bool objRefreshAreaDone;

// indexed by nbgl_obj_type_t
static const draw_function_t draw_functions[NB_OBJ_TYPES] = {
    [SCREEN]    = (draw_function_t) draw_screen,
    [CONTAINER] = (draw_function_t) draw_container,
    [IMAGE]     = (draw_function_t) draw_image,
    [TEXT_AREA] = (draw_function_t) draw_textArea,
    [BUTTON]    = (draw_function_t) draw_button,
#ifdef HAVE_SE_TOUCH
    [LINE]           = (draw_function_t) draw_line,
    [SWITCH]         = (draw_function_t) draw_switch,
    [PAGE_INDICATOR] = (draw_function_t) draw_pageIndicator,
    [RADIO_BUTTON]   = (draw_function_t) draw_radioButton,
#ifdef NBGL_QRCODE
    [QR_CODE] = (draw_function_t) draw_qrCode,
#endif  // NBGL_QRCODE
    [SPINNER] = (draw_function_t) draw_spinner,
#endif  // HAVE_SE_TOUCH
    [PROGRESS_BAR] = (draw_function_t) draw_progressBar,
#ifdef NBGL_KEYBOARD
    [KEYBOARD] = (draw_function_t) draw_keyboard,
#endif  // NBGL_KEYBOARD
#ifdef NBGL_KEYPAD
    [KEYPAD] = (draw_function_t) draw_keypad,
#endif  // NBGL_KEYPAD
    [IMAGE_FILE] = (draw_function_t) draw_image_file,
#ifndef HAVE_SE_TOUCH
    [TEXT_ENTRY] = (draw_function_t) draw_textEntry,
#endif  // HAVE_SE_TOUCH
#ifdef NBGL_MASKING
    [MASK_CONTROL] = (draw_function_t) draw_mask_control,
#endif  // NBGL_MASKING
};
/**********************
 *      VARIABLES
 **********************/
// buffer used either for image file uncompression and side screen string drawing
uint8_t ramBuffer[GZLIB_UNCOMPRESSED_CHUNK];

#ifdef BUILD_SCREENSHOTS
// Variables used to store important values (nb lines, bold state etc)
extern uint16_t last_nb_lines, last_nb_pages;
extern bool     last_bold_state, verbose;
#endif  // BUILD_SCREENSHOTS

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void compute_relativePosition(nbgl_obj_t *obj, nbgl_obj_t *prevObj)
{
    nbgl_container_t *parent = (nbgl_container_t *) (obj->parent);
    // LOG_DEBUG(OBJ_LOGGER,"compute_relativePosition()\n");
    // compute object absolute position thanks to layout/alignment
    if (obj->alignment == NO_ALIGNMENT) {
        LOG_DEBUG(
            OBJ_LOGGER,
            "compute_relativePosition() without align to, parent->layout = %d, prevObj = %p\n",
            parent->layout,
            prevObj);
        // align to parent, depending of layout
        if (parent->layout == VERTICAL) {
            obj->rel_x0 = obj->alignmentMarginX;
            if (prevObj != NULL) {
                obj->rel_y0 = prevObj->rel_y0 + prevObj->area.height + obj->alignmentMarginY;
            }
            else {
                obj->rel_y0 = obj->alignmentMarginY;
            }
        }
        else {  // HORIZONTAL
            if (prevObj != NULL) {
                obj->rel_x0 = prevObj->rel_x0 + prevObj->area.width + obj->alignmentMarginX;
            }
            else {
                obj->rel_x0 = obj->alignmentMarginX;
            }
            obj->rel_y0 = obj->alignmentMarginY;
        }
    }
    else {
        nbgl_obj_t *alignToObj = obj->alignTo;
        if (alignToObj == NULL) {
            alignToObj = obj->parent;
            LOG_DEBUG(OBJ_LOGGER, "compute_relativePosition() with align to parent\n");
        }
        else {
            LOG_DEBUG(OBJ_LOGGER, "compute_relativePosition() with align to = %p\n", alignToObj);
        }
        // align to the given object, with the given constraints
        if (alignToObj == obj->parent) {
            // align inside if the reference is the parent
            switch (obj->alignment) {
                case TOP_LEFT:
                    obj->rel_x0 = obj->alignmentMarginX;
                    obj->rel_y0 = obj->alignmentMarginY;
                    break;
                case TOP_MIDDLE:
                    obj->rel_x0
                        = (parent->obj.area.width - obj->area.width) / 2 + obj->alignmentMarginX;
                    obj->rel_y0 = obj->alignmentMarginY;
                    break;
                case TOP_RIGHT:
                    obj->rel_x0
                        = (parent->obj.area.width - obj->area.width) - obj->alignmentMarginX;
                    obj->rel_y0 = obj->alignmentMarginY;
                    break;
                case MID_LEFT:
                    obj->rel_x0 = obj->alignmentMarginX;
                    obj->rel_y0
                        = (parent->obj.area.height - obj->area.height) / 2 + obj->alignmentMarginY;
                    break;
                case CENTER:
                    obj->rel_x0
                        = (parent->obj.area.width - obj->area.width) / 2 + obj->alignmentMarginX;
                    obj->rel_y0
                        = (parent->obj.area.height - obj->area.height) / 2 + obj->alignmentMarginY;
                    break;
                case MID_RIGHT:
                    obj->rel_x0
                        = (parent->obj.area.width - obj->area.width) - obj->alignmentMarginX;
                    obj->rel_y0
                        = (parent->obj.area.height - obj->area.height) / 2 + obj->alignmentMarginY;
                    break;
                case BOTTOM_LEFT:
                    obj->rel_x0 = obj->alignmentMarginX;
                    obj->rel_y0
                        = (parent->obj.area.height - obj->area.height) - obj->alignmentMarginY;
                    break;
                case BOTTOM_MIDDLE:
                    obj->rel_x0
                        = (parent->obj.area.width - obj->area.width) / 2 + obj->alignmentMarginX;
                    obj->rel_y0
                        = (parent->obj.area.height - obj->area.height) - obj->alignmentMarginY;
                    break;
                case BOTTOM_RIGHT:
                    obj->rel_x0
                        = (parent->obj.area.width - obj->area.width) - obj->alignmentMarginX;
                    obj->rel_y0
                        = (parent->obj.area.height - obj->area.height) - obj->alignmentMarginY;
                    break;
                default:
                    // not supported
                    break;
            }
        }
        else {
            // align outside if the reference is a "brother"
            switch (obj->alignment) {
                case TOP_LEFT:
                    obj->rel_x0 = alignToObj->rel_x0 + obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 - obj->area.height - obj->alignmentMarginY;
                    break;
                case TOP_MIDDLE:
                    obj->rel_x0 = alignToObj->rel_x0
                                  + (alignToObj->area.width - obj->area.width) / 2
                                  + obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 - obj->area.height - obj->alignmentMarginY;
                    break;
                case TOP_RIGHT:
                    obj->rel_x0 = alignToObj->rel_x0 + (alignToObj->area.width - obj->area.width)
                                  - obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 - obj->area.height - obj->alignmentMarginY;
                    break;

                case LEFT_TOP:
                    obj->rel_x0 = alignToObj->rel_x0 - obj->area.width - obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 + obj->alignmentMarginY;
                    break;
                case MID_LEFT:
                    obj->rel_x0 = alignToObj->rel_x0 - obj->area.width - obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0
                                  + (alignToObj->area.height - obj->area.height) / 2
                                  + obj->alignmentMarginY;
                    break;
                case LEFT_BOTTOM:
                    obj->rel_x0 = alignToObj->rel_x0 - obj->area.width - obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 + (alignToObj->area.height - obj->area.height)
                                  + obj->alignmentMarginY;
                    break;

                case RIGHT_TOP:
                    obj->rel_x0
                        = alignToObj->rel_x0 + alignToObj->area.width + obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 + obj->alignmentMarginY;
                    break;
                case MID_RIGHT:
                    obj->rel_x0
                        = alignToObj->rel_x0 + alignToObj->area.width + obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0
                                  + (alignToObj->area.height - obj->area.height) / 2
                                  + obj->alignmentMarginY;
                    break;
                case RIGHT_BOTTOM:
                    obj->rel_x0
                        = alignToObj->rel_x0 + alignToObj->area.width + obj->alignmentMarginX;
                    obj->rel_y0 = alignToObj->rel_y0 + (alignToObj->area.height - obj->area.height)
                                  + obj->alignmentMarginY;
                    break;

                case BOTTOM_LEFT:
                    obj->rel_x0 = alignToObj->rel_x0 + obj->alignmentMarginX;
                    obj->rel_y0
                        = alignToObj->rel_y0 + alignToObj->area.height + obj->alignmentMarginY;
                    break;
                case BOTTOM_MIDDLE:
                    obj->rel_x0 = alignToObj->rel_x0
                                  + (alignToObj->area.width - obj->area.width) / 2
                                  + obj->alignmentMarginX;
                    obj->rel_y0
                        = alignToObj->rel_y0 + alignToObj->area.height + obj->alignmentMarginY;
                    break;
                case BOTTOM_RIGHT:
                    obj->rel_x0 = alignToObj->rel_x0 + (alignToObj->area.width - obj->area.width)
                                  - obj->alignmentMarginX;
                    obj->rel_y0
                        = alignToObj->rel_y0 + alignToObj->area.height + obj->alignmentMarginY;
                    break;
                default:
                    // not supported
                    break;
            }
        }
    }
}

static void compute_position(nbgl_obj_t *obj, nbgl_obj_t *prevObj)
{
    nbgl_container_t *parent = (nbgl_container_t *) (obj->parent);

    compute_relativePosition(obj, prevObj);

    if (parent == NULL) {
        // HUGE issue
        return;
    }
    // LOG_DEBUG(OBJ_LOGGER,"compute_position(), parent.type = %d, parent->obj.area.x0 = %d,
    // obj->rel_x0=%d\n",parent->type,parent->obj.area.x0,obj->rel_x0);
    // LOG_DEBUG(OBJ_LOGGER,"compute_position(), parent->obj.area.y0 = %d,
    // obj->rel_y0=%d\n",parent->obj.area.y0,obj->rel_y0);

    obj->area.x0 = parent->obj.area.x0 + obj->rel_x0;
    obj->area.y0 = parent->obj.area.y0 + obj->rel_y0;

    if ((obj->area.x0 + obj->area.width) > SCREEN_WIDTH) {
        LOG_FATAL(OBJ_LOGGER,
                  "compute_position(), forbidden width, obj->type = %d, x0=%d, width=%d\n",
                  obj->type,
                  obj->area.x0,
                  obj->area.width);
    }
#ifdef HAVE_SE_TOUCH
    if ((obj->area.y0 + obj->area.height) > SCREEN_HEIGHT) {
        LOG_FATAL(OBJ_LOGGER,
                  "compute_position(), forbidden height, obj->type = %d, y0=%d, height=%d\n",
                  obj->type,
                  obj->area.y0,
                  obj->area.height);
    }
#endif  // HAVE_SE_TOUCH
}

static void draw_screen(nbgl_container_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t rectArea;

    UNUSED(prevObj);
    UNUSED(computePosition);

    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    rectArea.x0              = obj->obj.area.x0;
    rectArea.y0              = obj->obj.area.y0;
    rectArea.width           = obj->obj.area.width;
    rectArea.height          = obj->obj.area.height;
    nbgl_frontDrawRect(&rectArea);
}

static void draw_container(nbgl_container_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER,
              "draw_container(), x0 = %d, y0 = %d, width = %d, height = %d\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->obj.area.width,
              obj->obj.area.height);
    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
    if (obj->forceClean) {
        nbgl_frontDrawRect((nbgl_area_t *) obj);
    }
}

#ifdef HAVE_SE_TOUCH
/**
 * @brief internal function used to draw a button
 * @note The button contains an icon AND/OR a single-line UTF-8 text
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if true, force to compute the object position
 */
static void draw_button(nbgl_button_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    uint16_t    textWidth = 0;
    const char *text      = NULL;
#define ICON_TEXT_SPACE 12

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER,
              "draw_button(), x0 = %d, y0 = %d, width = %d, height = %d\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->obj.area.width,
              obj->obj.area.height);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
    // draw the rounded corner rectangle if necessary
    if ((obj->innerColor == obj->borderColor)
        && (obj->innerColor != obj->obj.area.backgroundColor)) {
        nbgl_drawRoundedRect((nbgl_area_t *) obj, obj->radius, obj->innerColor);
    }
    else {
        nbgl_drawRoundedBorderedRect(
            (nbgl_area_t *) obj, obj->radius, BUTTON_STROKE, obj->innerColor, obj->borderColor);
    }
    // get the text of the button from the callback if not NULL
    if (obj->onDrawCallback != NULL) {
        obj->text = obj->onDrawCallback(obj->token);
    }
    text = obj->text;
    // draw the text (right of the icon, with 8 pixels between them)
    if (text != NULL) {
        nbgl_area_t rectArea;
        // Compute available with & height to display the text
        rectArea.x0 = obj->obj.area.x0;
        rectArea.y0 = obj->obj.area.y0;
        // Only one line of text
        rectArea.height = nbgl_getFontHeight(obj->fontId);
        rectArea.y0 += (obj->obj.area.height - rectArea.height) / 2;
        rectArea.width = obj->obj.area.width;
        if (obj->icon != NULL) {
            rectArea.x0 += obj->icon->width + ICON_TEXT_SPACE;
            rectArea.width -= obj->icon->width + ICON_TEXT_SPACE;
        }
        // Compute the width & number of characters displayed on first line
        uint16_t textLen;
        nbgl_getTextMaxLenAndWidth(obj->fontId, text, rectArea.width, &textLen, &textWidth, true);

#ifdef BUILD_SCREENSHOTS
        store_string_infos(text, obj->fontId, &rectArea, true, 1, 1, false);
#endif  // BUILD_SCREENSHOTS

        // Center the text, horizontally
        if (textWidth < rectArea.width) {
            rectArea.x0 += (rectArea.width - textWidth) / 2;
        }
        LOG_DEBUG(OBJ_LOGGER, "draw_button(), text = %s\n", text);
        rectArea.backgroundColor = obj->innerColor;
        nbgl_drawText(&rectArea, text, textLen, obj->fontId, obj->foregroundColor);
    }
    // draw the icon, if any
    if (obj->icon != NULL) {
        uint16_t    iconX0, iconY0;
        nbgl_area_t rectArea;

        if (text != NULL) {
            iconX0 = obj->obj.area.x0
                     + (obj->obj.area.width - (textWidth + obj->icon->width + ICON_TEXT_SPACE)) / 2;
        }
        else {
            iconX0 = obj->obj.area.x0 + (obj->obj.area.width - obj->icon->width) / 2;
        }
        LOG_DEBUG(OBJ_LOGGER,
                  "draw_button(), obj->obj.area.height = %d, obj->iconHeight = %d\n",
                  obj->obj.area.height,
                  obj->icon->height);
        iconY0 = obj->obj.area.y0 + (obj->obj.area.height - obj->icon->height) / 2;

        rectArea.backgroundColor = obj->innerColor;
        rectArea.x0              = iconX0;
        rectArea.y0              = iconY0;
        rectArea.width           = obj->icon->width;
        rectArea.height          = obj->icon->height;
        rectArea.bpp             = obj->icon->bpp;

        nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, obj->foregroundColor, obj->icon);
    }
}

/**
 * @brief internal function used to draw a vertical or horizontal line, up to 4 pixels thick (@ref
 * LINE type)
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if true, force to compute the object position
 */
static void draw_line(nbgl_line_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t rectArea;
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER, "draw_line(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);
    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
    LOG_DEBUG(OBJ_LOGGER,
              "draw_line(), backgroundColor = %d, lineColor = %d\n",
              obj->obj.area.backgroundColor,
              obj->lineColor);
    rectArea.x0              = obj->obj.area.x0;
    rectArea.y0              = obj->obj.area.y0;
    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    if (obj->direction == VERTICAL) {
        rectArea.width = obj->obj.area.width = obj->thickness;
        rectArea.height                      = obj->obj.area.height;
        nbgl_frontDrawLine(&rectArea, 0, obj->lineColor);
    }
    else {
        rectArea.width  = obj->obj.area.width;
        rectArea.height = obj->obj.area.height = obj->thickness;
        nbgl_frontDrawLine(&rectArea, 0, obj->lineColor);
    }
}
#else  // HAVE_SE_TOUCH
/**
 * @brief internal function used to draw a button
 * @note The button contains an icon AND/OR a single line UTF-8 text
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if true, force to compute the object position
 */
static void draw_button(nbgl_button_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    uint16_t    textWidth = 0;
    const char *text      = NULL;
#define ICON_TEXT_SPACE 2

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER,
              "draw_button(), x0 = %d, y0 = %d, width = %d, height = %d\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->obj.area.width,
              obj->obj.area.height);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
    // draw the rounded corner rectangle if necessary
    nbgl_drawRoundedRect((nbgl_area_t *) obj, obj->radius, obj->innerColor);

    // get the text of the button from the callback if not NULL
    if (obj->onDrawCallback != NULL) {
        obj->text = obj->onDrawCallback(obj->token);
    }
    text = obj->text;
    // draw the text (right of the icon, with ICON_TEXT_SPACE pixels between them)
    if (text != NULL) {
        nbgl_area_t rectArea;
        // Compute available with & height to display the text
        rectArea.x0 = obj->obj.area.x0;
        rectArea.y0 = obj->obj.area.y0;
        // Only one line of text is allowed
        rectArea.height = nbgl_getFontHeight(obj->fontId);
        rectArea.y0 += (obj->obj.area.height - rectArea.height) / 2;
        rectArea.width = obj->obj.area.width;
        if (obj->icon != NULL) {
            rectArea.x0 += obj->icon->width + ICON_TEXT_SPACE;
            rectArea.width -= obj->icon->width + ICON_TEXT_SPACE;
        }
        // Compute the width & number of characters displayed on first line
        uint16_t textLen;
        nbgl_getTextMaxLenAndWidth(obj->fontId, text, rectArea.width, &textLen, &textWidth, true);

        // Center the text, horizontally
        if (textWidth < rectArea.width) {
            rectArea.x0 += (rectArea.width - textWidth) / 2;
        }
        LOG_DEBUG(OBJ_LOGGER, "draw_button(), text = %s\n", text);
        rectArea.backgroundColor = obj->innerColor;
        nbgl_drawText(&rectArea, text, textLen, obj->fontId, obj->foregroundColor);
    }
    // draw the icon, if any
    if (obj->icon != NULL) {
        uint16_t    iconX0, iconY0;
        nbgl_area_t rectArea;

        if (text != NULL) {
            iconX0 = obj->obj.area.x0
                     + (obj->obj.area.width - (textWidth + obj->icon->width + ICON_TEXT_SPACE)) / 2;
        }
        else {
            iconX0 = obj->obj.area.x0 + (obj->obj.area.width - obj->icon->width) / 2;
        }
        LOG_DEBUG(OBJ_LOGGER,
                  "draw_button(), obj->obj.area.height = %d, obj->iconHeight = %d\n",
                  obj->obj.area.height,
                  obj->icon->height);
        iconY0 = obj->obj.area.y0 + (obj->obj.area.height - obj->icon->height) / 2;

        rectArea.backgroundColor = obj->innerColor;
        rectArea.x0              = iconX0;
        rectArea.y0              = iconY0;
        rectArea.width           = obj->icon->width;
        rectArea.height          = obj->icon->height;
        rectArea.bpp             = obj->icon->bpp;

        nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, obj->foregroundColor, obj->icon);
    }
}
#endif  // HAVE_SE_TOUCH

static void draw_image(nbgl_image_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    const nbgl_icon_details_t *iconDetails;
    nbgl_color_map_t           colorMap;

    // if buffer is NULL, let's try to call onDrawCallback, if not NULL, to get it
    if (obj->buffer == NULL) {
        if (obj->onDrawCallback != NULL) {
            iconDetails = obj->onDrawCallback(obj->token);
        }
        else {
            return;
        }
    }
    else {
        iconDetails = obj->buffer;
    }
    if (iconDetails == NULL) {
        return;
    }

    // use dimension and bpp from the icon details
    obj->obj.area.width  = iconDetails->width;
    obj->obj.area.height = iconDetails->height;
    obj->obj.area.bpp    = iconDetails->bpp;
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER, "draw_image(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);
    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
    if (obj->obj.area.bpp == NBGL_BPP_1) {
        colorMap = obj->foregroundColor;
    }
    else if (obj->obj.area.bpp == NBGL_BPP_2) {
        colorMap = ((WHITE << 6) | (LIGHT_GRAY << 4) | (DARK_GRAY << 2) | BLACK);
    }
    else {
        colorMap = obj->foregroundColor;
    }

    nbgl_drawIcon((nbgl_area_t *) obj, obj->transformation, colorMap, iconDetails);
}

#ifdef HAVE_SE_TOUCH
static void draw_switch(nbgl_switch_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t rectArea;

    // force dimensions
    obj->obj.area.width  = SWITCH_ICON.width;
    obj->obj.area.height = SWITCH_ICON.height;
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER, "draw_switch(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    rectArea.x0              = obj->obj.area.x0;
    rectArea.y0              = obj->obj.area.y0;
    rectArea.width           = obj->obj.area.width;
    rectArea.height          = obj->obj.area.height;
    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    rectArea.bpp             = NBGL_BPP_1;
    if (obj->state == OFF_STATE) {
#if NB_COLOR_BITS == 1
        nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, obj->onColor, &C_switch_off_24px);
#else
        nbgl_frontDrawImage(&rectArea, SWITCH_ICON.bitmap, NO_TRANSFORMATION, obj->offColor);
#endif
    }
    else {
#if NB_COLOR_BITS == 1
        nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, obj->onColor, &C_switch_on_24px);
#else
        nbgl_frontDrawImage(&rectArea, SWITCH_ICON.bitmap, VERTICAL_MIRROR, obj->onColor);
#endif
    }
}

static void draw_radioButton(nbgl_radio_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t                rectArea;
    const nbgl_icon_details_t *icon;
    nbgl_color_map_t           color_map;

    // force dimensions
    obj->obj.area.width  = RADIO_WIDTH;
    obj->obj.area.height = RADIO_HEIGHT;
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER,
              "draw_radioButton(), x0 = %d, y0 = %d, state = %d\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->state);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    if (obj->state == OFF_STATE) {
        icon = &RADIO_OFF_ICON;
#if NB_COLOR_BITS == 1
        color_map = obj->activeColor;
#else
        color_map = obj->borderColor;
#endif
    }
    else {
        icon      = &RADIO_ON_ICON;
        color_map = obj->activeColor;
    }
    rectArea.x0              = obj->obj.area.x0;
    rectArea.y0              = obj->obj.area.y0;
    rectArea.width           = obj->obj.area.width;
    rectArea.height          = obj->obj.area.height;
    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    rectArea.bpp             = icon->bpp;
    nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, color_map, icon);
}
#endif  // HAVE_SE_TOUCH

/**
 * @brief internal function used to draw a progress bar object (@ref PROGRESS_BAR type)
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_progressBar(nbgl_progress_bar_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
#ifdef HAVE_SE_TOUCH

    uint8_t stroke = PROGRESS_STROKE;  // pixels for border

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER,
              "draw_progressBar(), x0 = %d, y0 = %d, level = %d %%\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->state);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    if (obj->partialRedraw == false) {
        // Case of progress bar full draw
        if (obj->withBorder) {
            nbgl_drawRoundedBorderedRect((nbgl_area_t *) obj,
                                         RADIUS_0_PIXELS,
                                         stroke,
                                         obj->obj.area.backgroundColor,
                                         obj->foregroundColor);
        }
        else {
            nbgl_drawRoundedRect(
                (nbgl_area_t *) obj, RADIUS_0_PIXELS, obj->obj.area.backgroundColor);
        }
        // also reset previous width to be sure to clean up everything
        obj->previousWidth = 0;
    }
    // Setup bar draw
    nbgl_area_t barArea;
    uint16_t    barWidth    = ((obj->state * obj->obj.area.width)) / 100;
    int         barDiffWith = barWidth - obj->previousWidth;
    color_t     barColor;
    memcpy(&barArea, &obj->obj.area, sizeof(nbgl_area_t));

    if (barDiffWith > 0) {
        // Drawing "forward"
        barArea.x0    = obj->obj.area.x0 + obj->previousWidth;
        barArea.width = barDiffWith;
        barColor      = obj->foregroundColor;
    }
    else if (barDiffWith < 0) {
        // Drawing "backward"
        barArea.x0    = obj->obj.area.x0 + obj->previousWidth + barDiffWith;
        barArea.width = -barDiffWith;
        barColor      = obj->obj.area.backgroundColor;
    }

    if (barDiffWith != 0) {
        nbgl_drawRoundedRect((nbgl_area_t *) &barArea, RADIUS_0_PIXELS, barColor);
    }

    // reset partialRedraw to be sure that in case of full redraw of the screen we redraw the
    // full bar
    if (obj->partialRedraw == true) {
        obj->partialRedraw = false;
    }
    obj->previousWidth = barWidth;

    extendRefreshArea(&barArea);
    objRefreshAreaDone = true;
#else   // HAVE_SE_TOUCH
    uint8_t  stroke = 1;  // 1 pixels for border
    uint16_t levelWidth;

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER,
              "draw_progressBar(), x0 = %d, y0 = %d, level = %d %%\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->state);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    // draw external part if necessary
    if (obj->withBorder) {
        nbgl_drawRoundedBorderedRect((nbgl_area_t *) obj,
                                     RADIUS_3_PIXELS,
                                     stroke,
                                     obj->obj.area.backgroundColor,
                                     obj->foregroundColor);
    }
    else {
        nbgl_drawRoundedRect((nbgl_area_t *) obj, RADIUS_3_PIXELS, obj->obj.area.backgroundColor);
    }
    // draw level
    levelWidth = MIN((obj->obj.area.width - 2) * obj->state / 100, (obj->obj.area.width - 2));
    if (levelWidth > 0) {
        nbgl_area_t rectArea;
        rectArea.width           = levelWidth;
        rectArea.height          = obj->obj.area.height - 2;
        rectArea.backgroundColor = obj->foregroundColor;
        rectArea.bpp             = NBGL_BPP_1;
        rectArea.x0              = obj->obj.area.x0 + 1;
        rectArea.y0              = obj->obj.area.y0 + 1;

        nbgl_frontDrawRect(&rectArea);
    }
#endif  // HAVE_SE_TOUCH
}

#ifdef HAVE_SE_TOUCH
/**
 * @brief internal function used to draw a navigation indicator object (@ref PAGE_INDICATOR type)
 * @note It is represented as a dashed line with as many dashes as pages
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_pageIndicator(nbgl_page_indicator_t *obj,
                               nbgl_obj_t            *prevObj,
                               bool                   computePosition)
{
    nbgl_area_t rectArea;
    uint16_t    dashWidth;

    // display nothing if less than two pages
    if (obj->nbPages < 2) {
        return;
    }

    if (obj->nbPages <= NB_MAX_PAGES_WITH_DASHES) {
        int i;
        // force height
        obj->obj.area.height = 4;

        if (computePosition) {
            compute_position((nbgl_obj_t *) obj, prevObj);
        }
        LOG_DEBUG(OBJ_LOGGER,
                  "draw_pageIndicator(), x0 = %d, y0 = %d, page = %d/%d\n",
                  obj->obj.area.x0,
                  obj->obj.area.y0,
                  obj->activePage,
                  obj->nbPages);

        // inherit background from parent
        obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

        dashWidth      = (obj->obj.area.width - ((obj->nbPages - 1) * INTER_DASHES)) / obj->nbPages;
        rectArea.x0    = obj->obj.area.x0;
        rectArea.y0    = obj->obj.area.y0;
        rectArea.width = dashWidth;
        rectArea.height          = 4;
        rectArea.backgroundColor = obj->obj.area.backgroundColor;
        rectArea.bpp             = NBGL_BPP_1;
        // draw dashes
        for (i = 0; i < obj->activePage; i++) {
            nbgl_frontDrawLine(
                &rectArea, 0, (obj->style == PROGRESSIVE_INDICATOR) ? BLACK : LIGHT_GRAY);
            rectArea.x0 += dashWidth + INTER_DASHES;
        }
        nbgl_frontDrawLine(&rectArea, 0, BLACK);
        rectArea.x0 += dashWidth + INTER_DASHES;
        i++;

        for (; i < obj->nbPages; i++) {
            nbgl_frontDrawLine(&rectArea, 0, LIGHT_GRAY);
            rectArea.x0 += dashWidth + INTER_DASHES;
        }
    }
    else {
        char navText[11];  // worst case is "ccc of nnn"

        SPRINTF(navText, "%d of %d", obj->activePage + 1, obj->nbPages);
        // force height
        obj->obj.area.height = nbgl_getFontHeight(SMALL_REGULAR_FONT);
        // the width must be at least 80
        obj->obj.area.width = nbgl_getTextWidth(SMALL_REGULAR_FONT, navText);

        if (computePosition) {
            compute_position((nbgl_obj_t *) obj, prevObj);
        }
        LOG_DEBUG(OBJ_LOGGER,
                  "draw_pageIndicator(), x0 = %d, y0 = %d, page = %d/%d\n",
                  obj->obj.area.x0,
                  obj->obj.area.y0,
                  obj->activePage,
                  obj->nbPages);

        // inherit background from parent
        obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

        // draw active page
        rectArea.x0              = obj->obj.area.x0;
        rectArea.y0              = obj->obj.area.y0;
        rectArea.width           = obj->obj.area.width;
        rectArea.height          = obj->obj.area.height;
        rectArea.backgroundColor = obj->obj.area.backgroundColor;
        rectArea.bpp             = NBGL_BPP_1;
        nbgl_drawText(&rectArea, navText, strlen(navText), SMALL_REGULAR_FONT, LIGHT_TEXT_COLOR);
    }
}
#endif  // HAVE_SE_TOUCH

/**
 * @brief internal function used to draw a text area
 * @note the text itself is placed in the given area depending of textAlignment member of obj
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_textArea(nbgl_text_area_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t    rectArea;
    uint16_t       textWidth, fontHeight, lineHeight, textHeight, midHeight;
    uint8_t        line, nbLines;
    const char    *text;
    nbgl_font_id_e fontId = obj->fontId;

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    // get the text of the button from the callback if not NULL
    if (obj->onDrawCallback != NULL) {
        obj->text = obj->onDrawCallback(obj->token);
    }
    text = obj->text;
    if (text == NULL) {
        return;
    }

    LOG_DEBUG(
        OBJ_LOGGER,
        "draw_textArea(), wrapping = %d, x0 = %d, y0 = %d, width = %d, height = %d, text = %s\n",
        obj->wrapping,
        obj->obj.area.x0,
        obj->obj.area.y0,
        obj->obj.area.width,
        obj->obj.area.height,
        text);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    // draw background to make sure it's clean
#ifdef SCREEN_SIZE_NANO
    if (obj->style == INVERTED_COLORS) {
        obj->obj.area.backgroundColor = WHITE;
        rectArea.backgroundColor      = BLACK;
    }
    else
#endif  // SCREEN_SIZE_NANO
    {
        // inherit background from parent
        obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
        rectArea.backgroundColor      = obj->obj.area.backgroundColor;
    }
    rectArea.x0     = obj->obj.area.x0;
    rectArea.y0     = obj->obj.area.y0;
    rectArea.width  = obj->obj.area.width;
    rectArea.height = obj->obj.area.height;
#ifdef SCREEN_SIZE_NANO
    if (obj->style == INVERTED_COLORS) {
        nbgl_drawRoundedRect(&rectArea, RADIUS_1_PIXEL, WHITE);
    }
    else
#endif  // SCREEN_SIZE_NANO
    {
        nbgl_frontDrawRect(&rectArea);
    }

    fontHeight = nbgl_getFontHeight(fontId);
    lineHeight = nbgl_getFontLineHeight(fontId);
    // special case of autoHideLongLine, when the text is too long for a line, draw '...' at the
    // beginning
    if (obj->autoHideLongLine == true) {
#ifdef BUILD_SCREENSHOTS
        nbgl_getTextNbLinesInWidth(fontId, text, obj->obj.area.width, obj->wrapping);
        store_string_infos(text,
                           fontId,
                           &obj->obj.area,
                           obj->wrapping,
                           last_nb_lines,
                           last_nb_pages,
                           last_bold_state);
#endif  // BUILD_SCREENSHOTS
        textWidth = nbgl_getSingleLineTextWidth(fontId, text);
        if (textWidth > obj->obj.area.width) {
            uint16_t lineWidth, lineLen;
            uint16_t dotsWidth;

            dotsWidth      = nbgl_getTextWidth(fontId, "...");
            rectArea.x0    = obj->obj.area.x0;
            rectArea.y0    = obj->obj.area.y0 + (obj->obj.area.height - fontHeight) / 2;
            rectArea.width = dotsWidth;
            nbgl_drawText(&rectArea, "...", 3, fontId, obj->textColor);
            // then draw the end of text
            nbgl_getTextMaxLenAndWidthFromEnd(
                fontId, text, obj->obj.area.width - dotsWidth, &lineLen, &lineWidth);
            rectArea.x0 += dotsWidth;
            rectArea.width = lineWidth;
            nbgl_drawText(&rectArea,
                          &text[nbgl_getTextLength(text) - lineLen],
                          lineLen,
                          obj->fontId,
                          obj->textColor);
            return;
        }
    }

    // get nb lines in the given width (depending of wrapping)
    nbLines = nbgl_getTextNbLinesInWidth(fontId, text, obj->obj.area.width, obj->wrapping);
#ifdef BUILD_SCREENSHOTS
    store_string_infos(
        text, fontId, &obj->obj.area, obj->wrapping, last_nb_lines, last_nb_pages, last_bold_state);
#endif  // BUILD_SCREENSHOTS
    // saturate nb lines if nbMaxLines is greater than 0
    if ((obj->nbMaxLines > 0) && (obj->nbMaxLines < nbLines)) {
        nbLines = obj->nbMaxLines;
    }

    textHeight = (nbLines - 1) * lineHeight + fontHeight;

    midHeight = (obj->obj.area.height - textHeight) / 2;
#ifdef SCREEN_SIZE_NANO
    if (obj->style == INVERTED_COLORS) {
        midHeight--;
    }
#endif  // SCREEN_SIZE_NANO

    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    rectArea.height          = fontHeight;
    // draw each line
    for (line = 0; line < nbLines; line++) {
        uint16_t lineWidth, lineLen;

        nbgl_getTextMaxLenAndWidth(
            fontId, text, obj->obj.area.width, &lineLen, &lineWidth, obj->wrapping);
        if (obj->textAlignment == MID_LEFT) {
            rectArea.x0 = obj->obj.area.x0;
        }
        else if (obj->textAlignment == CENTER) {
            rectArea.x0 = obj->obj.area.x0 + (obj->obj.area.width - lineWidth) / 2;
        }
        else if (obj->textAlignment == MID_RIGHT) {
            rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - lineWidth;
        }
        else {
            LOG_FATAL(OBJ_LOGGER, "Forbidden obj->textAlignment = %d\n", obj->textAlignment);
        }
        rectArea.y0    = obj->obj.area.y0 + midHeight + line * lineHeight;
        rectArea.width = lineWidth;

        LOG_DEBUG(OBJ_LOGGER,
                  "draw_textArea(), [%s] line %d, lineLen %d lineWidth = %d, obj.area.height = %d, "
                  "textHeight = %d, nbMaxLines = %d, wrapping = %d\n",
                  text,
                  line,
                  lineLen,
                  lineWidth,
                  obj->obj.area.height,
                  textHeight,
                  obj->nbMaxLines,
                  obj->wrapping);
        if ((obj->nbMaxLines == 0) || (line < (obj->nbMaxLines - 1))) {
            fontId = nbgl_drawText(&rectArea, text, lineLen, fontId, obj->textColor);
        }
        else {
#ifdef HAVE_SE_TOUCH
            uint16_t dotsWidth = nbgl_getSingleLineTextWidth(obj->fontId, "...");
            // for last chunk, if nbMaxLines is used, replace the 3 last chars by "..."
            // draw line except 3 last chars
            if ((lineWidth + dotsWidth) >= obj->obj.area.width) {
                lineLen -= 3;
            }
            nbgl_drawText(&rectArea, text, lineLen, obj->fontId, obj->textColor);
            rectArea.x0 += nbgl_getSingleLineTextWidthInLen(obj->fontId, text, lineLen);

            // draw "..." after the other chars
            rectArea.width = dotsWidth;
            nbgl_drawText(&rectArea, "...", 3, obj->fontId, obj->textColor);
#else   // HAVE_SE_TOUCH
            nbgl_drawText(&rectArea, text, lineLen, fontId, obj->textColor);
#endif  // HAVE_SE_TOUCH
            return;
        }
        text += lineLen;
        /* skip trailing \n */
        if (*text == '\n') {
            text++;
        }
    }
}

#ifdef NBGL_QRCODE
/**
 * @brief internal function used to draw a QR Code
 * @note the QRCode is centered in the given object area
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_qrCode(nbgl_qrcode_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t rectArea;

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    // be sure to align vertical position on multiple of 4
    obj->obj.area.y0 &= ~0x3;
    LOG_DEBUG(OBJ_LOGGER,
              "draw_qrCode(), x0 = %d, y0 = %d, width = %d, height = %d, text = %s\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->obj.area.width,
              obj->obj.area.height,
              obj->text);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    rectArea.x0              = obj->obj.area.x0;
    rectArea.y0              = obj->obj.area.y0;
    rectArea.width           = obj->obj.area.width;
    rectArea.height          = obj->obj.area.height;
    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    nbgl_drawQrCode(&rectArea, obj->version, obj->text, obj->foregroundColor);
}
#endif  // NBGL_QRCODE

#ifdef NBGL_KEYBOARD
/**
 * @brief internal function used to draw a Keyboard object
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_keyboard(nbgl_keyboard_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
#ifndef HAVE_SE_TOUCH
    obj->obj.area.width  = KEYBOARD_WIDTH;
    obj->obj.area.height = KEYBOARD_KEY_HEIGHT;
#endif  // HAVE_SE_TOUCH

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(
        OBJ_LOGGER, "draw_keyboard(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    nbgl_objDrawKeyboard(obj);
}
#endif  // NBGL_KEYBOARD

#ifdef NBGL_KEYPAD
/**
 * @brief internal function used to draw a Keypad object
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_keypad(nbgl_keypad_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
#ifndef HAVE_SE_TOUCH
    obj->obj.area.height = KEYPAD_HEIGHT;
    obj->obj.area.width  = KEYPAD_WIDTH;
#endif  // HAVE_SE_TOUCH

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    obj->obj.area.y0 &= ~0x3;
    LOG_DEBUG(OBJ_LOGGER, "draw_keypad(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;

    nbgl_objDrawKeypad(obj);
}
#endif  // NBGL_KEYPAD

#ifdef HAVE_SE_TOUCH
/**
 * @brief internal function used to draw a Spinner object
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_spinner(nbgl_spinner_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t rectArea;
    color_t     foreColor;

    obj->obj.area.width  = SPINNER_WIDTH;
    obj->obj.area.height = SPINNER_HEIGHT;

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }
    LOG_DEBUG(OBJ_LOGGER, "draw_spinner(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);

    // inherit background from parent
    obj->obj.area.backgroundColor = obj->obj.parent->area.backgroundColor;
    // foreground color is the opposite of background one
    foreColor = (obj->obj.area.backgroundColor == WHITE) ? BLACK : WHITE;

    rectArea.bpp             = NBGL_BPP_1;
    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    // if position is OxFF, it means "fixed" so draw 4 corners
    if (obj->position == 0xFF) {
        // draw horizontal segments
        rectArea.x0     = obj->obj.area.x0;
        rectArea.y0     = obj->obj.area.y0;
        rectArea.width  = SPINNER_DASH_WIDTH;
        rectArea.height = SPINNER_DASH_STROKE;
        nbgl_frontDrawLine(&rectArea, 0, foreColor);  // top left
        rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - rectArea.width;
        nbgl_frontDrawLine(&rectArea, 0, foreColor);  // top right
        rectArea.y0 = obj->obj.area.y0 + obj->obj.area.height - 3;
        nbgl_frontDrawLine(&rectArea, 0, foreColor);  // bottom right
        rectArea.x0 = obj->obj.area.x0;
        nbgl_frontDrawLine(&rectArea, 0, foreColor);  // bottom left
        // draw vertical segments
        rectArea.x0              = obj->obj.area.x0;
        rectArea.y0              = obj->obj.area.y0;
        rectArea.width           = SPINNER_DASH_STROKE;
        rectArea.height          = SPINNER_DASH_HEIGHT;
        rectArea.backgroundColor = foreColor;
        nbgl_frontDrawRect(&rectArea);  // top left
        rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - rectArea.width;
        nbgl_frontDrawRect(&rectArea);  // top right
        rectArea.y0 = obj->obj.area.y0 + obj->obj.area.height - rectArea.height;
        nbgl_frontDrawRect(&rectArea);  // bottom right
        rectArea.x0 = obj->obj.area.x0;
        nbgl_frontDrawRect(&rectArea);  // bottom left
    }
    else {
        // clean up full rectangle
        rectArea.x0              = obj->obj.area.x0;
        rectArea.y0              = obj->obj.area.y0;
        rectArea.width           = obj->obj.area.width;
        rectArea.height          = obj->obj.area.height;
        rectArea.backgroundColor = obj->obj.area.backgroundColor;
        nbgl_frontDrawRect(&rectArea);  // top left

        // draw horizontal segment in foreColor
        rectArea.width  = SPINNER_DASH_WIDTH;
        rectArea.height = SPINNER_DASH_STROKE;
        switch (obj->position) {
            case 0:  // top left corner
                rectArea.x0 = obj->obj.area.x0;
                rectArea.y0 = obj->obj.area.y0;
                break;
            case 1:  // top right
                rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - rectArea.width;
                rectArea.y0 = obj->obj.area.y0;
                break;
            case 2:  // bottom right
                rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - rectArea.width;
                rectArea.y0 = obj->obj.area.y0 + obj->obj.area.height - 3;
                break;
            case 3:  // bottom left
                rectArea.x0 = obj->obj.area.x0;
                rectArea.y0 = obj->obj.area.y0 + obj->obj.area.height - 3;
                break;
            default:
                return;
        }
        nbgl_frontDrawLine(&rectArea, 0, foreColor);

        // draw vertical segment in foreColor
        rectArea.width           = SPINNER_DASH_STROKE;
        rectArea.height          = SPINNER_DASH_HEIGHT;
        rectArea.backgroundColor = foreColor;
        switch (obj->position) {
            case 0:  // top left corner
                rectArea.x0 = obj->obj.area.x0;
                rectArea.y0 = obj->obj.area.y0;
                break;
            case 1:  // top right corner
                rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - rectArea.width;
                rectArea.y0 = obj->obj.area.y0;
                break;
            case 2:  // bottom right corner
                rectArea.x0 = obj->obj.area.x0 + obj->obj.area.width - rectArea.width;
                rectArea.y0 = obj->obj.area.y0 + obj->obj.area.height - rectArea.height;
                break;
            case 3:  // bottom left corner
                rectArea.x0 = obj->obj.area.x0;
                rectArea.y0 = obj->obj.area.y0 + obj->obj.area.height - rectArea.height;
                break;
            default:
                return;
        }
        nbgl_frontDrawRect(&rectArea);
    }
}

#else   // HAVE_SE_TOUCH

/**
 * @brief internal function used to draw a text entry
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_textEntry(nbgl_text_entry_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    nbgl_area_t rectArea;
    int         textLen = strlen(obj->text);
    uint32_t    offsetX;

    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }

    LOG_DEBUG(OBJ_LOGGER,
              "draw_textEntry(), x0 = %d, y0 = %d, width = %d, height = %d\n",
              obj->obj.area.x0,
              obj->obj.area.y0,
              obj->obj.area.width,
              obj->obj.area.height);

    // draw background to make sure it's clean
    obj->obj.area.backgroundColor = WHITE;
    rectArea.backgroundColor      = BLACK;
    rectArea.x0                   = obj->obj.area.x0;
    rectArea.y0                   = obj->obj.area.y0;
    rectArea.width                = obj->obj.area.width;
    rectArea.height               = obj->obj.area.height;
    rectArea.bpp                  = NBGL_BPP_1;
    nbgl_drawRoundedRect(&rectArea, RADIUS_3_PIXELS, WHITE);

    rectArea.backgroundColor = obj->obj.area.backgroundColor;
    rectArea.height          = nbgl_getFontHeight(obj->fontId);
    if (obj->nbChars > NB_MAX_LETTERS) {
        return;
    }
    offsetX = (obj->obj.area.width - (obj->nbChars * 10)) / 2;
    // draw each of the nb chars
    for (int i = 0; i < obj->nbChars; i++) {
        char digit;
        rectArea.x0    = obj->obj.area.x0 + offsetX + (i * 10);
        rectArea.y0    = obj->obj.area.y0 - 2;
        rectArea.width = 8;
        if (textLen < obj->nbChars) {
            if (i < textLen) {
                digit = obj->text[i];
            }
            else {
                digit = '_';
            }
        }
        else {
            // first char is '..' to notify continuing
            if (i == 0) {
                nbgl_drawText(&rectArea, "..", 2, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp, BLACK);
                continue;
            }
            else if (i < (obj->nbChars - 1)) {
                digit = obj->text[textLen - obj->nbChars + 1 + i];
            }
            else {
                digit = '_';
            }
        }
        nbgl_drawText(&rectArea, &digit, 1, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp, BLACK);
    }
}
#endif  // HAVE_SE_TOUCH

/**
 * @brief internal function used to draw an image file
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_image_file(nbgl_image_file_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    if (obj->buffer == NULL) {
        return;
    }
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }

    LOG_DEBUG(
        OBJ_LOGGER, "draw_image_file(), x0 = %d, y0 = %d\n", obj->obj.area.x0, obj->obj.area.y0);
    obj->obj.area.backgroundColor = WHITE;
    nbgl_frontDrawImageFile((nbgl_area_t *) obj, obj->buffer, BLACK, ramBuffer);
}

#ifdef NBGL_MASKING
static void draw_mask_control(nbgl_mask_control_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    if (computePosition) {
        compute_position((nbgl_obj_t *) obj, prevObj);
    }

    if (obj->enableMasking) {
        nbgl_frontControlAreaMasking(obj->maskIndex, &obj->obj.area);
    }
    else {
        nbgl_frontControlAreaMasking(obj->maskIndex, NULL);
    }
}
#endif  // NBGL_MASKING

/**
 * @brief internal function used to draw an object of any type
 *
 * @param obj the object to draw
 * @param prevObj the previous object drawned in the same container, with the default layout
 * @param computePosition if TRUE, force to compute the object position
 */
static void draw_object(nbgl_obj_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    LOG_DEBUG(OBJ_LOGGER, "draw_object() obj->type = %d, prevObj = %p\n", obj->type, prevObj);
    objRefreshAreaDone = false;
    if ((obj->type < NB_OBJ_TYPES) && (draw_functions[obj->type] != NULL)) {
        if ((obj->type != SCREEN) && (obj->parent == NULL)) {
            LOG_WARN(OBJ_LOGGER, "object with type [%d] has not parent\n", obj->type);
            return;
        }
        draw_function_t func = (draw_function_t) PIC(draw_functions[obj->type]);
        func(obj, prevObj, computePosition);
    }
    else {
        LOG_WARN(OBJ_LOGGER, "Not existing object type [%d]\n", obj->type);
        return;
    }

#ifdef HAVE_SERIALIZED_NBGL
    io_seph_ux_send_nbgl_serialized(NBGL_DRAW_OBJ, obj);
#endif
    if (!objRefreshAreaDone) {
        extendRefreshArea(&obj->area);
    }
}

/**
 * @brief This function redraws the given object and its children (recursive version)
 *
 * @param obj the object to redraw
 * @param prevObj the previous child of this object in the parent's children array (may be NULL for
 * first element of the array)
 * @param computePosition if set to true, means that the position of the object will be recomputed
 * (needs prevObj if alignment on this object)
 */
static void draw_obj_and_chidren(nbgl_obj_t *obj, nbgl_obj_t *prevObj, bool computePosition)
{
    uint8_t i = 0;
    LOG_DEBUG(OBJ_LOGGER, "draw_obj_and_chidren(): obj = %p\n", obj);
    // draw the object itself
    draw_object(obj, prevObj, computePosition);

    if ((obj->type == SCREEN) || (obj->type == CONTAINER)) {
        nbgl_container_t *container = (nbgl_container_t *) obj;
        nbgl_obj_t       *prev      = NULL;
        LOG_DEBUG(
            OBJ_LOGGER, "draw_obj_and_chidren(): container->children = %p\n", container->children);
        // draw the children, if any
        if (container->children != NULL) {
            for (i = 0; i < container->nbChildren; i++) {
                nbgl_obj_t *current = container->children[i];
                if (current != NULL) {
                    current->parent = (nbgl_obj_t *) container;
                    draw_obj_and_chidren(current, prev, true);
                    if (current->alignTo == NULL) {
                        prev = current;
                    }
                }
            }
        }
    }
}

/**
 * @brief internal function used when an object is drawn, to extend the area to be refresh in
 * display
 *
 * @param area area to refresh
 */
static void extendRefreshArea(nbgl_area_t *area)
{
    int16_t x1, y1;  // bottom right corner
    x1 = refreshArea.x0 + refreshArea.width;
    y1 = refreshArea.y0 + refreshArea.height;

    // if obj top-left is on left of current top-left corner, move top-left corner
    if (area->x0 < refreshArea.x0) {
        // No negative coordinates
        refreshArea.x0 = MAX(0, area->x0);
    }
    // if obj bottom-right is on right of current bottom-right corner, move bottom-right corner
    if (((area->x0 + area->width) > x1) || (refreshArea.width == 0)) {
        // Not beyond width
        x1 = MIN(SCREEN_WIDTH, area->x0 + area->width);
    }
    // if obj top-left is on top of current top-left corner, move top-left corner
    if (area->y0 < refreshArea.y0) {
        // No negative coordinates and align on lower multiple of alignment
        refreshArea.y0 = MAX(0, area->y0) & ~(VERTICAL_ALIGNMENT - 1);
    }
    // if obj bottom-right is on bottom of current bottom-right corner, move bottom-right corner
    if (((area->y0 + area->height) > y1) || (refreshArea.height == 0)) {
        // Not beyond height
        y1 = MIN(SCREEN_HEIGHT, area->y0 + area->height);
        // align on upper multiple of alignment
        y1 = (y1 + (VERTICAL_ALIGNMENT - 1)) & ~(VERTICAL_ALIGNMENT - 1);
    }

    // sanity check
    if (x1 > SCREEN_WIDTH) {
        LOG_FATAL(OBJ_LOGGER,
                  "extendRefreshArea: Impossible area x0 = %d width %d\n",
                  refreshArea.x0,
                  refreshArea.width);
    }
    if (y1 > SCREEN_HEIGHT) {
#ifdef HAVE_SE_TOUCH
        LOG_FATAL(OBJ_LOGGER,
                  "extendRefreshArea: Impossible area y0 = %d height %d\n",
                  refreshArea.y0,
                  refreshArea.height);
#else   // HAVE_SE_TOUCH
        y1 = SCREEN_HEIGHT;
#endif  // HAVE_SE_TOUCH
    }
    // recompute width and height
    refreshArea.width  = x1 - refreshArea.x0;
    refreshArea.height = y1 - refreshArea.y0;

    // revaluate area bpp
    if (area->bpp > refreshArea.bpp) {
        refreshArea.bpp = area->bpp;
    }
}

/**
 * @brief This function draws or redraws the given object and its children (recursive version)
 *
 * @param obj the object to redraw
 */
void nbgl_objDraw(nbgl_obj_t *obj)
{
    bool computePosition = false;
    bool fromApp         = false;

    LOG_DEBUG(OBJ_LOGGER, "nbgl_objDraw(): obj = %p\n", obj);
    // check whether it's necessary to compute position, and if this object belongs to
    // an Application screen
    if (obj->type == SCREEN) {
        // always compute position for screen and all sub-objects
        computePosition = true;
        fromApp         = !(((nbgl_screen_t *) obj)->isUxScreen);
    }
    else {
        nbgl_obj_t *parent = obj;
        // search screen in parenthood
        while (parent->parent != NULL) {
            parent = parent->parent;
        }
        if (parent->type == SCREEN) {
            fromApp = !(((nbgl_screen_t *) parent)->isUxScreen);
        }
        else {
            // should never happen
            fromApp = false;
        }
    }
    // forbid redrawing App screens by UX or vice versa
    if ((os_sched_current_task() == TASK_BOLOS_UX) == fromApp) {
        return;
    }
    // actually draw the object and its children, if it is allowed
    if (objDrawingDisabled && fromApp) {
        return;
    }
    draw_obj_and_chidren(obj, NULL, computePosition);
}

/**
 * @brief This functions refreshes the actual screen on display with what has changed since the last
 * refresh
 *
 */
void nbgl_refresh(void)
{
    nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
}

/**
 * @brief This functions refreshes the actual screen on display with what has changed since the last
 * refresh, according to the given mode (Black&White and other)
 * @param mode mode of refresh
 */
void nbgl_refreshSpecial(nbgl_refresh_mode_t mode)
{
    if ((refreshArea.width == 0) || (refreshArea.height == 0)) {
        return;
    }

    nbgl_frontRefreshArea(&refreshArea, mode, POST_REFRESH_FORCE_POWER_OFF);
    LOG_DEBUG(OBJ_LOGGER,
              "nbgl_refreshSpecial(), x0,y0 = [%d, %d], w,h = [%d, %d]\n",
              refreshArea.x0,
              refreshArea.y0,
              refreshArea.width,
              refreshArea.height);
    nbgl_refreshReset();
}

void nbgl_refreshSpecialWithPostRefresh(nbgl_refresh_mode_t mode, nbgl_post_refresh_t post_refresh)
{
    if ((refreshArea.width == 0) || (refreshArea.height == 0)) {
        return;
    }

    nbgl_frontRefreshArea(&refreshArea, mode, post_refresh);
    LOG_DEBUG(OBJ_LOGGER,
              "nbgl_refreshSpecialNoPoff(), x0,y0 = [%d, %d], w,h = [%d, %d]\n",
              refreshArea.x0,
              refreshArea.y0,
              refreshArea.width,
              refreshArea.height);
    nbgl_refreshReset();
}

/**
 * @brief This functions returns true if there is something to refresh
 * @return true if there is something to refresh
 */
bool nbgl_refreshIsNeeded(void)
{
    if ((refreshArea.width == 0) || (refreshArea.height == 0)) {
        return false;
    }
    return true;
}

/**
 * @brief This functions resets all changes since the last refresh
 *
 */
void nbgl_refreshReset(void)
{
#ifdef HAVE_SERIALIZED_NBGL
    io_seph_ux_send_nbgl_serialized(NBGL_REFRESH_AREA, (nbgl_obj_t *) &refreshArea);
#endif
    refreshArea.x0     = SCREEN_WIDTH - 1;
    refreshArea.width  = 0;
    refreshArea.y0     = SCREEN_HEIGHT - 1;
    refreshArea.height = 0;
    refreshArea.bpp    = NBGL_BPP_2;
}

/**
 * @brief This functions inits all internal of nbgl objects layer
 * @note it is supposed to be called only from App
 *
 */
void nbgl_objInit(void)
{
    // init area to the smallest size
    nbgl_refreshReset();
    objDrawingDisabled = false;
#ifdef HAVE_SE_TOUCH
    nbgl_touchInit(false);
#endif
}

/**
 * @brief This functions enables or disables drawing/refresh for all further calls
 *
 * @param enable if true, enables drawing/refresh, otherwise disables
 */
void nbgl_objAllowDrawing(bool enable)
{
    objDrawingDisabled = !enable;
}

/**
 * @brief This function is used to get the all purpose RAM buffer
 *
 * @return a pointer to the all purpose RAM buffer
 */
uint8_t *nbgl_objGetRAMBuffer(void)
{
    return ramBuffer;
}

/**
 * @brief This function returns true if the object belongs to a UxScreen
 *
 * @return true if the object belongs to a UxScreen
 */
bool nbgl_objIsUx(nbgl_obj_t *obj)
{
    nbgl_obj_t *parent = obj;
    // search screen in parenthood
    while (parent->parent != NULL) {
        parent = parent->parent;
    }
    if (parent->type == SCREEN) {
        return (((nbgl_screen_t *) parent)->isUxScreen);
    }
    else {
        // should never happen
        return true;
    }
}
