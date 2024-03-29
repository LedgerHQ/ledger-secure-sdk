/**
 * @file nbgl_layout_kbd.c
 * @brief Implementation of keypad management of predefined layouts management for Applications
 * @note This file applies only to wallet size products (Stax, Flex...)
 */

#ifdef HAVE_SE_TOUCH
#ifdef NBGL_KEYPAD
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdlib.h>
#include "nbgl_debug.h"
#include "nbgl_front.h"
#include "nbgl_layout_internal.h"
#include "nbgl_obj.h"
#include "nbgl_draw.h"
#include "nbgl_screen.h"
#include "nbgl_touch.h"
#include "glyphs.h"
#include "os_pic.h"
#include "os_helpers.h"
#include "lcx_rng.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL API FUNCTIONS
 **********************/

/**
 * @brief Adds a keypad on bottom of the screen, with the associated callback
 *
 * @note Validate and Backspace keys are not enabled at start-up
 *
 * @param layout the current layout
 * @param callback function called when any of the key is touched
 * @param shuffled if set to true, digits are shuffled in keypad
 * @return the index of keypad, to use in @ref nbgl_layoutUpdateKeypad()
 */
int nbgl_layoutAddKeypad(nbgl_layout_t *layout, keyboardCallback_t callback, bool shuffled)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_keypad_t         *keypad;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddKeypad():\n");
    if (layout == NULL) {
        return -1;
    }

    // create keypad
    keypad                       = (nbgl_keypad_t *) nbgl_objPoolGet(KEYPAD, layoutInt->layer);
    keypad->obj.alignmentMarginY = 0;
    keypad->obj.alignment        = BOTTOM_MIDDLE;
    keypad->obj.alignTo          = NULL;
    keypad->borderColor          = LIGHT_GRAY;
    keypad->callback             = PIC(callback);
    keypad->enableDigits         = true;
    keypad->enableBackspace      = false;
    keypad->enableValidate       = false;
    keypad->shuffled             = shuffled;
    // set this new keypad as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) keypad);

    // return index of keypad to be modified later on
    return (layoutInt->container->nbChildren - 1);
}

/**
 * @brief Updates an existing keypad on bottom of the screen, with the given configuration
 *
 * @param layout the current layout
 * @param index index returned by @ref nbgl_layoutAddKeypad()
 * @param enableValidate if true, enable Validate key
 * @param enableBackspace if true, enable Backspace key
 * @param enableDigits if true, enable all digit keys
 * @return >=0 if OK
 */
int nbgl_layoutUpdateKeypad(nbgl_layout_t *layout,
                            uint8_t        index,
                            bool           enableValidate,
                            bool           enableBackspace,
                            bool           enableDigits)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_keypad_t         *keypad;

    LOG_DEBUG(LAYOUT_LOGGER,
              "nbgl_layoutUpdateKeypad(): enableValidate = %d, enableBackspace = %d\n",
              enableValidate,
              enableBackspace);
    if (layout == NULL) {
        return -1;
    }

    // get existing keypad
    keypad = (nbgl_keypad_t *) layoutInt->container->children[index];
    if ((keypad == NULL) || (keypad->obj.type != KEYPAD)) {
        return -1;
    }
    // partial redraw only if only validate and backspace have changed
    keypad->partial         = (keypad->enableDigits == enableDigits);
    keypad->enableValidate  = enableValidate;
    keypad->enableBackspace = enableBackspace;
    keypad->enableDigits    = enableDigits;

    nbgl_redrawObject((nbgl_obj_t *) keypad, NULL, false);

    return 0;
}

/**
 * @brief Adds a placeholder for hidden digits on top of a keypad, to represent the entered digits,
 * as full circles The placeholder is "underligned" with a thin horizontal line of the expected full
 * length
 *
 * @note It must be the last added object, after potential back key, title, and keypad. Vertical
 * positions of title and hidden digits will be computed here
 *
 * @param layout the current layout
 * @param nbDigits number of digits to be displayed
 * @return the index of digits set, to use in @ref nbgl_layoutUpdateHiddenDigits()
 */
int nbgl_layoutAddHiddenDigits(nbgl_layout_t *layout, uint8_t nbDigits)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    uint8_t                space;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddHiddenDigits():\n");
    if (layout == NULL) {
        return -1;
    }
    if (nbDigits > KEYPAD_MAX_DIGITS) {
        return -1;
    }
    if (nbDigits > 8) {
        space = 4;
    }
    else {
        space = 12;
    }

    // create a container, invisible or bordered
    container             = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    container->nbChildren = nbDigits;
#ifdef TARGET_STAX
    container->nbChildren++;  // +1 for the line
#endif                        // TARGET_STAX
    container->children = nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
    // <space> pixels between each icon (knowing that the effective round are 18px large and the
    // icon 24px)
    container->obj.area.width = nbDigits * C_round_24px.width + (nbDigits + 1) * space;
#ifdef TARGET_STAX
    container->obj.area.height = 48;
    // distance from digits to title is fixed to 20 px, except if title is more than 1 line and a
    // back key is present
    if ((layoutInt->container->nbChildren != 3)
        || (layoutInt->container->children[1]->area.height == 32)) {
        container->obj.alignmentMarginY = 20;
    }
    else {
        container->obj.alignmentMarginY = 12;
    }
#else   // TARGET_STAX
    container->obj.area.height = 64;
#endif  // TARGET_STAX

    // item N-2 is the title
    container->obj.alignTo   = layoutInt->container->children[layoutInt->container->nbChildren - 2];
    container->obj.alignment = BOTTOM_MIDDLE;

    // set this new container as child of the main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    // create children of the container, as images (empty circles)
    nbgl_objPoolGetArray(IMAGE, nbDigits, layoutInt->layer, (nbgl_obj_t **) container->children);
    for (int i = 0; i < nbDigits; i++) {
        nbgl_image_t *image = (nbgl_image_t *) container->children[i];
#ifdef TARGET_STAX
        image->buffer = &C_round_24px;
#else   // TARGET_STAX
        image->buffer = &C_pin_24;
#endif  // TARGET_STAX
        image->foregroundColor      = WHITE;
        image->obj.alignmentMarginX = space;
        if (i > 0) {
            image->obj.alignment = MID_RIGHT;
            image->obj.alignTo   = (nbgl_obj_t *) container->children[i - 1];
        }
        else {
            image->obj.alignment        = NO_ALIGNMENT;
            image->obj.alignmentMarginY = (container->obj.area.height - C_round_24px.width) / 2;
        }
    }
#ifdef TARGET_STAX
    nbgl_line_t *line;
    // create gray line
    line                          = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
    line->lineColor               = LIGHT_GRAY;
    line->obj.alignmentMarginY    = 0;
    line->obj.alignTo             = NULL;
    line->obj.alignment           = BOTTOM_MIDDLE;
    line->obj.area.width          = container->obj.area.width;
    line->obj.area.height         = 4;
    line->direction               = HORIZONTAL;
    line->thickness               = 2;
    line->offset                  = 2;
    container->children[nbDigits] = (nbgl_obj_t *) line;
#endif  // TARGET_STAX

    // return index of keypad to be modified later on
    return (layoutInt->container->nbChildren - 1);
}

/**
 * @brief Updates an existing set of hidden digits, with the given configuration
 *
 * @param layout the current layout
 * @param index index returned by @ref nbgl_layoutAddHiddenDigits()
 * @param nbActive number of "active" digits (represented by discs instead of circles)
 * @return >=0 if OK
 */
int nbgl_layoutUpdateHiddenDigits(nbgl_layout_t *layout, uint8_t index, uint8_t nbActive)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_image_t          *image;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateHiddenDigits(): nbActive = %d\n", nbActive);
    if (layout == NULL) {
        return -1;
    }

    // get container
    container = (nbgl_container_t *) layoutInt->container->children[index];
    // sanity check
    if ((container == NULL) || (container->obj.type != CONTAINER)) {
        return -1;
    }
    if (nbActive > container->nbChildren) {
        return -1;
    }
    if (nbActive == 0) {
        // deactivate the first digit
        image = (nbgl_image_t *) container->children[0];
        if ((image == NULL) || (image->obj.type != IMAGE)) {
            return -1;
        }
        image->foregroundColor = WHITE;
    }
    else {
        image = (nbgl_image_t *) container->children[nbActive - 1];
        if ((image == NULL) || (image->obj.type != IMAGE)) {
            return -1;
        }
        // if the last "active" is already active, it means that we are decreasing the number of
        // active otherwise we are increasing it
        if (image->foregroundColor == BLACK) {
            // all digits are already active
            if (nbActive == container->nbChildren) {
                return 0;
            }
            // deactivate the next digit
            image                  = (nbgl_image_t *) container->children[nbActive];
            image->foregroundColor = WHITE;
        }
        else {
#ifdef TARGET_STAX
            image->buffer = &C_round_24px;
#else   // TARGET_STAX
            image->buffer = &C_pin_24;
#endif  // TARGET_STAX
            image->foregroundColor = BLACK;
        }
    }

    nbgl_redrawObject((nbgl_obj_t *) image, NULL, false);

    return 0;
}
/**
 * @brief Adds a "digits entry" area under the previously entered object. A horizontal gray line
 * is placed under the text. This text is vertically placed in the screen with offsetY
 *
 * @param layout the current layout
 * @param text string to display in the area
 * @param offsetY vertical offset from the top of the page
 * @return >= 0 if OK
 */
int nbgl_layoutAddEnteredDigits(nbgl_layout_t *layout, const char *text, int offsetY)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *textArea;
    nbgl_line_t           *line;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddEnteredDigits():\n");
    if (layout == NULL) {
        return -1;
    }

    // create gray line
    line                       = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
    line->lineColor            = LIGHT_GRAY;
    line->obj.alignmentMarginY = offsetY;
    line->obj.alignTo     = layoutInt->container->children[layoutInt->container->nbChildren - 1];
    line->obj.alignment   = TOP_MIDDLE;
    line->obj.area.width  = SCREEN_WIDTH - 2 * 32;
    line->obj.area.height = 4;
    line->direction       = HORIZONTAL;
    line->thickness       = 2;
    line->offset          = 2;
    // set this new line as child of the main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) line);

    // create text area
    textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor     = BLACK;
    textArea->text          = text;
    textArea->textAlignment = MID_LEFT;
    textArea->fontId        = LARGE_MEDIUM_1BPP_FONT;
#ifdef TARGET_STAX
    textArea->obj.alignmentMarginY = 12;
#else   // TARGET_STAX
    textArea->obj.alignmentMarginY = 16;
#endif  // TARGET_STAX
    textArea->obj.alignTo      = (nbgl_obj_t *) line;
    textArea->obj.alignment    = TOP_LEFT;
    textArea->obj.area.width   = line->obj.area.width;
    textArea->obj.area.height  = nbgl_getFontLineHeight(textArea->fontId);
    textArea->autoHideLongLine = true;

    // set this new text area as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    // return index of text area to be modified later on
    return (layoutInt->container->nbChildren - 1);
}

/**
 * @brief Updates an existing "digits entry" area, created with @ref nbgl_layoutAddEnteredDigits()
 *
 * @param layout the current layout
 * @param index index of the text (return value of @ref nbgl_layoutAddEnteredDigits())
 * @param text string to display in the area
 * @return <0 if error, 0 if OK with text fitting the area, 1 of 0K with text
 * not fitting the area
 */
int nbgl_layoutUpdateEnteredDigits(nbgl_layout_t *layout, uint8_t index, const char *text)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateEnteredDigits():\n");
    if (layout == NULL) {
        return -1;
    }

    // update main text area
    textArea = (nbgl_text_area_t *) layoutInt->container->children[index];
    if ((textArea == NULL) || (textArea->obj.type != TEXT_AREA)) {
        return -1;
    }
    textArea->text          = text;
    textArea->textColor     = BLACK;
    textArea->textAlignment = MID_LEFT;
    nbgl_redrawObject((nbgl_obj_t *) textArea, NULL, false);

    // if the text doesn't fit, indicate it by returning 1 instead of 0, for different refresh
    if (nbgl_getSingleLineTextWidth(textArea->fontId, text) > textArea->obj.area.width) {
        return 1;
    }
    return 0;
}
#endif  // NBGL_KEYPAD
#endif  // HAVE_SE_TOUCH
