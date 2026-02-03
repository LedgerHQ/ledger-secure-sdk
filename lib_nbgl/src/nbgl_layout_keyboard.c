/**
 * @file nbgl_layout_keyboard.c
 * @brief Implementation of predefined keyboard related layouts management
 * @note This file applies only to wallet size products (Stax, Flex...)
 */

#ifdef HAVE_SE_TOUCH
#ifdef NBGL_KEYBOARD
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "nbgl_debug.h"
#include "nbgl_front.h"
#include "nbgl_layout_internal.h"
#include "nbgl_obj.h"
#include "nbgl_draw.h"
#include "nbgl_screen.h"
#include "nbgl_touch.h"
#include "glyphs.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"
#include "os_pic.h"
#include "os_helpers.h"

/*********************
 *      DEFINES
 *********************/
#if defined(TARGET_FLEX) || defined(TARGET_APEX)
#define USE_PARTIAL_BUTTONS 1
#endif  // TARGET_FLEX

// for suggestion buttons, on Flex there are other objects than buttons
enum {
    PAGE_INDICATOR_INDEX = 0,
#ifdef USE_PARTIAL_BUTTONS
    LEFT_HALF_INDEX,   // half disc displayed on the bottom left
    RIGHT_HALF_INDEX,  // half disc displayed on the bottom right
#endif                 // USE_PARTIAL_BUTTONS
    FIRST_BUTTON_INDEX,
    SECOND_BUTTON_INDEX,
#ifndef USE_PARTIAL_BUTTONS
    THIRD_BUTTON_INDEX,
    FOURTH_BUTTON_INDEX,
#endif  // !USE_PARTIAL_BUTTONS
    NB_SUGGESTION_CHILDREN
};

// Keyboard Text Entry container children
enum {
    NUMBER_INDEX = 0,
    TEXT_INDEX,
    DELETE_INDEX,
    LINE_INDEX
};

#if defined(TARGET_STAX)
#define TEXT_ENTRY_NORMAL_HEIGHT  64
#define TEXT_ENTRY_COMPACT_HEIGHT 64
#define BOTTOM_NORMAL_MARGIN      24
#define BOTTOM_CONFIRM_MARGIN     24
#define BOTTOM_COMPACT_MARGIN     24
#define TOP_NORMAL_MARGIN         20
#define TOP_CONFIRM_MARGIN        20
#define TOP_COMPACT_MARGIN        20
#define TITLE_ENTRY_MARGIN_Y      4
#define TEXT_ENTRY_FONT           LARGE_MEDIUM_1BPP_FONT
// space between number and text
#define NUMBER_TEXT_SPACE         8
#define NUMBER_WIDTH              56
#define DELETE_ICON               C_Close_32px
#elif defined(TARGET_FLEX)
#define TEXT_ENTRY_NORMAL_HEIGHT  72
#define TEXT_ENTRY_COMPACT_HEIGHT 56
#define BOTTOM_NORMAL_MARGIN      24
#define BOTTOM_CONFIRM_MARGIN     24
#define BOTTOM_COMPACT_MARGIN     12
#define TOP_NORMAL_MARGIN         20
#define TOP_CONFIRM_MARGIN        20
#define TOP_COMPACT_MARGIN        12
#define TITLE_ENTRY_MARGIN_Y      4
#define TEXT_ENTRY_FONT           LARGE_MEDIUM_1BPP_FONT
// space between number and text
#define NUMBER_TEXT_SPACE         8
#define NUMBER_WIDTH              56
#define DELETE_ICON               C_Close_40px
#elif defined(TARGET_APEX)
#define TEXT_ENTRY_NORMAL_HEIGHT  44
#define TEXT_ENTRY_COMPACT_HEIGHT 44
#define BOTTOM_NORMAL_MARGIN      20
#define BOTTOM_CONFIRM_MARGIN     16
#define BOTTOM_COMPACT_MARGIN     8
#define TOP_NORMAL_MARGIN         20
#define TOP_CONFIRM_MARGIN        12
#define TOP_COMPACT_MARGIN        8
#define TITLE_ENTRY_MARGIN_Y      4
#define TEXT_ENTRY_FONT           LARGE_MEDIUM_1BPP_FONT
// space between number and text
#define NUMBER_TEXT_SPACE         4
#define NUMBER_WIDTH              40
#define DELETE_ICON               C_Close_Tiny_24px
#endif  // TARGETS

#ifdef USE_PARTIAL_BUTTONS
#if defined(TARGET_FLEX)
#define LEFT_HALF_ICON              C_left_half_64px
#define SUGGESTION_CONTAINER_HEIGHT 92
#elif defined(TARGET_APEX)
#define LEFT_HALF_ICON              C_half_disc_left_40px_1bpp
#define SUGGESTION_CONTAINER_HEIGHT 56
#endif  // TARGETS
#endif  // USE_PARTIAL_BUTTONS

// space on left and right of suggestion buttons
#define SUGGESTION_BUTTONS_SIDE_MARGIN BORDER_MARGIN
#if defined(TARGET_APEX)
#define LINE_THICKNESS 1
#define LINE_COLOR     BLACK
#else
#define LINE_THICKNESS 2
#define LINE_COLOR     LIGHT_GRAY
#endif  // TARGETS

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

static const char *choiceTexts[NB_MAX_SUGGESTION_BUTTONS];
static char        numText[5];
static uint8_t     nbActiveButtons;
#ifdef USE_PARTIAL_BUTTONS
static nbgl_image_t *partialButtonImages[2];
#endif  // USE_PARTIAL_BUTTONS

/**********************
 *  STATIC PROTOTYPES
 **********************/

// function used on Flex to display (or not) beginning of next button and/or end of
// previous button, and update buttons when swipping
static bool updateSuggestionButtons(nbgl_container_t *container,
                                    nbgl_touchType_t  eventType,
                                    uint8_t           currentLeftButtonIndex)
{
    bool     needRefresh = false;
    uint8_t  page        = 0;
    uint32_t i;

    if ((eventType == SWIPED_LEFT)
        && (currentLeftButtonIndex
            < (uint32_t) (nbActiveButtons - NB_MAX_VISIBLE_SUGGESTION_BUTTONS))) {
        // shift all buttons on the left if there are still at least
        // NB_MAX_VISIBLE_SUGGESTION_BUTTONS buttons to display
        currentLeftButtonIndex += NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
        ((nbgl_button_t *) container->children[FIRST_BUTTON_INDEX])->text
            = choiceTexts[currentLeftButtonIndex];

        for (i = 1; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
            if (currentLeftButtonIndex < (uint32_t) (nbActiveButtons - i)) {
                ((nbgl_button_t *) container->children[FIRST_BUTTON_INDEX + i])->text
                    = choiceTexts[currentLeftButtonIndex + i];
            }
            else {
                ((nbgl_button_t *) container->children[FIRST_BUTTON_INDEX + i])->text = NULL;
            }
        }
        page        = currentLeftButtonIndex / NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
        needRefresh = true;
    }
    else if ((eventType == SWIPED_RIGHT)
             && (currentLeftButtonIndex > (NB_MAX_VISIBLE_SUGGESTION_BUTTONS - 1))) {
        // shift all buttons on the left if we are not already displaying the
        // NB_MAX_VISIBLE_SUGGESTION_BUTTONS first ones
        currentLeftButtonIndex -= NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
        for (i = 0; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
            ((nbgl_button_t *) container->children[FIRST_BUTTON_INDEX + i])->text
                = choiceTexts[currentLeftButtonIndex + i];
        }
        page        = currentLeftButtonIndex / NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
        needRefresh = true;
    }
    // align top-left button on the left
    if (container->children[FIRST_BUTTON_INDEX] != NULL) {
        container->children[FIRST_BUTTON_INDEX]->alignmentMarginX = SUGGESTION_BUTTONS_SIDE_MARGIN;
        container->children[FIRST_BUTTON_INDEX]->alignmentMarginY = 0;
        container->children[FIRST_BUTTON_INDEX]->alignment        = TOP_LEFT;
        container->children[FIRST_BUTTON_INDEX]->alignTo          = (nbgl_obj_t *) container;
    }

    // align top-right button on top-left one
    if (container->children[SECOND_BUTTON_INDEX] != NULL) {
        container->children[SECOND_BUTTON_INDEX]->alignmentMarginX = INTERNAL_MARGIN;
        container->children[SECOND_BUTTON_INDEX]->alignmentMarginY = 0;
        container->children[SECOND_BUTTON_INDEX]->alignment        = MID_RIGHT;
        container->children[SECOND_BUTTON_INDEX]->alignTo = container->children[FIRST_BUTTON_INDEX];
    }
#ifndef USE_PARTIAL_BUTTONS
    // align bottom-left button on top_left one
    if (container->children[THIRD_BUTTON_INDEX] != NULL) {
        container->children[THIRD_BUTTON_INDEX]->alignmentMarginX = 0;
        container->children[THIRD_BUTTON_INDEX]->alignmentMarginY = INTERNAL_MARGIN;
        container->children[THIRD_BUTTON_INDEX]->alignment        = BOTTOM_MIDDLE;
        container->children[THIRD_BUTTON_INDEX]->alignTo = container->children[FIRST_BUTTON_INDEX];
    }

    // align bottom-right button on bottom-left one
    if (container->children[FOURTH_BUTTON_INDEX] != NULL) {
        container->children[FOURTH_BUTTON_INDEX]->alignmentMarginX = INTERNAL_MARGIN;
        container->children[FOURTH_BUTTON_INDEX]->alignmentMarginY = 0;
        container->children[FOURTH_BUTTON_INDEX]->alignment        = MID_RIGHT;
        container->children[FOURTH_BUTTON_INDEX]->alignTo = container->children[THIRD_BUTTON_INDEX];
    }
#endif  // !USE_PARTIAL_BUTTONS

    // the first child is used by the progress indicator, displayed if more that
    // NB_MAX_VISIBLE_SUGGESTION_BUTTONS buttons
    nbgl_page_indicator_t *indicator
        = (nbgl_page_indicator_t *) container->children[PAGE_INDICATOR_INDEX];
    indicator->activePage = page;

#ifdef USE_PARTIAL_BUTTONS
    // if not on the first button, display end of previous button
    if (currentLeftButtonIndex > 0) {
        container->children[LEFT_HALF_INDEX] = (nbgl_obj_t *) partialButtonImages[0];
    }
    else {
        container->children[LEFT_HALF_INDEX] = NULL;
    }
    // if not on the last button, display beginning of next button
    if (currentLeftButtonIndex < (nbActiveButtons - NB_MAX_VISIBLE_SUGGESTION_BUTTONS)) {
        container->children[RIGHT_HALF_INDEX] = (nbgl_obj_t *) partialButtonImages[1];
    }
    else {
        container->children[RIGHT_HALF_INDEX] = NULL;
    }
#endif  // USE_PARTIAL_BUTTONS
    return needRefresh;
}

/**********************
 *   GLOBAL INTERNAL FUNCTIONS
 **********************/

bool keyboardSwipeCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType)
{
    nbgl_container_t *container = (nbgl_container_t *) obj;
    nbgl_container_t *suggestionsContainer;

    if ((container->nbChildren < 2) || (container->children[1]->type != CONTAINER)) {
        return false;
    }
    suggestionsContainer = (nbgl_container_t *) container->children[1];
    // try if suggestions buttons (more than NB_MAX_VISIBLE_SUGGESTION_BUTTONS)
    if (((eventType == SWIPED_LEFT) || (eventType == SWIPED_RIGHT))
        && (suggestionsContainer->nbChildren == NB_SUGGESTION_CHILDREN)
        && (nbActiveButtons > NB_MAX_VISIBLE_SUGGESTION_BUTTONS)) {
        uint32_t i = 0;
        while (i < (uint32_t) nbActiveButtons) {
            if (((nbgl_button_t *) suggestionsContainer->children[FIRST_BUTTON_INDEX])->text
                == choiceTexts[i]) {
                break;
            }
            i++;
        }

        if (i < (uint32_t) nbActiveButtons) {
            if (updateSuggestionButtons(suggestionsContainer, eventType, i)) {
                os_io_seph_cmd_piezo_play_tune(TUNE_TAP_CASUAL);
                nbgl_objDraw((nbgl_obj_t *) suggestionsContainer);
                nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
            }

            return true;
        }
    }
    return false;
}

static nbgl_container_t *addTextEntry(nbgl_layoutInternal_t *layoutInt,
                                      const char            *title,
                                      const char            *text,
                                      bool                   numbered,
                                      uint8_t                number,
                                      int                    textToken,
                                      bool                   compactMode)
{
    nbgl_container_t *mainContainer, *container;
    nbgl_text_area_t *textArea;
    layoutObj_t      *obj;
    uint16_t textEntryHeight = (compactMode ? TEXT_ENTRY_COMPACT_HEIGHT : TEXT_ENTRY_NORMAL_HEIGHT);
    bool     withCross       = ((text != NULL) && (strlen(text) > 0));

    // create a main container, to store title, and text-entry container
    mainContainer             = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    mainContainer->nbChildren = 2;
    mainContainer->children   = nbgl_containerPoolGet(mainContainer->nbChildren, layoutInt->layer);
    mainContainer->obj.area.width = AVAILABLE_WIDTH;
    mainContainer->obj.alignment  = CENTER;

    if (title != NULL) {
        // create text area for title
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = BLACK;
        textArea->text          = title;
        textArea->textAlignment = CENTER;
        textArea->fontId        = SMALL_REGULAR_FONT;
        textArea->wrapping      = true;
        textArea->obj.alignment = TOP_MIDDLE;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
        mainContainer->children[0]     = (nbgl_obj_t *) textArea;
        mainContainer->obj.area.height = textArea->obj.area.height + TITLE_ENTRY_MARGIN_Y;
    }

    // create a text-entry container number, entered text, potential cross and underline
    container                 = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    container->nbChildren     = 4;
    container->children       = nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
    container->obj.area.width = AVAILABLE_WIDTH;
    container->obj.alignment  = BOTTOM_MIDDLE;

    if (numbered) {
        // create Word num typed text
        textArea            = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor = BLACK;
        snprintf(numText, sizeof(numText), "%d.", number);
        textArea->text            = numText;
        textArea->textAlignment   = CENTER;
        textArea->fontId          = TEXT_ENTRY_FONT;
        textArea->obj.area.width  = NUMBER_WIDTH;
        textArea->obj.alignment   = MID_LEFT;
        textArea->obj.area.height = nbgl_getFontHeight(textArea->fontId);
        // set this text area as child of the container
        container->children[NUMBER_INDEX] = (nbgl_obj_t *) textArea;
    }

    // create text area for entered text
    textArea                 = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor      = BLACK;
    textArea->text           = text;
    textArea->textAlignment  = MID_LEFT;
    textArea->fontId         = TEXT_ENTRY_FONT;
    textArea->obj.area.width = AVAILABLE_WIDTH - DELETE_ICON.width - NUMBER_TEXT_SPACE;
    if (numbered) {
        textArea->obj.alignmentMarginX = NUMBER_TEXT_SPACE;
        textArea->obj.alignTo          = container->children[0];
        textArea->obj.alignment        = MID_RIGHT;
        textArea->obj.area.width -= textArea->obj.alignmentMarginX + NUMBER_WIDTH;
    }
    else {
        textArea->obj.alignment = MID_LEFT;
    }
    textArea->obj.area.height  = nbgl_getFontHeight(textArea->fontId);
    textArea->autoHideLongLine = true;

    container->children[TEXT_INDEX] = (nbgl_obj_t *) textArea;
    container->obj.area.height      = textEntryHeight;

    // Create Cross
    nbgl_image_t *image = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
    image->buffer       = &DELETE_ICON;
    // only display it if text non empty
    image->foregroundColor = withCross ? BLACK : WHITE;
    obj = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) image, textToken, NBGL_NO_TUNE);
    if (obj == NULL) {
        return NULL;
    }
    image->obj.alignment              = MID_RIGHT;
    image->obj.alignTo                = container->children[TEXT_INDEX];
    image->obj.alignmentMarginX       = NUMBER_TEXT_SPACE;
    image->obj.touchMask              = (1 << TOUCHED);
    image->obj.touchId                = ENTERED_TEXT_ID;
    container->children[DELETE_INDEX] = (nbgl_obj_t *) image;

    // create gray line
    nbgl_line_t *line = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
    line->lineColor   = LINE_COLOR;
    // align on bottom of the text entry container
    line->obj.alignment   = BOTTOM_MIDDLE;
    line->obj.area.width  = AVAILABLE_WIDTH;
    line->obj.area.height = LINE_THICKNESS;
    line->direction       = HORIZONTAL;
    line->thickness       = LINE_THICKNESS;
    // set this line as child of the text entry container
    container->children[LINE_INDEX] = (nbgl_obj_t *) line;

    // set this text entry container as child of the main container
    mainContainer->children[1] = (nbgl_obj_t *) container;
    mainContainer->obj.area.height += container->obj.area.height;

    return mainContainer;
}

static nbgl_container_t *addSuggestionButtons(nbgl_layoutInternal_t *layoutInt,
                                              uint8_t                nbUsedButtons,
                                              const char           **buttonTexts,
                                              int                    firstButtonToken,
                                              tune_index_e           tuneId)
{
    nbgl_container_t *suggestionsContainer;
    layoutObj_t      *obj;

    nbActiveButtons      = nbUsedButtons;
    suggestionsContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    suggestionsContainer->layout         = VERTICAL;
    suggestionsContainer->obj.area.width = SCREEN_WIDTH;
#ifndef USE_PARTIAL_BUTTONS
    // 2 rows of buttons with radius=32, and a intervale of 8px
    suggestionsContainer->obj.area.height = 2 * SMALL_BUTTON_HEIGHT + INTERNAL_MARGIN + 28;
#else   // USE_PARTIAL_BUTTONS
    // 1 row of buttons + 24px + page indicator
    suggestionsContainer->obj.area.height = SUGGESTION_CONTAINER_HEIGHT;
#endif  // USE_PARTIAL_BUTTONS
    suggestionsContainer->nbChildren = NB_SUGGESTION_CHILDREN;
    suggestionsContainer->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(NB_SUGGESTION_CHILDREN, layoutInt->layer);

    // put suggestionsContainer at the bottom of main container
    suggestionsContainer->obj.alignmentMarginY = BOTTOM_NORMAL_MARGIN;
    suggestionsContainer->obj.alignment        = BOTTOM_MIDDLE;

    // create all possible suggestion buttons, even if not displayed at first
    for (int i = 0; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
        nbgl_button_t *button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);

        obj = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) button, firstButtonToken + i, tuneId);
        if (obj == NULL) {
            return NULL;
        }

        button->innerColor      = BLACK;
        button->borderColor     = BLACK;
        button->foregroundColor = WHITE;
        button->obj.area.width
            = (SCREEN_WIDTH - (2 * SUGGESTION_BUTTONS_SIDE_MARGIN) - INTERNAL_MARGIN) / 2;
        button->obj.area.height = SMALL_BUTTON_HEIGHT;
        button->radius          = SMALL_BUTTON_RADIUS_INDEX;
        button->fontId          = SMALL_BOLD_1BPP_FONT;
        button->text            = buttonTexts[i];
        button->obj.touchMask   = (1 << TOUCHED);
        button->obj.touchId     = CONTROLS_ID + i;

        suggestionsContainer->children[i + FIRST_BUTTON_INDEX] = (nbgl_obj_t *) button;
    }
    // The first child is used by the progress indicator, if more that
    // NB_MAX_VISIBLE_SUGGESTION_BUTTONS buttons
    nbgl_page_indicator_t *indicator
        = (nbgl_page_indicator_t *) nbgl_objPoolGet(PAGE_INDICATOR, layoutInt->layer);
    indicator->activePage = 0;
    indicator->nbPages    = (nbActiveButtons + NB_MAX_VISIBLE_SUGGESTION_BUTTONS - 1)
                         / NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
    indicator->obj.area.width
        = (indicator->nbPages < 3) ? STEPPER_2_PAGES_WIDTH : STEPPER_N_PAGES_WIDTH;
    indicator->obj.alignment                             = BOTTOM_MIDDLE;
    indicator->style                                     = CURRENT_INDICATOR;
    suggestionsContainer->children[PAGE_INDICATOR_INDEX] = (nbgl_obj_t *) indicator;
#ifdef USE_PARTIAL_BUTTONS
    // also allocate the semi disc that may be displayed on the left or right of the full
    // buttons
    nbgl_objPoolGetArray(IMAGE, 2, layoutInt->layer, (nbgl_obj_t **) &partialButtonImages);
    partialButtonImages[0]->buffer          = &LEFT_HALF_ICON;
    partialButtonImages[0]->obj.alignment   = TOP_LEFT;
    partialButtonImages[0]->foregroundColor = BLACK;
    partialButtonImages[0]->transformation  = VERTICAL_MIRROR;
    partialButtonImages[1]->buffer          = &LEFT_HALF_ICON;
    partialButtonImages[1]->obj.alignment   = TOP_RIGHT;
    partialButtonImages[1]->foregroundColor = BLACK;
    partialButtonImages[1]->transformation  = NO_TRANSFORMATION;
    updateSuggestionButtons(suggestionsContainer, 0, 0);
#endif  // USE_PARTIAL_BUTTONS

    return suggestionsContainer;
}

static nbgl_button_t *addConfirmationButton(nbgl_layoutInternal_t *layoutInt,
                                            bool                   active,
                                            const char            *text,
                                            int                    token,
                                            tune_index_e           tuneId,
                                            bool                   compactMode)
{
    nbgl_button_t *button;
    layoutObj_t   *obj;

    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
    obj    = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) button, token, tuneId);
    if (obj == NULL) {
        return NULL;
    }

    // put button at the bottom of the main container
    button->obj.alignmentMarginY = compactMode ? BOTTOM_COMPACT_MARGIN : BOTTOM_CONFIRM_MARGIN;
    button->obj.alignment        = BOTTOM_MIDDLE;
    button->foregroundColor      = WHITE;
    if (active) {
        button->innerColor    = BLACK;
        button->borderColor   = BLACK;
        button->obj.touchMask = (1 << TOUCHED);
        button->obj.touchId   = BOTTOM_BUTTON_ID;
    }
    else {
        button->borderColor = INACTIVE_COLOR;
        button->innerColor  = INACTIVE_COLOR;
    }
    button->text            = PIC(text);
    button->fontId          = SMALL_BOLD_1BPP_FONT;
    button->obj.area.width  = AVAILABLE_WIDTH;
    button->obj.area.height = BUTTON_DIAMETER;
    button->radius          = BUTTON_RADIUS;

    return button;
}

/**********************
 *   GLOBAL API FUNCTIONS
 **********************/

/**
 * @brief Creates a keyboard on bottom of the screen, with the given configuration
 *
 * @param layout the current layout
 * @param kbdInfo configuration of the keyboard to draw (including the callback when touched)
 * @return the index of keyboard, to use in @ref nbgl_layoutUpdateKeyboard()
 */
int nbgl_layoutAddKeyboard(nbgl_layout_t *layout, const nbgl_layoutKbd_t *kbdInfo)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_keyboard_t       *keyboard;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddKeyboard():\n");
    if (layout == NULL) {
        return -1;
    }
    // footer must be empty
    if (layoutInt->footerContainer != NULL) {
        return -1;
    }

    // create keyboard
    keyboard                  = (nbgl_keyboard_t *) nbgl_objPoolGet(KEYBOARD, layoutInt->layer);
    keyboard->obj.area.width  = SCREEN_WIDTH;
    keyboard->obj.area.height = 3 * KEYBOARD_KEY_HEIGHT;
    if (!kbdInfo->lettersOnly) {
        keyboard->obj.area.height += KEYBOARD_KEY_HEIGHT;
    }
#ifdef TARGET_STAX
    keyboard->obj.alignmentMarginY = 56;
#endif  // TARGET_STAX
    keyboard->obj.alignment = BOTTOM_MIDDLE;
    keyboard->borderColor   = LIGHT_GRAY;
    keyboard->callback      = PIC(kbdInfo->callback);
    keyboard->lettersOnly   = kbdInfo->lettersOnly;
    keyboard->mode          = kbdInfo->mode;
    keyboard->keyMask       = kbdInfo->keyMask;
    keyboard->casing        = kbdInfo->casing;

    // the keyboard occupies the footer
    layoutInt->footerContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    layoutInt->footerContainer->obj.area.width = SCREEN_WIDTH;
    layoutInt->footerContainer->layout         = VERTICAL;
    layoutInt->footerContainer->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(1, layoutInt->layer);
    layoutInt->footerContainer->obj.alignment = BOTTOM_MIDDLE;
    layoutInt->footerContainer->obj.area.height
        = keyboard->obj.area.height + keyboard->obj.alignmentMarginY;
    layoutInt->footerContainer->children[0] = (nbgl_obj_t *) keyboard;
    layoutInt->footerContainer->nbChildren  = 1;

    // add footer to layout children
    layoutInt->children[FOOTER_INDEX] = (nbgl_obj_t *) layoutInt->footerContainer;

    // subtract footer height from main container height
    layoutInt->container->obj.area.height -= layoutInt->footerContainer->obj.area.height;

    // create the 2 children for main container (to hold keyboard content)
    layoutAddObject(layoutInt, (nbgl_obj_t *) NULL);
    layoutAddObject(layoutInt, (nbgl_obj_t *) NULL);

    layoutInt->footerType = KEYBOARD_FOOTER_TYPE;

    return layoutInt->footerContainer->obj.area.height;
}

/**
 * @brief Updates an existing keyboard on bottom of the screen, with the given configuration
 *
 * @param layout the current layout
 * @param index index returned by @ref nbgl_layoutAddKeyboard() (unused)
 * @param keyMask mask of keys to activate/deactivate on keyboard
 * @param updateCasing if true, update keyboard casing with given value
 * @param casing  casing to use
 * @return >=0 if OK
 */
int nbgl_layoutUpdateKeyboard(nbgl_layout_t *layout,
                              uint8_t        index,
                              uint32_t       keyMask,
                              bool           updateCasing,
                              keyboardCase_t casing)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_keyboard_t       *keyboard;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateKeyboard(): keyMask = 0x%X\n", keyMask);
    if (layout == NULL) {
        return -1;
    }
    UNUSED(index);

    // get existing keyboard (in the footer container)
    keyboard = (nbgl_keyboard_t *) layoutInt->footerContainer->children[0];
    if ((keyboard == NULL) || (keyboard->obj.type != KEYBOARD)) {
        return -1;
    }
    keyboard->keyMask = keyMask;
    if (updateCasing) {
        keyboard->casing = casing;
    }

    nbgl_objDraw((nbgl_obj_t *) keyboard);

    return 0;
}

/**
 * @brief function called to know whether the keyboard has been redrawn and needs a refresh
 *
 * @param layout the current layout
 * @param index index returned by @ref nbgl_layoutAddKeyboard() (unused)
 * @return true if keyboard needs a refresh
 */
bool nbgl_layoutKeyboardNeedsRefresh(nbgl_layout_t *layout, uint8_t index)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_keyboard_t       *keyboard;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutKeyboardNeedsRefresh(): \n");
    if (layout == NULL) {
        return -1;
    }
    UNUSED(index);

    // get existing keyboard (in the footer container)
    keyboard = (nbgl_keyboard_t *) layoutInt->footerContainer->children[0];
    if ((keyboard == NULL) || (keyboard->obj.type != KEYBOARD)) {
        return -1;
    }
    if (keyboard->needsRefresh) {
        keyboard->needsRefresh = false;
        return true;
    }

    return false;
}

/**
 * @brief Adds a "text entry" area under the previously entered object. This area can be preceded
 * (beginning of line) by an index, indicating for example the entered world. A vertical gray line
 * is placed under the text. This text must be vertical placed in the screen with offsetY
 * @deprecated Use @ref nbgl_layoutAddKeyboardContent instead
 *
 * @note This area is touchable
 *
 * @param layout the current layout
 * @param numbered if true, the "number" param is used as index
 * @param number index of the text
 * @param text string to display in the area
 * @param grayedOut if true, the text is grayed out (unused)
 * @param offsetY vertical offset from the top of the page
 * @param token token provided in onActionCallback when the "cross" is touched
 * @return >= 0 if OK
 */
int nbgl_layoutAddEnteredText(nbgl_layout_t *layout,
                              bool           numbered,
                              uint8_t        number,
                              const char    *text,
                              bool           grayedOut,
                              int            offsetY,
                              int            token)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    uint8_t                enteredTextIndex = (layoutInt->container->nbChildren == 2) ? 0 : 1;
    bool compactMode = ((layoutInt->container->children[enteredTextIndex + 1] != NULL)
                        && (layoutInt->container->children[enteredTextIndex + 1]->type == BUTTON)
                        && (layoutInt->container->nbChildren == 3));

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddEnteredText():\n");
    if (layout == NULL) {
        return -1;
    }
    UNUSED(offsetY);
    UNUSED(grayedOut);

    container = addTextEntry(layoutInt, NULL, text, numbered, number, token, compactMode);

    // set this container as first or 2nd child of the main layout container
    layoutInt->container->children[enteredTextIndex] = (nbgl_obj_t *) container;

    if (layoutInt->container->children[enteredTextIndex + 1] != NULL) {
        if (layoutInt->container->children[enteredTextIndex + 1]->type == BUTTON) {
            nbgl_button_t *button
                = (nbgl_button_t *) layoutInt->container->children[enteredTextIndex + 1];
            container->obj.alignmentMarginY
                -= (button->obj.area.height + button->obj.alignmentMarginY
                    + (compactMode ? TOP_COMPACT_MARGIN : TOP_NORMAL_MARGIN))
                   / 2;
        }
        else if (layoutInt->container->children[enteredTextIndex + 1]->type == CONTAINER) {
            nbgl_container_t *suggestionContainer
                = (nbgl_container_t *) layoutInt->container->children[enteredTextIndex + 1];
            container->obj.alignmentMarginY
                -= (suggestionContainer->obj.area.height + suggestionContainer->obj.alignmentMarginY
                    + (compactMode ? TOP_COMPACT_MARGIN : TOP_NORMAL_MARGIN))
                   / 2;
        }
    }
    // if a centered info has be used for title, entered text is the second child and we have to
    // adjust layout
    if (layoutInt->container->nbChildren == 3) {
        container->obj.alignmentMarginY += layoutInt->container->children[0]->area.height / 2;
    }

    // return 0
    return 0;
}

/**
 * @brief Updates an existing "text entry" area, created with @ref nbgl_layoutAddEnteredText()
 * @deprecated Use @ref nbgl_layoutUpdateKeyboardContent instead
 *
 * @param layout the current layout
 * @param index index of the text (return value of @ref nbgl_layoutAddEnteredText())
 * @param numbered if set to true, the text is preceded on the left by 'number.'
 * @param number if numbered is true, number used to build 'number.' text
 * @param text string to display in the area
 * @param grayedOut if true, the text is grayed out (but not the potential number)
 * @return <0 if error, 0 if OK with text fitting the area, 1 of 0K with text
 * not fitting the area
 */
int nbgl_layoutUpdateEnteredText(nbgl_layout_t *layout,
                                 uint8_t        index,
                                 bool           numbered,
                                 uint8_t        number,
                                 const char    *text,
                                 bool           grayedOut)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_text_area_t      *textArea;
    uint8_t                enteredTextIndex = (layoutInt->container->nbChildren == 2) ? 0 : 1;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateEnteredText():\n");
    if (layout == NULL) {
        return -1;
    }
    UNUSED(index);

    // update text entry area
    container = (nbgl_container_t *) layoutInt->container->children[enteredTextIndex];
    if ((container == NULL) || (container->obj.type != CONTAINER)) {
        return -1;
    }
    textArea = (nbgl_text_area_t *) container->children[2];
    if ((textArea == NULL) || (textArea->obj.type != TEXT_AREA)) {
        return -1;
    }
    textArea->text          = text;
    textArea->textColor     = grayedOut ? INACTIVE_TEXT_COLOR : BLACK;
    textArea->textAlignment = MID_LEFT;
    nbgl_objDraw((nbgl_obj_t *) textArea);

    // update number text area
    if (numbered) {
        // it is the previously created object
        textArea = (nbgl_text_area_t *) layoutInt->container->children[1];
        snprintf(numText, sizeof(numText), "%d.", number);
        textArea->text = numText;
        nbgl_objDraw((nbgl_obj_t *) textArea);
    }
    // if the text doesn't fit, indicate it by returning 1 instead of 0, for different refresh
    if (nbgl_getSingleLineTextWidth(textArea->fontId, text) > textArea->obj.area.width) {
        return 1;
    }
    return 0;
}

/**
 * @brief Adds a black full width confirmation button on top of the previously added keyboard.
 * @deprecated Use @ref nbgl_layoutAddKeyboardContent instead
 *
 * @param layout the current layout
 * @param active if true, button is active, otherwise inactive (grayed-out)
 * @param text text of the button
 * @param token token of the button, used in onActionCallback
 * @param tuneId tune to play when button is pressed
 * @return >= 0 if OK
 */
int nbgl_layoutAddConfirmationButton(nbgl_layout_t *layout,
                                     bool           active,
                                     const char    *text,
                                     int            token,
                                     tune_index_e   tuneId)
{
    nbgl_button_t         *button;
    nbgl_layoutInternal_t *layoutInt        = (nbgl_layoutInternal_t *) layout;
    uint8_t                enteredTextIndex = (layoutInt->container->nbChildren == 2) ? 0 : 1;
    bool                   compactMode      = (layoutInt->container->nbChildren == 3);

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddConfirmationButton():\n");
    if (layout == NULL) {
        return -1;
    }

    button = addConfirmationButton(layoutInt, active, text, token, tuneId, compactMode);
    // set this button as second child of the main layout container
    layoutInt->container->children[enteredTextIndex + 1] = (nbgl_obj_t *) button;
    if (layoutInt->container->children[enteredTextIndex] != NULL) {
        ((nbgl_container_t *) layoutInt->container->children[enteredTextIndex])
            ->obj.alignmentMarginY
            -= (button->obj.area.height + button->obj.alignmentMarginY
                + (compactMode ? TOP_COMPACT_MARGIN : TOP_NORMAL_MARGIN))
               / 2;
    }
    // return 0
    return 0;
}

/**
 * @brief Updates an existing black full width confirmation button on top of the previously added
keyboard.
 * @deprecated Use @ref nbgl_layoutUpdateKeyboardContent instead
 *
 * @param layout the current layout
 * @param index returned value of @ref nbgl_layoutAddConfirmationButton()
 * @param active if true, button is active
 * @param text text of the button
= * @return >= 0 if OK
 */
int nbgl_layoutUpdateConfirmationButton(nbgl_layout_t *layout,
                                        uint8_t        index,
                                        bool           active,
                                        const char    *text)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_button_t         *button;
    uint8_t                enteredTextIndex = (layoutInt->container->nbChildren == 2) ? 0 : 1;

    UNUSED(index);

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateConfirmationButton():\n");
    if (layout == NULL) {
        return -1;
    }

    // update main text area
    button = (nbgl_button_t *) layoutInt->container->children[enteredTextIndex + 1];
    if ((button == NULL) || (button->obj.type != BUTTON)) {
        return -1;
    }
    button->text = text;

    if (active) {
        button->innerColor    = BLACK;
        button->borderColor   = BLACK;
        button->obj.touchMask = (1 << TOUCHED);
        button->obj.touchId   = BOTTOM_BUTTON_ID;
    }
    else {
        button->borderColor = INACTIVE_COLOR;
        button->innerColor  = INACTIVE_COLOR;
    }
    nbgl_objDraw((nbgl_obj_t *) button);
    return 0;
}

/**
 * @brief Adds an area containing a potential title, a text entry and either confirmation
 * or suggestion buttons, on top of the keyboard
 *
 * @param layout the current layout
 * @param content structure containing the info
 * @return the height of the area if OK
 */
int nbgl_layoutAddKeyboardContent(nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *textEntryContainer;
    bool compactMode = ((content->type == KEYBOARD_WITH_BUTTON) && (content->title != NULL));

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddKeyboardContent():\n");
    if (layout == NULL) {
        return -1;
    }

    textEntryContainer = addTextEntry(layoutInt,
                                      content->title,
                                      content->text,
                                      content->numbered,
                                      content->number,
                                      content->textToken,
                                      compactMode);

    // set this container as first child of the main layout container
    layoutInt->container->children[0] = (nbgl_obj_t *) textEntryContainer;

    if (content->type == KEYBOARD_WITH_SUGGESTIONS) {
        nbgl_container_t *suggestionsContainer
            = addSuggestionButtons(layoutInt,
                                   content->suggestionButtons.nbUsedButtons,
                                   content->suggestionButtons.buttons,
                                   content->suggestionButtons.firstButtonToken,
                                   content->tuneId);
        // set this container as second child of the main layout container
        layoutInt->container->children[1] = (nbgl_obj_t *) suggestionsContainer;
        // the main container is swipable on Flex
        if (layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) layoutInt->container, 0, NBGL_NO_TUNE)
            == NULL) {
            return -1;
        }
        layoutInt->container->obj.touchMask = (1 << SWIPED_LEFT) | (1 << SWIPED_RIGHT);
        layoutInt->container->obj.touchId   = CONTROLS_ID;
        layoutInt->swipeUsage               = SWIPE_USAGE_SUGGESTIONS;
        textEntryContainer->obj.alignmentMarginY
            -= (suggestionsContainer->obj.area.height + suggestionsContainer->obj.alignmentMarginY
                + TOP_NORMAL_MARGIN)
               / 2;
    }
    else if (content->type == KEYBOARD_WITH_BUTTON) {
        nbgl_button_t *button = addConfirmationButton(layoutInt,
                                                      content->confirmationButton.active,
                                                      content->confirmationButton.text,
                                                      content->confirmationButton.token,
                                                      content->tuneId,
                                                      (content->title != NULL));
        // set this button as second child of the main layout container
        layoutInt->container->children[1] = (nbgl_obj_t *) button;
        textEntryContainer->obj.alignmentMarginY
            -= (button->obj.area.height + button->obj.alignmentMarginY
                + ((content->title != NULL) ? TOP_COMPACT_MARGIN : TOP_CONFIRM_MARGIN))
               / 2;
    }
    return layoutInt->container->obj.area.height;
}

/**
 * @brief Updates an area containing a potential title, a text entry and either confirmation
 * or suggestion buttons, on top of the keyboard
 * This area must have been built with @ref nbgl_layoutAddKeyboardContent, and the type must not
 * change
 *
 * @param layout the current layout
 * @param content structure containing the updated info
 * @return the height of the area if OK
 */
int nbgl_layoutUpdateKeyboardContent(nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *mainContainer, *container;
    nbgl_text_area_t      *textArea;
    nbgl_image_t          *image;
    int                    ret = 0;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateKeyboardContent():\n");
    if (layout == NULL) {
        return -1;
    }

    // get top container from main container (it shall be the 1st object)
    mainContainer = (nbgl_container_t *) layoutInt->container->children[0];
    container     = (nbgl_container_t *) mainContainer->children[1];

    if (content->numbered) {
        // get Word number typed text
        textArea = (nbgl_text_area_t *) container->children[NUMBER_INDEX];
        snprintf(numText, sizeof(numText), "%d.", content->number);
        nbgl_objDraw((nbgl_obj_t *) textArea);
    }

    // get text area for entered text
    textArea       = (nbgl_text_area_t *) container->children[TEXT_INDEX];
    textArea->text = content->text;
    nbgl_objDraw((nbgl_obj_t *) textArea);

    // get delete cross
    image = (nbgl_image_t *) container->children[DELETE_INDEX];
    if ((textArea->text != NULL) && (strlen(textArea->text) > 0)) {
        if (image->foregroundColor == WHITE) {
            image->foregroundColor = BLACK;
            nbgl_objDraw((nbgl_obj_t *) image);
        }
    }
    else {
        if (image->foregroundColor == BLACK) {
            image->foregroundColor = WHITE;
            nbgl_objDraw((nbgl_obj_t *) image);
        }
    }

    if (content->type == KEYBOARD_WITH_SUGGESTIONS) {
        uint8_t i       = 0;
        nbActiveButtons = content->suggestionButtons.nbUsedButtons;
        nbgl_container_t *suggestionsContainer
            = (nbgl_container_t *) layoutInt->container->children[1];

        // update suggestion texts
        for (i = 0; i < NB_MAX_SUGGESTION_BUTTONS; i++) {
            choiceTexts[i] = content->suggestionButtons.buttons[i];
        }
        // update suggestion buttons
        for (i = 0; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
            // some buttons may not be visible
            if (i < nbActiveButtons) {
                ((nbgl_button_t *) suggestionsContainer->children[i + FIRST_BUTTON_INDEX])->text
                    = choiceTexts[i];
            }
            else {
                ((nbgl_button_t *) suggestionsContainer->children[i + FIRST_BUTTON_INDEX])->text
                    = NULL;
            }
        }
        suggestionsContainer->forceClean = true;
        // the first child is used by the progress indicator, if more than
        // NB_MAX_VISIBLE_SUGGESTION_BUTTONS buttons
        nbgl_page_indicator_t *indicator
            = (nbgl_page_indicator_t *) suggestionsContainer->children[PAGE_INDICATOR_INDEX];
        indicator->nbPages = (nbActiveButtons + NB_MAX_VISIBLE_SUGGESTION_BUTTONS - 1)
                             / NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
        indicator->activePage = 0;
        updateSuggestionButtons(suggestionsContainer, 0, 0);

        nbgl_objDraw((nbgl_obj_t *) suggestionsContainer);
    }
    else if (content->type == KEYBOARD_WITH_BUTTON) {
        // update main text area
        nbgl_button_t *button = (nbgl_button_t *) layoutInt->container->children[1];
        if ((button == NULL) || (button->obj.type != BUTTON)) {
            return -1;
        }
        button->text = content->confirmationButton.text;

        if (content->confirmationButton.active) {
#if NB_COLOR_BITS == 4
            // if the button was inactive (in grey), a non-fast refresh will be necessary
            if (button->innerColor == INACTIVE_COLOR) {
                ret = 1;
            }
#endif
            button->innerColor    = BLACK;
            button->borderColor   = BLACK;
            button->obj.touchMask = (1 << TOUCHED);
            button->obj.touchId   = BOTTOM_BUTTON_ID;
        }
        else {
            button->borderColor = INACTIVE_COLOR;
            button->innerColor  = INACTIVE_COLOR;
        }
        nbgl_objDraw((nbgl_obj_t *) button);
    }

    // if the entered text doesn't fit, indicate it by returning 1 instead of 0, for different
    // refresh
    if (nbgl_getSingleLineTextWidth(textArea->fontId, content->text) > textArea->obj.area.width) {
        ret = 1;
    }
    return ret;
}

#endif  // NBGL_KEYBOARD
#endif  // HAVE_SE_TOUCH
