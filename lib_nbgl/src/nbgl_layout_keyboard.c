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
// for suggestion buttons, on Flex there are other objects than buttons
enum {
    PAGE_INDICATOR_INDEX = 0,
#ifndef TARGET_STAX
    LEFT_HALF_INDEX,   // half disc displayed on the bottom left
    RIGHT_HALF_INDEX,  // half disc displayed on the bottom right
#endif                 // TARGET_STAX
    FIRST_BUTTON_INDEX,
    SECOND_BUTTON_INDEX,
#ifdef TARGET_STAX
    THIRD_BUTTON_INDEX,
    FOURTH_BUTTON_INDEX,
#endif  // TARGET_STAX
    NB_SUGGESTION_CHILDREN
};

#ifdef TARGET_STAX
#define TEXT_ENTRY_NORMAL_HEIGHT  64
#define TEXT_ENTRY_COMPACT_HEIGHT 64
#define BOTTOM_NORMAL_MARGIN      24
#define BOTTOM_COMPACT_MARGIN     24
#define TOP_NORMAL_MARGIN         20
#define TOP_COMPACT_MARGIN        20
#else  // TARGET_STAX
#define TEXT_ENTRY_NORMAL_HEIGHT  72
#define TEXT_ENTRY_COMPACT_HEIGHT 56
#define BOTTOM_NORMAL_MARGIN      24
#define BOTTOM_COMPACT_MARGIN     12
#define TOP_NORMAL_MARGIN         20
#define TOP_COMPACT_MARGIN        12
#endif  // TARGET_STAX

// a horizontal line, even if displayed on 2 pixels, takes 4 pixels
#define LINE_REAL_HEIGHT 4

#define NUMBER_WIDTH 56

// space between number and text
#define NUMBER_TEXT_SPACE 8

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

static nbgl_button_t *choiceButtons[NB_MAX_SUGGESTION_BUTTONS];
static char           numText[5];
static uint8_t        nbActiveButtons;
#ifndef TARGET_STAX
static nbgl_image_t *partialButtonImages[2];
#endif  // TARGET_STAX

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
        container->children[FIRST_BUTTON_INDEX]
            = (nbgl_obj_t *) choiceButtons[currentLeftButtonIndex];

        for (i = 1; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
            if (currentLeftButtonIndex < (uint32_t) (nbActiveButtons - i)) {
                container->children[FIRST_BUTTON_INDEX + i]
                    = (nbgl_obj_t *) choiceButtons[currentLeftButtonIndex + i];
            }
            else {
                container->children[FIRST_BUTTON_INDEX + i] = NULL;
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
            container->children[FIRST_BUTTON_INDEX + i]
                = (nbgl_obj_t *) choiceButtons[currentLeftButtonIndex + i];
        }
        page        = currentLeftButtonIndex / NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
        needRefresh = true;
    }
    // align top-left button on the left
    if (container->children[FIRST_BUTTON_INDEX] != NULL) {
        container->children[FIRST_BUTTON_INDEX]->alignmentMarginX = BORDER_MARGIN;
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
#ifdef TARGET_STAX
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
#endif  // TARGET_STAX

    // the first child is used by the progress indicator, displayed if more that
    // NB_MAX_VISIBLE_SUGGESTION_BUTTONS buttons
    nbgl_page_indicator_t *indicator
        = (nbgl_page_indicator_t *) container->children[PAGE_INDICATOR_INDEX];
    indicator->activePage = page;

#ifndef TARGET_STAX
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
#endif  // TARGET_STAX
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
        && (suggestionsContainer->nbChildren == (nbActiveButtons + FIRST_BUTTON_INDEX))
        && (nbActiveButtons > NB_MAX_VISIBLE_SUGGESTION_BUTTONS)) {
        uint32_t i = 0;
        while (i < (uint32_t) nbActiveButtons) {
            if (suggestionsContainer->children[FIRST_BUTTON_INDEX]
                == (nbgl_obj_t *) choiceButtons[i]) {
                break;
            }
            i++;
        }

        if (i < (uint32_t) nbActiveButtons) {
            if (updateSuggestionButtons(suggestionsContainer, eventType, i)) {
                io_seproxyhal_play_tune(TUNE_TAP_CASUAL);
                nbgl_redrawObject((nbgl_obj_t *) suggestionsContainer, NULL, false);
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
                                      bool                   grayedOut,
                                      int                    textToken,
                                      bool                   compactMode)
{
    nbgl_container_t *container;
    nbgl_text_area_t *textArea;
    layoutObj_t      *obj;
    uint16_t textEntryHeight = (compactMode ? TEXT_ENTRY_COMPACT_HEIGHT : TEXT_ENTRY_NORMAL_HEIGHT);

    // create a container, to store title, entered text and underline
    container                 = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    container->nbChildren     = 4;
    container->children       = nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
    container->obj.area.width = AVAILABLE_WIDTH;
    container->obj.alignment  = CENTER;

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
        container->children[0]     = (nbgl_obj_t *) textArea;
        container->obj.area.height = textArea->obj.area.height + 4;
    }

    if (numbered) {
        // create Word num typed text
        textArea            = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor = BLACK;
        snprintf(numText, sizeof(numText), "%d.", number);
        textArea->text           = numText;
        textArea->textAlignment  = CENTER;
        textArea->fontId         = LARGE_MEDIUM_1BPP_FONT;
        textArea->obj.area.width = NUMBER_WIDTH;
        if (title != NULL) {
            textArea->obj.alignmentMarginY = 4 + LINE_REAL_HEIGHT;
            textArea->obj.alignTo          = container->children[0];
            textArea->obj.alignment        = BOTTOM_LEFT;
        }
        else {
            textArea->obj.alignmentMarginY = LINE_REAL_HEIGHT;
            textArea->obj.alignment        = TOP_LEFT;
        }
        textArea->obj.area.height = textEntryHeight - 2 * LINE_REAL_HEIGHT;
        // set this text area as child of the container
        container->children[1] = (nbgl_obj_t *) textArea;
    }

    // create text area for entered text
    textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor     = grayedOut ? LIGHT_GRAY : BLACK;
    textArea->text          = text;
    textArea->textAlignment = MID_LEFT;
    textArea->fontId        = LARGE_MEDIUM_1BPP_FONT;
    if (title != NULL) {
        textArea->obj.alignmentMarginY = 4 + LINE_REAL_HEIGHT;
        textArea->obj.alignTo          = container->children[0];
        textArea->obj.alignment        = BOTTOM_LEFT;
    }
    else {
        textArea->obj.alignmentMarginY = LINE_REAL_HEIGHT;
        textArea->obj.alignment        = TOP_LEFT;
    }
    textArea->obj.area.width = AVAILABLE_WIDTH;
    if (numbered) {
        textArea->obj.alignmentMarginX = NUMBER_WIDTH + NUMBER_TEXT_SPACE;
        textArea->obj.area.width -= textArea->obj.alignmentMarginX;
    }
    textArea->obj.area.height  = textEntryHeight - 2 * LINE_REAL_HEIGHT;
    textArea->autoHideLongLine = true;

    obj = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) textArea, textToken, NBGL_NO_TUNE);
    if (obj == NULL) {
        return NULL;
    }
    textArea->obj.touchMask = (1 << TOUCHED);
    textArea->obj.touchId   = ENTERED_TEXT_ID;
    container->children[2]  = (nbgl_obj_t *) textArea;
    container->obj.area.height += textEntryHeight;

    // create gray line
    nbgl_line_t *line = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
    line->lineColor   = LIGHT_GRAY;
    // align on bottom of the container
    line->obj.alignment   = BOTTOM_MIDDLE;
    line->obj.area.width  = AVAILABLE_WIDTH;
    line->obj.area.height = LINE_REAL_HEIGHT;
    line->direction       = HORIZONTAL;
    line->thickness       = 2;
    line->offset          = 2;
    // set this line as child of the container
    container->children[3] = (nbgl_obj_t *) line;

    return container;
}

static nbgl_container_t *addSuggestionButtons(nbgl_layoutInternal_t *layoutInt,
                                              uint8_t                nbUsedButtons,
                                              const char           **buttonTexts,
                                              int                    firstButtonToken,
                                              tune_index_e           tuneId,
                                              bool                   compactMode)
{
    nbgl_container_t *suggestionsContainer;
    layoutObj_t      *obj;

    nbActiveButtons      = nbUsedButtons;
    suggestionsContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    suggestionsContainer->layout         = VERTICAL;
    suggestionsContainer->obj.area.width = SCREEN_WIDTH;
#ifdef TARGET_STAX
    // 2 rows of buttons with radius=32, and a intervale of 8px
    suggestionsContainer->obj.area.height = 2 * SMALL_BUTTON_HEIGHT + INTERNAL_MARGIN + 28;
#else   // TARGET_STAX
    // 1 row of buttons + 24px + page indicator
    suggestionsContainer->obj.area.height = SMALL_BUTTON_HEIGHT + 28;
#endif  // TARGET_STAX
    suggestionsContainer->nbChildren = nbActiveButtons + FIRST_BUTTON_INDEX;
    suggestionsContainer->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(NB_SUGGESTION_CHILDREN, layoutInt->layer);

    // put suggestionsContainer at 24px of the bottom of main container
    suggestionsContainer->obj.alignmentMarginY
        = compactMode ? BOTTOM_COMPACT_MARGIN : BOTTOM_NORMAL_MARGIN;
    suggestionsContainer->obj.alignment = BOTTOM_MIDDLE;

    // create all possible suggestion buttons, even if not displayed at first
    nbgl_objPoolGetArray(
        BUTTON, NB_MAX_SUGGESTION_BUTTONS, layoutInt->layer, (nbgl_obj_t **) &choiceButtons);
    for (int i = 0; i < NB_MAX_SUGGESTION_BUTTONS; i++) {
        obj = layoutAddCallbackObj(
            layoutInt, (nbgl_obj_t *) choiceButtons[i], firstButtonToken + i, tuneId);
        if (obj == NULL) {
            return NULL;
        }

        choiceButtons[i]->innerColor      = BLACK;
        choiceButtons[i]->borderColor     = BLACK;
        choiceButtons[i]->foregroundColor = WHITE;
        choiceButtons[i]->obj.area.width  = (AVAILABLE_WIDTH - INTERNAL_MARGIN) / 2;
        choiceButtons[i]->obj.area.height = SMALL_BUTTON_HEIGHT;
        choiceButtons[i]->radius          = RADIUS_32_PIXELS;
        choiceButtons[i]->fontId          = SMALL_BOLD_1BPP_FONT;
        choiceButtons[i]->text            = buttonTexts[i];
        choiceButtons[i]->obj.touchMask   = (1 << TOUCHED);
        choiceButtons[i]->obj.touchId     = CONTROLS_ID + i;
        // some buttons may not be visible
        if (i < MIN(NB_MAX_VISIBLE_SUGGESTION_BUTTONS, nbActiveButtons)) {
            suggestionsContainer->children[i + FIRST_BUTTON_INDEX]
                = (nbgl_obj_t *) choiceButtons[i];
        }
    }
    // The first child is used by the progress indicator, if more that
    // NB_MAX_VISIBLE_SUGGESTION_BUTTONS buttons
    nbgl_page_indicator_t *indicator
        = (nbgl_page_indicator_t *) nbgl_objPoolGet(PAGE_INDICATOR, layoutInt->layer);
    indicator->activePage = 0;
    indicator->nbPages    = (nbActiveButtons + NB_MAX_VISIBLE_SUGGESTION_BUTTONS - 1)
                         / NB_MAX_VISIBLE_SUGGESTION_BUTTONS;
    indicator->obj.area.width                            = 184;
    indicator->obj.alignment                             = BOTTOM_MIDDLE;
    indicator->style                                     = CURRENT_INDICATOR;
    suggestionsContainer->children[PAGE_INDICATOR_INDEX] = (nbgl_obj_t *) indicator;
#ifdef TARGET_FLEX
    // also allocate the semi disc that may be displayed on the left or right of the full
    // buttons
    nbgl_objPoolGetArray(IMAGE, 2, layoutInt->layer, (nbgl_obj_t **) &partialButtonImages);
    partialButtonImages[0]->buffer          = &C_left_half_64px;
    partialButtonImages[0]->obj.alignment   = TOP_LEFT;
    partialButtonImages[0]->foregroundColor = BLACK;
    partialButtonImages[0]->transformation  = VERTICAL_MIRROR;
    partialButtonImages[1]->buffer          = &C_left_half_64px;
    partialButtonImages[1]->obj.alignment   = TOP_RIGHT;
    partialButtonImages[1]->foregroundColor = BLACK;
    partialButtonImages[1]->transformation  = NO_TRANSFORMATION;
    updateSuggestionButtons(suggestionsContainer, 0, 0);
#endif  // TARGET_STAX

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

    // put button at 24px/12px of the keyboard
    button->obj.alignmentMarginY = compactMode ? BOTTOM_COMPACT_MARGIN : BOTTOM_NORMAL_MARGIN;
    button->obj.alignment        = BOTTOM_MIDDLE;
    button->foregroundColor      = WHITE;
    if (active) {
        button->innerColor    = BLACK;
        button->borderColor   = BLACK;
        button->obj.touchMask = (1 << TOUCHED);
        button->obj.touchId   = BOTTOM_BUTTON_ID;
    }
    else {
        button->borderColor = LIGHT_GRAY;
        button->innerColor  = LIGHT_GRAY;
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

    nbgl_redrawObject((nbgl_obj_t *) keyboard, NULL, false);

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
 * @brief Adds up to 4 black suggestion buttons under the previously added object
 * @deprecated Use @ref nbgl_layoutAddKeyboardContent instead
 *
 * @param layout the current layout
 * @param nbUsedButtons the number of actually used buttons
 * @param buttonTexts array of 4 strings for buttons (last ones can be NULL)
 * @param firstButtonToken first token used for buttons, provided in onActionCallback (the next 3
 * values will be used for other buttons)
 * @param tuneId tune to play when any button is pressed
 * @return >= 0 if OK
 */
int nbgl_layoutAddSuggestionButtons(nbgl_layout_t *layout,
                                    uint8_t        nbUsedButtons,
                                    const char    *buttonTexts[NB_MAX_SUGGESTION_BUTTONS],
                                    int            firstButtonToken,
                                    tune_index_e   tuneId)
{
    nbgl_container_t      *container;
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    // if a centered info has be used for title, entered text is the second child
    uint8_t enteredTextIndex = (layoutInt->container->nbChildren == 2) ? 0 : 1;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSuggestionButtons():\n");
    if (layout == NULL) {
        return -1;
    }

    container = addSuggestionButtons(
        layoutInt, nbUsedButtons, buttonTexts, firstButtonToken, tuneId, false);
    // set this container as 2nd or 3rd child of the main layout container
    layoutInt->container->children[enteredTextIndex + 1] = (nbgl_obj_t *) container;
    if (layoutInt->container->children[enteredTextIndex] != NULL) {
        ((nbgl_container_t *) layoutInt->container->children[enteredTextIndex])
            ->obj.alignmentMarginY
            -= (container->obj.area.height + container->obj.alignmentMarginY + 20) / 2;
    }
#ifdef TARGET_FLEX
    // the main container is swipable on Flex
    if (layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) layoutInt->container, 0, NBGL_NO_TUNE)
        == NULL) {
        return -1;
    }
    layoutInt->container->obj.touchMask = (1 << SWIPED_LEFT) | (1 << SWIPED_RIGHT);
    layoutInt->container->obj.touchId   = CONTROLS_ID;
    layoutInt->swipeUsage               = SWIPE_USAGE_SUGGESTIONS;
#endif  // TARGET_FLEX
    // set this new container as child of the main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    // return index of container to be modified later on
    return (layoutInt->container->nbChildren - 1);
}

/**
 * @brief Updates the number and/or the text suggestion buttons created with @ref
 * nbgl_layoutAddSuggestionButtons()
 * @deprecated Use @ref nbgl_layoutUpdateKeyboardContent instead
 *
 * @param layout the current layout
 * @param index index returned by @ref nbgl_layoutAddSuggestionButtons() (unused)
 * @param nbUsedButtons the number of actually used buttons
 * @param buttonTexts array of 4 strings for buttons (last ones can be NULL)
 * @return >= 0 if OK
 */
int nbgl_layoutUpdateSuggestionButtons(nbgl_layout_t *layout,
                                       uint8_t        index,
                                       uint8_t        nbUsedButtons,
                                       const char    *buttonTexts[NB_MAX_SUGGESTION_BUTTONS])
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    uint8_t                enteredTextIndex = (layoutInt->container->nbChildren == 2) ? 0 : 1;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateSuggestionButtons():\n");
    if (layout == NULL) {
        return -1;
    }
    UNUSED(index);

    container = (nbgl_container_t *) layoutInt->container->children[enteredTextIndex + 1];
    if ((container == NULL) || (container->obj.type != CONTAINER)) {
        return -1;
    }
    nbActiveButtons       = nbUsedButtons;
    container->nbChildren = nbUsedButtons + FIRST_BUTTON_INDEX;

    // update suggestion buttons
    for (int i = 0; i < NB_MAX_SUGGESTION_BUTTONS; i++) {
        choiceButtons[i]->text = buttonTexts[i];
        // some buttons may not be visible
        if (i < MIN(NB_MAX_VISIBLE_SUGGESTION_BUTTONS, nbUsedButtons)) {
            if ((i % 2) == 0) {
                choiceButtons[i]->obj.alignmentMarginX = BORDER_MARGIN;
#ifdef TARGET_STAX
                // second row 8px under the first one
                if (i != 0) {
                    choiceButtons[i]->obj.alignmentMarginY = INTERNAL_MARGIN;
                }
                choiceButtons[i]->obj.alignment = NO_ALIGNMENT;
#else   // TARGET_STAX
                if (i == 0) {
                    choiceButtons[i]->obj.alignment = TOP_LEFT;
                }
#endif  // TARGET_STAX
            }
            else {
                choiceButtons[i]->obj.alignmentMarginX = INTERNAL_MARGIN;
                choiceButtons[i]->obj.alignment        = MID_RIGHT;
                choiceButtons[i]->obj.alignTo          = (nbgl_obj_t *) choiceButtons[i - 1];
            }
            container->children[i + FIRST_BUTTON_INDEX] = (nbgl_obj_t *) choiceButtons[i];
        }
        else {
            container->children[i + FIRST_BUTTON_INDEX] = NULL;
        }
    }
    container->forceClean = true;
#ifndef TARGET_STAX
    // on Flex, the first child is used by the progress indicator, if more that 2 buttons
    nbgl_page_indicator_t *indicator
        = (nbgl_page_indicator_t *) container->children[PAGE_INDICATOR_INDEX];
    indicator->nbPages    = (nbUsedButtons + 1) / 2;
    indicator->activePage = 0;
    updateSuggestionButtons(container, 0, 0);
#endif  // TARGET_STAX

    nbgl_redrawObject((nbgl_obj_t *) container, NULL, false);

    return 0;
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
 * @param grayedOut if true, the text is grayed out (but not the potential number)
 * @param offsetY vertical offset from the top of the page
 * @param token token provided in onActionCallback when this area is touched
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

    container
        = addTextEntry(layoutInt, NULL, text, numbered, number, grayedOut, token, compactMode);

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
    textArea->textColor     = grayedOut ? LIGHT_GRAY : BLACK;
    textArea->textAlignment = MID_LEFT;
    nbgl_redrawObject((nbgl_obj_t *) textArea, NULL, false);

    // update number text area
    if (numbered) {
        // it is the previously created object
        textArea = (nbgl_text_area_t *) layoutInt->container->children[1];
        snprintf(numText, sizeof(numText), "%d.", number);
        textArea->text = numText;
        nbgl_redrawObject((nbgl_obj_t *) textArea, NULL, false);
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
        button->borderColor = LIGHT_GRAY;
        button->innerColor  = LIGHT_GRAY;
    }
    nbgl_redrawObject((nbgl_obj_t *) button, NULL, false);
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
    nbgl_container_t      *container;
    bool compactMode = ((content->type == KEYBOARD_WITH_BUTTON) && (content->title != NULL));

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddKeyboardContent():\n");
    if (layout == NULL) {
        return -1;
    }

    container = addTextEntry(layoutInt,
                             content->title,
                             content->text,
                             content->numbered,
                             content->number,
                             content->grayedOut,
                             content->textToken,
                             compactMode);

    // set this container as first child of the main layout container
    layoutInt->container->children[0] = (nbgl_obj_t *) container;

    if (content->type == KEYBOARD_WITH_SUGGESTIONS) {
        nbgl_container_t *suggestionsContainer
            = addSuggestionButtons(layoutInt,
                                   content->suggestionButtons.nbUsedButtons,
                                   content->suggestionButtons.buttons,
                                   content->suggestionButtons.firstButtonToken,
                                   content->tuneId,
                                   compactMode);
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
        container->obj.alignmentMarginY
            -= (suggestionsContainer->obj.area.height + suggestionsContainer->obj.alignmentMarginY
                + (compactMode ? TOP_COMPACT_MARGIN : TOP_NORMAL_MARGIN))
               / 2;
    }
    else if (content->type == KEYBOARD_WITH_BUTTON) {
        nbgl_button_t *button = addConfirmationButton(layoutInt,
                                                      content->confirmationButton.active,
                                                      content->confirmationButton.text,
                                                      content->confirmationButton.token,
                                                      content->tuneId,
                                                      compactMode);
        // set this button as second child of the main layout container
        layoutInt->container->children[1] = (nbgl_obj_t *) button;
        container->obj.alignmentMarginY
            -= (button->obj.area.height + button->obj.alignmentMarginY
                + (compactMode ? TOP_COMPACT_MARGIN : TOP_NORMAL_MARGIN))
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
    nbgl_container_t      *container;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutUpdateKeyboardContent():\n");
    if (layout == NULL) {
        return -1;
    }

    // get top container from main container (it shall be the 1st object)
    container = (nbgl_container_t *) layoutInt->container->children[0];

    if (content->numbered) {
        // get Word number typed text
        textArea = (nbgl_text_area_t *) container->children[1];
        snprintf(numText, sizeof(numText), "%d.", content->number);
        nbgl_redrawObject((nbgl_obj_t *) textArea, NULL, false);
    }

    // get text area for entered text
    textArea            = (nbgl_text_area_t *) container->children[2];
    textArea->textColor = content->grayedOut ? LIGHT_GRAY : BLACK;
    textArea->text      = content->text;
    nbgl_redrawObject((nbgl_obj_t *) textArea, NULL, false);

    if (content->type == KEYBOARD_WITH_SUGGESTIONS) {
        nbActiveButtons = content->suggestionButtons.nbUsedButtons;
        nbgl_container_t *suggestionsContainer
            = (nbgl_container_t *) layoutInt->container->children[1];
        suggestionsContainer->nbChildren = nbActiveButtons + FIRST_BUTTON_INDEX;

        // update suggestion buttons
        for (int i = 0; i < NB_MAX_SUGGESTION_BUTTONS; i++) {
            choiceButtons[i]->text = content->suggestionButtons.buttons[i];
            // some buttons may not be visible
            if (i < MIN(NB_MAX_VISIBLE_SUGGESTION_BUTTONS, nbActiveButtons)) {
                suggestionsContainer->children[i + FIRST_BUTTON_INDEX]
                    = (nbgl_obj_t *) choiceButtons[i];
            }
            else {
                suggestionsContainer->children[i + FIRST_BUTTON_INDEX] = NULL;
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

        nbgl_redrawObject((nbgl_obj_t *) suggestionsContainer, NULL, false);
    }
    else if (content->type == KEYBOARD_WITH_BUTTON) {
        // update main text area
        nbgl_button_t *button = (nbgl_button_t *) layoutInt->container->children[1];
        if ((button == NULL) || (button->obj.type != BUTTON)) {
            return -1;
        }
        button->text = content->confirmationButton.text;

        if (content->confirmationButton.active) {
            button->innerColor    = BLACK;
            button->borderColor   = BLACK;
            button->obj.touchMask = (1 << TOUCHED);
            button->obj.touchId   = BOTTOM_BUTTON_ID;
        }
        else {
            button->borderColor = LIGHT_GRAY;
            button->innerColor  = LIGHT_GRAY;
        }
        nbgl_redrawObject((nbgl_obj_t *) button, NULL, false);
    }

    // if the entered text doesn't fit, indicate it by returning 1 instead of 0, for different
    // refresh
    if (nbgl_getSingleLineTextWidth(textArea->fontId, content->text) > textArea->obj.area.width) {
        return 1;
    }
    return 0;
}

#endif  // NBGL_KEYBOARD
#endif  // HAVE_SE_TOUCH
