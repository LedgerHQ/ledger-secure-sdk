/**
 * @file nbgl_layout_keypad.c
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

/*********************
 *      DEFINES
 *********************/
#ifdef TARGET_STAX
#define DIGIT_ICON C_round_24px
#else  // TARGET_STAX
#define DIGIT_ICON C_pin_24
#endif  // TARGET_STAX

enum {
    TITLE_INDEX = 0,
    INPUT_INDEX,
    LINE_INDEX,
    NB_CHILDREN
};

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
    // footer must be empty
    if (layoutInt->footerContainer != NULL) {
        return -1;
    }

    // create keypad
    keypad                       = (nbgl_keypad_t *) nbgl_objPoolGet(KEYPAD, layoutInt->layer);
    keypad->obj.alignmentMarginY = 0;
    keypad->obj.alignment        = BOTTOM_MIDDLE;
    keypad->obj.alignTo          = NULL;
    keypad->obj.area.width       = SCREEN_WIDTH;
    keypad->obj.area.height      = 4 * KEYPAD_KEY_HEIGHT;
    keypad->borderColor          = LIGHT_GRAY;
    keypad->callback             = PIC(callback);
    keypad->enableDigits         = true;
    keypad->enableBackspace      = false;
    keypad->enableValidate       = false;
    keypad->shuffled             = shuffled;

    // the keypad occupies the footer
    layoutInt->footerContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    layoutInt->footerContainer->obj.area.width = SCREEN_WIDTH;
    layoutInt->footerContainer->layout         = VERTICAL;
    layoutInt->footerContainer->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(1, layoutInt->layer);
    layoutInt->footerContainer->obj.alignment   = BOTTOM_MIDDLE;
    layoutInt->footerContainer->obj.area.height = keypad->obj.area.height;
    layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
        = (nbgl_obj_t *) keypad;
    layoutInt->footerContainer->nbChildren++;

    // add to layout children
    layoutInt->children[FOOTER_INDEX] = (nbgl_obj_t *) layoutInt->footerContainer;

    // subtract footer height from main container height
    layoutInt->container->obj.area.height -= layoutInt->footerContainer->obj.area.height;

    layoutInt->footerType = KEYPAD_FOOTER_TYPE;

    return layoutInt->footerContainer->obj.area.height;
}

/**
 * @brief Updates an existing keypad on bottom of the screen, with the given configuration
 *
 * @param layout the current layout
 * @param index index returned by @ref nbgl_layoutAddKeypad() (unused, for compatibility)
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
    UNUSED(index);

    // get existing keypad (in the footer container)
    keypad = (nbgl_keypad_t *) layoutInt->footerContainer->children[0];
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
 * @deprecated Use @ref nbgl_layoutAddKeypadContent instead
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
    container->obj.area.width = nbDigits * DIGIT_ICON.width + (nbDigits - 1) * space;
#ifdef TARGET_STAX
    container->obj.area.height = 50;
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
        nbgl_image_t *image    = (nbgl_image_t *) container->children[i];
        image->buffer          = &DIGIT_ICON;
        image->foregroundColor = WHITE;
        if (i > 0) {
            image->obj.alignment        = MID_RIGHT;
            image->obj.alignTo          = (nbgl_obj_t *) container->children[i - 1];
            image->obj.alignmentMarginX = space;
        }
        else {
            image->obj.alignment = MID_LEFT;
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
 * @deprecated Use @ref nbgl_layoutUpdateKeypadContent instead
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
            image->buffer          = &DIGIT_ICON;
            image->foregroundColor = BLACK;
        }
    }

    nbgl_redrawObject((nbgl_obj_t *) image, NULL, false);

    return 0;
}

/**
 * @brief Adds an area with a title and a placeholder for hidden digits on top of a keypad, to
 * represent the entered digits as small discs. On Stax, the placeholder is "underligned" with a
 * thin horizontal line of the expected full length
 *
 * @note It shall be the only object added in the layout, beside a potential header and the keypad
 * itself
 *
 * @param layout the current layout
 * @param title the text to use on top of the digits
 * @param hidden if set to true, digits appear as discs, otherwise as visible digits (given in text
 * param)
 * @param nbDigits number of digits to be displayed (only used if hidden is true)
 * @param text only used if hidden is false
 * @return the height of this area, if no error, < 0 otherwise
 */
int nbgl_layoutAddKeypadContent(nbgl_layout_t *layout,
                                const char    *title,
                                bool           hidden,
                                uint8_t        nbDigits,
                                const char    *text)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddKeypadContent():\n");
    if (layout == NULL) {
        return -1;
    }
    // create a container, to store both title and "digits" (and line on Stax)
    container                 = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    container->nbChildren     = NB_CHILDREN;
    container->children       = nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
    container->obj.area.width = AVAILABLE_WIDTH;
    container->obj.alignment  = TOP_MIDDLE;
    container->obj.alignmentMarginY = 8;

    // create text area for title
    textArea                  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor       = BLACK;
    textArea->text            = title;
    textArea->textAlignment   = CENTER;
    textArea->fontId          = SMALL_REGULAR_FONT;
    textArea->wrapping        = true;
    textArea->obj.alignment   = TOP_MIDDLE;
    textArea->obj.area.width  = AVAILABLE_WIDTH;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    container->children[TITLE_INDEX] = (nbgl_obj_t *) textArea;
    container->obj.area.height += textArea->obj.area.height;

    if (hidden) {
        nbgl_container_t *digitsContainer;
        uint8_t           space;

        if (nbDigits > KEYPAD_MAX_DIGITS) {
            return -1;
        }
        // space between "digits"
        if (nbDigits > 8) {
            space = 4;
        }
        else {
#ifdef TARGET_STAX
            space = 10;
#else   // TARGET_STAX
            space = 12;
#endif  // TARGET_STAX
        }

        // create digits container, to store "discs"
        digitsContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
        digitsContainer->nbChildren = nbDigits;
        digitsContainer->children
            = nbgl_containerPoolGet(digitsContainer->nbChildren, layoutInt->layer);
        // <space> pixels between each icon (knowing that the effective round are 18px large and the
        // icon 24px)
        digitsContainer->obj.area.width = nbDigits * DIGIT_ICON.width + (nbDigits - 1) * space;
#ifdef TARGET_STAX
        digitsContainer->obj.area.height = 44;
#else   // TARGET_STAX
        digitsContainer->obj.area.height = 64;
#endif  // TARGET_STAX
        // align at the bottom of title
        digitsContainer->obj.alignTo   = container->children[0];
        digitsContainer->obj.alignment = BOTTOM_MIDDLE;
#ifdef TARGET_STAX
        digitsContainer->obj.alignmentMarginY = 28;
#endif  // TARGET_STAX
        container->children[INPUT_INDEX] = (nbgl_obj_t *) digitsContainer;
        container->obj.area.height += digitsContainer->obj.area.height;

        // create children of the container, as images (empty circles)
        nbgl_objPoolGetArray(
            IMAGE, nbDigits, layoutInt->layer, (nbgl_obj_t **) digitsContainer->children);
        for (int i = 0; i < nbDigits; i++) {
            nbgl_image_t *image    = (nbgl_image_t *) digitsContainer->children[i];
            image->buffer          = &DIGIT_ICON;
            image->foregroundColor = WHITE;
            if (i > 0) {
                image->obj.alignment        = MID_RIGHT;
                image->obj.alignTo          = (nbgl_obj_t *) digitsContainer->children[i - 1];
                image->obj.alignmentMarginX = space;
            }
            else {
                image->obj.alignment = MID_LEFT;
            }
        }
    }
    else {
        // create text area
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = BLACK;
        textArea->text          = text;
        textArea->textAlignment = MID_LEFT;
        textArea->fontId        = LARGE_MEDIUM_1BPP_FONT;
        textArea->obj.area.width   = container->obj.area.width;
        textArea->obj.area.height  = nbgl_getFontLineHeight(textArea->fontId);
        textArea->autoHideLongLine = true;
        // align at the bottom of title
        textArea->obj.alignTo   = container->children[TITLE_INDEX];
        textArea->obj.alignment = BOTTOM_MIDDLE;
#ifdef TARGET_STAX
        textArea->obj.alignmentMarginY = 24;
#endif  // TARGET_STAX
        container->children[INPUT_INDEX] = (nbgl_obj_t *) textArea;
        container->obj.area.height += textArea->obj.area.height;
    }

    // set this new container as child of the main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);
#ifdef TARGET_STAX
    nbgl_line_t *line;
    // create gray line
    line                            = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
    line->lineColor                 = LIGHT_GRAY;
    line->obj.alignTo               = container->children[INPUT_INDEX];
    line->obj.alignment             = BOTTOM_MIDDLE;
    line->obj.area.width            = 288;
    line->obj.area.height           = 4;
    line->direction                 = HORIZONTAL;
    line->thickness                 = 2;
    line->offset                    = 2;
    container->children[LINE_INDEX] = (nbgl_obj_t *) line;
#endif  // TARGET_STAX

    // return height of the area
    return container->obj.area.height;
}

/**
 * @brief Updates an existing set of hidden digits, with the given configuration
 *
 * @param layout the current layout
 * @param hidden if set to true, digits appear as discs, otherwise as visible digits (given in text
 * param)
 * @param nbActiveDigits number of "active" digits (represented by discs instead of circles) (only
 * used if hidden is true)
 * @param text only used if hidden is false
 *
 * @return >=0 if OK
 */
int nbgl_layoutUpdateKeypadContent(nbgl_layout_t *layout,
                                   bool           hidden,
                                   uint8_t        nbActiveDigits,
                                   const char    *text)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_image_t          *image;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateHiddenDigits(): nbActive = %d\n", nbActiveDigits);
    if (layout == NULL) {
        return -1;
    }

    UNUSED(index);

    if (hidden) {
        // get digits container (second child of the main container)
        container = (nbgl_container_t *) ((nbgl_container_t *) layoutInt->container->children[0])
                        ->children[1];
        // sanity check
        if ((container == NULL) || (container->obj.type != CONTAINER)) {
            return -1;
        }
        if (nbActiveDigits > container->nbChildren) {
            return -1;
        }
        if (nbActiveDigits == 0) {
            // deactivate the first digit
            image = (nbgl_image_t *) container->children[0];
            if ((image == NULL) || (image->obj.type != IMAGE)) {
                return -1;
            }
            image->foregroundColor = WHITE;
        }
        else {
            image = (nbgl_image_t *) container->children[nbActiveDigits - 1];
            if ((image == NULL) || (image->obj.type != IMAGE)) {
                return -1;
            }
            // if the last "active" is already active, it means that we are decreasing the number of
            // active otherwise we are increasing it
            if (image->foregroundColor == BLACK) {
                // all digits are already active
                if (nbActiveDigits == container->nbChildren) {
                    return 0;
                }
                // deactivate the next digit by turning it to white
                image                  = (nbgl_image_t *) container->children[nbActiveDigits];
                image->foregroundColor = WHITE;
            }
            else {
                // activate it the last digit by turning it to black
                image->foregroundColor = BLACK;
            }
        }

        nbgl_redrawObject((nbgl_obj_t *) image, NULL, false);
    }
    else {
        // update main text area (second child of the main container)
        nbgl_text_area_t *textArea
            = (nbgl_text_area_t *) ((nbgl_container_t *) layoutInt->container->children[0])
                  ->children[1];
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
    }

    return 0;
}
#endif  // NBGL_KEYPAD
#endif  // HAVE_SE_TOUCH
