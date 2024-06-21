/**
 * @file nbgl_layout.c
 * @brief Implementation of predefined layouts management for Applications
 * @note This file applies only to wallet size products (Stax, Flex...)
 */

#include "app_config.h"

#ifdef HAVE_SE_TOUCH
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
// half internal margin, between items
#define INTERNAL_SPACE 16
// inner margin, between buttons
#define INNER_MARGIN   12

#define NB_MAX_LAYOUTS 3

// used by container
#define NB_MAX_CONTAINER_CHILDREN 20

#define TAG_VALUE_ICON_WIDTH 32

#ifdef TARGET_STAX
#define RADIO_CHOICE_HEIGHT 96
#define FOOTER_HEIGHT       80
#define BAR_INTERVALE       12
#define BACK_KEY_WIDTH      88
#else  // TARGET_STAX
#define RADIO_CHOICE_HEIGHT 92
#define FOOTER_HEIGHT       80
#define BAR_INTERVALE       16
#define BACK_KEY_WIDTH      104
#endif  // TARGET_STAX

// refresh period of the spinner, in ms
#define SPINNER_REFRESH_PERIOD 400

#ifdef TARGET_STAX
#define FIRST_BUTTON_INDEX 0
#else   // TARGET_STAX
// for suggestion buttons, on Flex there are other objects than buttons
enum {
    PAGE_INDICATOR_INDEX = 0,
    LEFT_HALF_INDEX,   // half disc displayed on the bottom left
    RIGHT_HALF_INDEX,  // half disc displayed on the bottom right
    FIRST_BUTTON_INDEX
};
#endif  // TARGET_STAX

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    TOUCHABLE_BAR_ITEM,
    SWITCH_ITEM,
    NB_ITEM_TYPES
} listItemType_t;

// used to build either a touchable bar or a switch
typedef struct {
    listItemType_t             type;
    const nbgl_icon_details_t *iconLeft;   // a buffer containing the 1BPP icon for icon on
                                           // left (can be NULL)
    const nbgl_icon_details_t *iconRight;  // a buffer containing the 1BPP icon for icon 2 (can be
                                           // NULL). Dimensions must be the same as iconLeft
    const char  *text;                     // text (can be NULL)
    const char  *subText;                  // sub text (can be NULL)
    uint8_t      token;  // the token that will be used as argument of the callback
    nbgl_state_t state;  // state of the item
    bool         large;  // set to true only for the main level of OS settings
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  // if not @ref NBGL_NO_TUNE, a tune will be played
#endif                    // HAVE_PIEZO_SOUND
} listItem_t;

/**********************
 *      VARIABLES
 **********************/

/**
 * @brief array of layouts, if used by modal
 *
 */
static nbgl_layoutInternal_t gLayout[NB_MAX_LAYOUTS] = {0};

// numbers of touchable controls for the whole page
static uint8_t nbTouchableControls = 0;

/**********************
 *  STATIC PROTOTYPES
 **********************/

#ifdef HAVE_FAST_HOLD_TO_APPROVE
// Unit step in % of touchable progress bar
#define HOLD_TO_APPROVE_STEP_PERCENT     (7)
// Duration in ms the user must hold the progress bar
// to make it progress HOLD_TO_APPROVE_STEP_PERCENT %.
// This duration must be higher than the screen refresh duration.
#define HOLD_TO_APPROVE_STEP_DURATION_MS (100)
#else
#define HOLD_TO_APPROVE_STEP_PERCENT     (25)
#define HOLD_TO_APPROVE_STEP_DURATION_MS (400)
#endif  // HAVE_FAST_HOLD_TO_APPROVE

static inline uint8_t get_hold_to_approve_percent(uint32_t touch_duration)
{
#ifdef HAVE_FAST_HOLD_TO_APPROVE
    uint8_t current_step_nb = (touch_duration / HOLD_TO_APPROVE_STEP_DURATION_MS);
#else
    uint8_t current_step_nb = (touch_duration / HOLD_TO_APPROVE_STEP_DURATION_MS) + 1;
#endif
    return (current_step_nb * HOLD_TO_APPROVE_STEP_PERCENT);
}

// function used to retrieve the concerned layout and layout obj matching the given touched obj
static bool getLayoutAndLayoutObj(nbgl_obj_t             *obj,
                                  nbgl_layoutInternal_t **layout,
                                  layoutObj_t           **layoutObj)
{
    uint8_t i = NB_MAX_LAYOUTS;

    // parse all layouts (starting with modals) to find the object
    *layout = NULL;
    while (i > 0) {
        i--;
        if (gLayout[i].nbChildren > 0) {
            uint8_t j;

            // search index of obj in this layout
            for (j = 0; j < gLayout[i].nbUsedCallbackObjs; j++) {
                if (obj == gLayout[i].callbackObjPool[j].obj) {
                    LOG_DEBUG(LAYOUT_LOGGER,
                              "getLayoutAndLayoutObj(): obj found in layout[%d], index = %d, "
                              "nbUsedCallbackObjs = %d\n",
                              i,
                              j,
                              gLayout[i].nbUsedCallbackObjs);
                    *layout    = &gLayout[i];
                    *layoutObj = &(gLayout[i].callbackObjPool[j]);
                    return true;
                }
            }
        }
    }
    // not found
    return false;
}

static void radioTouchCallback(nbgl_obj_t            *obj,
                               nbgl_touchType_t       eventType,
                               nbgl_layoutInternal_t *layout);
static void longTouchCallback(nbgl_obj_t            *obj,
                              nbgl_touchType_t       eventType,
                              nbgl_layoutInternal_t *layout,
                              layoutObj_t           *layoutObj);

// callback for most touched object
static void touchCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType)
{
    nbgl_layoutInternal_t *layout;
    layoutObj_t           *layoutObj;
    bool                   needRefresh = false;

    if (obj == NULL) {
        return;
    }
    LOG_DEBUG(LAYOUT_LOGGER, "touchCallback(): eventType = %d, obj = %p\n", eventType, obj);
    if (getLayoutAndLayoutObj(obj, &layout, &layoutObj) == false) {
        // try with parent, if existing
        if (getLayoutAndLayoutObj(obj->parent, &layout, &layoutObj) == false) {
            LOG_WARN(
                LAYOUT_LOGGER,
                "touchCallback(): eventType = %d, obj = %p, no active layout or obj not found\n",
                eventType,
                obj);
            return;
        }
    }

    // case of swipe
    if (((eventType == SWIPED_UP) || (eventType == SWIPED_DOWN) || (eventType == SWIPED_LEFT)
         || (eventType == SWIPED_RIGHT))
        && (obj->type == CONTAINER)) {
#ifdef NBGL_KEYBOARD
        if (layout->swipeUsage == SWIPE_USAGE_SUGGESTIONS) {
            keyboardSwipeCallback(obj, eventType);
            return;
        }
#endif  // NBGL_KEYBOARD
        if (layout->swipeUsage == SWIPE_USAGE_CUSTOM) {
            layoutObj->index = eventType;
        }
        else if ((layout->swipeUsage == SWIPE_USAGE_NAVIGATION)
                 && ((nbgl_obj_t *) layout->container == obj)) {
            nbgl_container_t *navContainer;
            if (layout->footerType == FOOTER_NAV) {
                navContainer = (nbgl_container_t *) layout->footerContainer;
            }
            else if (layout->footerType == FOOTER_TEXT_AND_NAV) {
                navContainer = (nbgl_container_t *) layout->footerContainer->children[1];
            }
            else {
                return;
            }

            if (layoutNavigationCallback(
                    (nbgl_obj_t *) navContainer, eventType, layout->nbPages, &layout->activePage)
                == false) {
                // navigation was impossible
                return;
            }
            layoutObj->index = layout->activePage;
        }
    }

    // case of navigation bar
    if (((obj->parent == (nbgl_obj_t *) layout->footerContainer)
         && (layout->footerType == FOOTER_NAV))
        || ((obj->parent->type == CONTAINER)
            && (obj->parent->parent == (nbgl_obj_t *) layout->footerContainer)
            && (layout->footerType == FOOTER_TEXT_AND_NAV))) {
        if (layoutNavigationCallback(obj, eventType, layout->nbPages, &layout->activePage)
            == false) {
            // navigation was impossible
            return;
        }
        layoutObj->index = layout->activePage;
    }

    // case of switch
    if ((obj->type == CONTAINER) && (((nbgl_container_t *) obj)->nbChildren >= 2)
        && (((nbgl_container_t *) obj)->children[1] != NULL)
        && (((nbgl_container_t *) obj)->children[1]->type == SWITCH)) {
        nbgl_switch_t *lSwitch = (nbgl_switch_t *) ((nbgl_container_t *) obj)->children[1];
        lSwitch->state         = (lSwitch->state == ON_STATE) ? OFF_STATE : ON_STATE;
        nbgl_redrawObject((nbgl_obj_t *) lSwitch, false, false);
        // refresh will be done after tune playback
        needRefresh = true;
        // index is used for state
        layoutObj->index = lSwitch->state;
    }
    // case of radio
    else if ((obj->type == CONTAINER) && (((nbgl_container_t *) obj)->nbChildren == 2)
             && (((nbgl_container_t *) obj)->children[1] != NULL)
             && (((nbgl_container_t *) obj)->children[1]->type == RADIO_BUTTON)) {
        radioTouchCallback(obj, eventType, layout);
        return;
    }
    // case of long press
    else if ((obj->type == CONTAINER) && (((nbgl_container_t *) obj)->nbChildren == 4)
             && (((nbgl_container_t *) obj)->children[3] != NULL)
             && (((nbgl_container_t *) obj)->children[3]->type == PROGRESS_BAR)) {
        longTouchCallback(obj, eventType, layout, layoutObj);
        return;
    }
    LOG_DEBUG(LAYOUT_LOGGER, "touchCallback(): layout->callback = %p\n", layout->callback);
    if (layout->callback != NULL) {
#ifdef HAVE_PIEZO_SOUND
        if (layoutObj->tuneId < NBGL_NO_TUNE) {
            io_seproxyhal_play_tune(layoutObj->tuneId);
        }
#endif  // HAVE_PIEZO_SOUND
        if (needRefresh) {
            nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
        }
        layout->callback(layoutObj->token, layoutObj->index);
    }
}

// callback for long press button
static void longTouchCallback(nbgl_obj_t            *obj,
                              nbgl_touchType_t       eventType,
                              nbgl_layoutInternal_t *layout,
                              layoutObj_t           *layoutObj)
{
    nbgl_container_t *container = (nbgl_container_t *) obj;
    // 4th child of container is the progress bar
    nbgl_progress_bar_t *progressBar = (nbgl_progress_bar_t *) container->children[3];

    LOG_DEBUG(LAYOUT_LOGGER,
              "longTouchCallback(): eventType = %d, obj = %p, gLayout[1].nbChildren = %d\n",
              eventType,
              obj,
              gLayout[1].nbChildren);

    // case of pressing a long press button
    if (eventType == TOUCHING) {
        uint32_t touchDuration = nbgl_touchGetTouchDuration(obj);

        // Compute the new progress bar state in %
        uint8_t new_state = get_hold_to_approve_percent(touchDuration);

        // Ensure the callback is triggered once,
        // when the progress bar state reaches 100%
        bool trigger_callback = (new_state >= 100) && (progressBar->state < 100);

        // Cap progress bar state at 100%
        if (new_state >= 100) {
            new_state = 100;
        }

        // Update progress bar state
        if (new_state != progressBar->state) {
            progressBar->previousState = progressBar->state;
            progressBar->state         = new_state;

            nbgl_redrawObject((nbgl_obj_t *) progressBar, false, false);
            // Ensure progress bar is fully drawn
            // before calling the callback.
            nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_FAST_REFRESH,
                                               POST_REFRESH_FORCE_POWER_ON_WITH_PIPELINE);
        }

        if (trigger_callback) {
            // End of progress bar reached: trigger callback
            if (layout->callback != NULL) {
                layout->callback(layoutObj->token, layoutObj->index);
            }
        }
    }
    // case of releasing a long press button (or getting out of it)
    else if ((eventType == TOUCH_RELEASED) || (eventType == OUT_OF_TOUCH)
             || (eventType == SWIPED_LEFT) || (eventType == SWIPED_RIGHT)) {
        nbgl_wait_pipeline();
        progressBar->state = 0;
        nbgl_redrawObject((nbgl_obj_t *) progressBar, false, false);
        nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_REFRESH, POST_REFRESH_FORCE_POWER_OFF);
    }
}

// callback for radio button touch
static void radioTouchCallback(nbgl_obj_t            *obj,
                               nbgl_touchType_t       eventType,
                               nbgl_layoutInternal_t *layout)
{
    uint8_t i = NB_MAX_LAYOUTS, radioIndex = 0, foundRadio = 0xFF, foundRadioIndex;

    if (eventType != TOUCHED) {
        return;
    }

    i = 0;
    // parse all objs to find all containers of radio buttons
    while (i < layout->nbUsedCallbackObjs) {
        if ((obj == (nbgl_obj_t *) layout->callbackObjPool[i].obj)
            && (layout->callbackObjPool[i].obj->type == CONTAINER)) {
            nbgl_radio_t *radio
                = (nbgl_radio_t *) ((nbgl_container_t *) layout->callbackObjPool[i].obj)
                      ->children[1];
            nbgl_text_area_t *textArea
                = (nbgl_text_area_t *) ((nbgl_container_t *) layout->callbackObjPool[i].obj)
                      ->children[0];
            foundRadio      = i;
            foundRadioIndex = radioIndex;
            // set text as active (black and bold)
            textArea->textColor = BLACK;
            textArea->fontId    = SMALL_BOLD_FONT;
            // ensure that radio button is ON
            radio->state = ON_STATE;
            // redraw container
            nbgl_redrawObject((nbgl_obj_t *) obj, NULL, false);
        }
        else if ((layout->callbackObjPool[i].obj->type == CONTAINER)
                 && (((nbgl_container_t *) layout->callbackObjPool[i].obj)->nbChildren == 2)
                 && (((nbgl_container_t *) layout->callbackObjPool[i].obj)->children[1]->type
                     == RADIO_BUTTON)) {
            nbgl_radio_t *radio
                = (nbgl_radio_t *) ((nbgl_container_t *) layout->callbackObjPool[i].obj)
                      ->children[1];
            nbgl_text_area_t *textArea
                = (nbgl_text_area_t *) ((nbgl_container_t *) layout->callbackObjPool[i].obj)
                      ->children[0];
            radioIndex++;
            // set to OFF the one that was in ON
            if (radio->state == ON_STATE) {
                radio->state = OFF_STATE;
                // set text it as inactive (gray and normal)
                textArea->textColor = DARK_GRAY;
                textArea->fontId    = SMALL_REGULAR_FONT;
                // redraw container
                nbgl_redrawObject((nbgl_obj_t *) layout->callbackObjPool[i].obj, NULL, false);
            }
        }
        i++;
    }
    // call callback after redraw to avoid asynchronicity
    if (foundRadio != 0xFF) {
        if (layout->callback != NULL) {
#ifdef HAVE_PIEZO_SOUND
            if (layout->callbackObjPool[foundRadio].tuneId < NBGL_NO_TUNE) {
                io_seproxyhal_play_tune(layout->callbackObjPool[foundRadio].tuneId);
            }
            nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
#endif  // HAVE_PIEZO_SOUND
            layout->callback(layout->callbackObjPool[foundRadio].token, foundRadioIndex);
        }
    }
}

// callback for spinner ticker
static void spinnerTickerCallback(void)
{
    nbgl_spinner_t        *spinner;
    uint8_t                i = 0;
    nbgl_layoutInternal_t *layout;

    // gLayout[1] is on top of gLayout[0] so if gLayout[1] is active, it must catch the event
    if (gLayout[1].nbChildren > 0) {
        layout = &gLayout[1];
    }
    else {
        layout = &gLayout[0];
    }

    // get index of obj
    while (i < layout->container->nbChildren) {
        if (layout->container->children[i]->type == SPINNER) {
            spinner = (nbgl_spinner_t *) layout->container->children[i];
            spinner->position++;
            spinner->position &= 3;  // modulo 4
            nbgl_redrawObject((nbgl_obj_t *) spinner, NULL, false);
            nbgl_refreshSpecial(BLACK_AND_WHITE_FAST_REFRESH);
            return;
        }
        i++;
    }
}

static nbgl_line_t *createHorizontalLine(uint8_t layer)
{
    nbgl_line_t *line;

    line                  = (nbgl_line_t *) nbgl_objPoolGet(LINE, layer);
    line->lineColor       = LIGHT_GRAY;
    line->obj.area.width  = SCREEN_WIDTH;
    line->obj.area.height = 4;
    line->direction       = HORIZONTAL;
    line->thickness       = 1;
    return line;
}

static nbgl_line_t *createLeftVerticalLine(uint8_t layer)
{
    nbgl_line_t *line;

    line                  = (nbgl_line_t *) nbgl_objPoolGet(LINE, layer);
    line->lineColor       = LIGHT_GRAY;
    line->obj.area.width  = 1;
    line->obj.area.height = SCREEN_HEIGHT;
    line->direction       = VERTICAL;
    line->thickness       = 1;
    line->obj.alignment   = MID_LEFT;
    return line;
}

// function adding a layout object in the callbackObjPool array for the given layout, and
// configuring it
layoutObj_t *layoutAddCallbackObj(nbgl_layoutInternal_t *layout,
                                  nbgl_obj_t            *obj,
                                  uint8_t                token,
                                  tune_index_e           tuneId)
{
    layoutObj_t *layoutObj = NULL;

    if (layout->nbUsedCallbackObjs < (LAYOUT_OBJ_POOL_LEN - 1)) {
        layoutObj = &layout->callbackObjPool[layout->nbUsedCallbackObjs];
        layout->nbUsedCallbackObjs++;
        layoutObj->obj    = obj;
        layoutObj->token  = token;
        layoutObj->tuneId = tuneId;
    }
    else {
        LOG_FATAL(LAYOUT_LOGGER, "layoutAddCallbackObj: no more callback obj\n");
    }

    return layoutObj;
}

/**
 * @brief adds the given obj to the main container
 *
 * @param layout
 * @param obj
 */
void layoutAddObject(nbgl_layoutInternal_t *layout, nbgl_obj_t *obj)
{
    if (layout->container->nbChildren == NB_MAX_CONTAINER_CHILDREN) {
        LOG_FATAL(LAYOUT_LOGGER, "layoutAddObject(): No more object\n");
    }
    layout->container->children[layout->container->nbChildren] = obj;
    layout->container->nbChildren++;
}

/**
 * @brief add the swipe feature to the main container
 *
 * @param layout the current layout
 * @param token the token that will be used as argument of the callback
 * @param swipesMask the type of swipes to be handled by the container
 * @param usage usage of the swipe (custom or navigation)
 * @return >= 0 if OK
 */
static int addSwipeInternal(nbgl_layoutInternal_t *layoutInt,
                            uint16_t               swipesMask,
                            nbgl_swipe_usage_t     usage,
                            uint8_t                token,
                            tune_index_e           tuneId)
{
    layoutObj_t *obj;

    if ((swipesMask & SWIPE_MASK) == 0) {
        return -1;
    }

    obj = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) layoutInt->container, token, tuneId);
    if (obj == NULL) {
        return -1;
    }
    layoutInt->container->obj.touchMask = swipesMask;
    layoutInt->swipeUsage               = usage;

    return 0;
}

/**
 * @brief Adds a item of a list. An item can be a touchable bar or a switch
 *
 * @param layout the current layout
 * @param barLayout the properties of the bar
 * @return the height of the bar if OK
 */
static nbgl_container_t *addListItem(nbgl_layoutInternal_t *layoutInt, const listItem_t *itemDesc)
{
    layoutObj_t      *obj;
    nbgl_text_area_t *textArea;
    nbgl_container_t *container;
    color_t color      = ((itemDesc->type == TOUCHABLE_BAR_ITEM) && (itemDesc->state == OFF_STATE))
                             ? LIGHT_GRAY
                             : BLACK;
    int16_t usedHeight = 40;

    LOG_DEBUG(LAYOUT_LOGGER, "addListItem():\n");

    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    obj       = layoutAddCallbackObj(
        layoutInt, (nbgl_obj_t *) container, itemDesc->token, itemDesc->tuneId);
    if (obj == NULL) {
        return NULL;
    }

    // get container children (up to 4: text +  left+right icons +  sub text)
    container->children   = nbgl_containerPoolGet(4, layoutInt->layer);
    container->nbChildren = 0;

    container->obj.area.width  = AVAILABLE_WIDTH;
    container->obj.area.height = itemDesc->large ? TOUCHABLE_MAIN_BAR_HEIGHT : TOUCHABLE_BAR_HEIGHT;
    container->layout          = HORIZONTAL;
    container->obj.alignmentMarginX = BORDER_MARGIN;
    container->obj.alignment        = NO_ALIGNMENT;
    container->obj.alignTo          = NULL;
    // the bar can only be touched if not inactive AND if one of the icon is present
    // otherwise it is seen as a title
    if (((itemDesc->type == TOUCHABLE_BAR_ITEM) && (itemDesc->state == ON_STATE))
        || (itemDesc->type == SWITCH_ITEM)) {
        container->obj.touchMask = (1 << TOUCHED);
        container->obj.touchId   = CONTROLS_ID + nbTouchableControls;
        nbTouchableControls++;
    }

    // allocate main text because always present
    textArea                 = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor      = color;
    textArea->text           = PIC(itemDesc->text);
    textArea->onDrawCallback = NULL;
    textArea->fontId         = SMALL_BOLD_FONT;
    textArea->wrapping       = true;
    textArea->obj.area.width = container->obj.area.width;
    if (itemDesc->iconLeft != NULL) {
        // reduce text width accordingly
        textArea->obj.area.width
            -= ((nbgl_icon_details_t *) PIC(itemDesc->iconLeft))->width + BAR_INTERVALE;
    }
    if (itemDesc->iconRight != NULL) {
        // reduce text width accordingly
        textArea->obj.area.width
            -= ((nbgl_icon_details_t *) PIC(itemDesc->iconRight))->width + BAR_INTERVALE;
    }
    else if (itemDesc->type == SWITCH_ITEM) {
        textArea->obj.area.width -= 60 + BAR_INTERVALE;
    }
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    usedHeight                                 = MAX(usedHeight, textArea->obj.area.height);
    textArea->style                            = NO_STYLE;
    textArea->obj.alignment                    = MID_LEFT;
    textArea->textAlignment                    = MID_LEFT;
    container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
    container->nbChildren++;

    // allocate left icon if present
    if (itemDesc->iconLeft != NULL) {
        nbgl_image_t *imageLeft    = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        imageLeft->foregroundColor = color;
        imageLeft->buffer          = PIC(itemDesc->iconLeft);
        // align at the left of text
        imageLeft->obj.alignment                   = MID_LEFT;
        imageLeft->obj.alignTo                     = (nbgl_obj_t *) textArea;
        imageLeft->obj.alignmentMarginX            = BAR_INTERVALE;
        container->children[container->nbChildren] = (nbgl_obj_t *) imageLeft;
        container->nbChildren++;

        textArea->obj.alignmentMarginX = imageLeft->buffer->width + BAR_INTERVALE;

        usedHeight = MAX(usedHeight, imageLeft->buffer->height);
    }
    // allocate right icon if present
    if (itemDesc->iconRight != NULL) {
        nbgl_image_t *imageRight    = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        imageRight->foregroundColor = color;
        imageRight->buffer          = PIC(itemDesc->iconRight);
        // align at the right of text
        imageRight->obj.alignment        = MID_RIGHT;
        imageRight->obj.alignmentMarginX = BAR_INTERVALE;
        imageRight->obj.alignTo          = (nbgl_obj_t *) textArea;

        container->children[container->nbChildren] = (nbgl_obj_t *) imageRight;
        container->nbChildren++;

        usedHeight = MAX(usedHeight, imageRight->buffer->height);
    }
    else if (itemDesc->type == SWITCH_ITEM) {
        nbgl_switch_t *switchObj = (nbgl_switch_t *) nbgl_objPoolGet(SWITCH, layoutInt->layer);
        switchObj->onColor       = BLACK;
        switchObj->offColor      = LIGHT_GRAY;
        switchObj->state         = itemDesc->state;
        switchObj->obj.alignment = MID_RIGHT;
        switchObj->obj.alignmentMarginX = BAR_INTERVALE;
        switchObj->obj.alignTo          = (nbgl_obj_t *) textArea;

        container->children[container->nbChildren] = (nbgl_obj_t *) switchObj;
        container->nbChildren++;
    }

    if (itemDesc->subText != NULL) {
        nbgl_text_area_t *subTextArea
            = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);

        subTextArea->textColor                     = color;
        subTextArea->text                          = PIC(itemDesc->subText);
        subTextArea->textAlignment                 = MID_LEFT;
        subTextArea->fontId                        = SMALL_REGULAR_FONT;
        subTextArea->style                         = NO_STYLE;
        subTextArea->wrapping                      = true;
        subTextArea->obj.alignment                 = MID_LEFT;
        subTextArea->obj.area.width                = container->obj.area.width;
        subTextArea->obj.area.height               = nbgl_getTextHeightInWidth(subTextArea->fontId,
                                                                 subTextArea->text,
                                                                 subTextArea->obj.area.width,
                                                                 subTextArea->wrapping);
        container->children[container->nbChildren] = (nbgl_obj_t *) subTextArea;
        container->nbChildren++;
        container->obj.area.height += subTextArea->obj.area.height + 12;

        // modify alignments to have sub-text under (icon left - text - icon right)
        textArea->obj.alignmentMarginY    = -(subTextArea->obj.area.height + 12) / 2;
        subTextArea->obj.alignmentMarginY = (usedHeight + 12) / 2;
    }

    // set this new container as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    return container;
}

/**
 * @brief Creates a container on the center of the main panel, with a possible icon,
 * and possible texts under it
 *
 * @param layout the current layout
 * @param info structure giving the description of the Content Center
 * @return the created container
 */
static nbgl_container_t *addContentCenter(nbgl_layoutInternal_t      *layoutInt,
                                          const nbgl_contentCenter_t *info)
{
    nbgl_container_t *container;
    nbgl_text_area_t *textArea   = NULL;
    nbgl_image_t     *image      = NULL;
    uint16_t          fullHeight = 0;

    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);

    // get container children
    container->nbChildren = 0;
    container->children   = nbgl_containerPoolGet(5, layoutInt->layer);

    // add icon if present
    if (info->icon != NULL) {
        image                       = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        image->foregroundColor      = BLACK;
        image->buffer               = PIC(info->icon);
        image->obj.alignment        = TOP_MIDDLE;
        image->obj.alignmentMarginY = info->iconHug;

        fullHeight += image->buffer->height + info->iconHug;
        container->children[container->nbChildren] = (nbgl_obj_t *) image;
        container->nbChildren++;
    }
    // add title if present
    if (info->title != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = BLACK;
        textArea->text          = PIC(info->title);
        textArea->textAlignment = CENTER;
        textArea->fontId        = LARGE_MEDIUM_FONT;
        textArea->wrapping      = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);

        // if not the first child, put on bottom of the previous, with a margin
        if (container->nbChildren > 0) {
            textArea->obj.alignment = BOTTOM_MIDDLE;
            textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
            textArea->obj.alignmentMarginY = BOTTOM_BORDER_MARGIN + info->iconHug;
        }
        else {
            textArea->obj.alignment = TOP_MIDDLE;
        }

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    // add small title if present
    if (info->smallTitle != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = BLACK;
        textArea->text          = PIC(info->smallTitle);
        textArea->textAlignment = CENTER;
        textArea->fontId        = SMALL_BOLD_FONT;
        textArea->wrapping      = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);

        // if not the first child, put on bottom of the previous, with a margin
        if (container->nbChildren > 0) {
            textArea->obj.alignment = BOTTOM_MIDDLE;
            textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
            textArea->obj.alignmentMarginY = BOTTOM_BORDER_MARGIN;
            if (container->children[container->nbChildren - 1]->type == IMAGE) {
                textArea->obj.alignmentMarginY = BOTTOM_BORDER_MARGIN + info->iconHug;
            }
            else {
                textArea->obj.alignmentMarginY = 16;
            }
        }
        else {
            textArea->obj.alignment = TOP_MIDDLE;
        }

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    // add description if present
    if (info->description != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = BLACK;
        textArea->text          = PIC(info->description);
        textArea->textAlignment = CENTER;
        textArea->fontId        = SMALL_REGULAR_FONT;
        textArea->wrapping      = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);

        // if not the first child, put on bottom of the previous, with a margin
        if (container->nbChildren > 0) {
            textArea->obj.alignment = BOTTOM_MIDDLE;
            textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
            if (container->children[container->nbChildren - 1]->type == TEXT_AREA) {
                // if previous element is text, only space of 16 px
                textArea->obj.alignmentMarginY = 16;
            }
            else {
                textArea->obj.alignmentMarginY = BOTTOM_BORDER_MARGIN + info->iconHug;
            }
        }
        else {
            textArea->obj.alignment = TOP_MIDDLE;
        }

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    // add sub-text if present
    if (info->subText != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = DARK_GRAY;
        textArea->text          = PIC(info->subText);
        textArea->textAlignment = CENTER;
        textArea->fontId        = SMALL_REGULAR_FONT;
        textArea->wrapping      = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
        // sub-text is included in a hug of 8px
        textArea->obj.area.height += 2 * 8;
        // if not the first child, put on bottom of the previous, with a margin
        if (container->nbChildren > 0) {
            textArea->obj.alignment = BOTTOM_MIDDLE;
            textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
            textArea->obj.alignmentMarginY = 16;
            if (container->children[container->nbChildren - 1]->type == IMAGE) {
                textArea->obj.alignmentMarginY += info->iconHug;
            }
        }
        else {
            textArea->obj.alignment = TOP_MIDDLE;
        }

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    container->layout          = VERTICAL;
    container->obj.alignment   = CENTER;
    container->obj.area.width  = AVAILABLE_WIDTH;
    container->obj.area.height = fullHeight;
    if (info->padding) {
        container->obj.area.height += 40;
    }

    // set this new container as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    return container;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief returns a layout of the given type. The layout is reset
 *
 * @param description description of layout
 * @return a pointer to the corresponding layout
 */
nbgl_layout_t *nbgl_layoutGet(const nbgl_layoutDescription_t *description)
{
    nbgl_layoutInternal_t *layout = NULL;

    // find an empty layout in the proper "layer"
    if (description->modal) {
        if (gLayout[1].nbChildren == 0) {
            layout = &gLayout[1];
        }
        else if (gLayout[2].nbChildren == 0) {
            layout = &gLayout[2];
        }
    }
    else {
        // automatically "release" a potentially opened non-modal layout
        gLayout[0].nbChildren = 0;
        layout                = &gLayout[0];
    }
    if (layout == NULL) {
        LOG_WARN(LAYOUT_LOGGER, "nbgl_layoutGet(): impossible to get a layout!\n");
        return NULL;
    }

    // reset globals
    memset(layout, 0, sizeof(nbgl_layoutInternal_t));

    nbTouchableControls = 0;

    layout->callback       = (nbgl_layoutTouchCallback_t) PIC(description->onActionCallback);
    layout->modal          = description->modal;
    layout->withLeftBorder = description->withLeftBorder;
    if (description->modal) {
        layout->layer = nbgl_screenPush(&layout->children,
                                        NB_MAX_SCREEN_CHILDREN,
                                        &description->ticker,
                                        (nbgl_touchCallback_t) touchCallback);
    }
    else {
        nbgl_screenSet(&layout->children,
                       NB_MAX_SCREEN_CHILDREN,
                       &description->ticker,
                       (nbgl_touchCallback_t) touchCallback);
        layout->layer = 0;
    }
    layout->container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layout->layer);
    layout->container->obj.area.width  = SCREEN_WIDTH;
    layout->container->obj.area.height = SCREEN_HEIGHT;
    layout->container->layout          = VERTICAL;
    layout->container->children = nbgl_containerPoolGet(NB_MAX_CONTAINER_CHILDREN, layout->layer);
    // by default, if no header, main container is aligned on top-left
    layout->container->obj.alignment = TOP_LEFT;
    // main container is always the second object, leaving space for header
    layout->children[MAIN_CONTAINER_INDEX] = (nbgl_obj_t *) layout->container;
    layout->nbChildren                     = NB_MAX_SCREEN_CHILDREN;

    // if a tap text is defined, make the container tapable and display this text in gray
    if (description->tapActionText != NULL) {
        layoutObj_t *obj;

        obj = &layout->callbackObjPool[layout->nbUsedCallbackObjs];
        layout->nbUsedCallbackObjs++;
        obj->obj                         = (nbgl_obj_t *) layout->container;
        obj->token                       = description->tapActionToken;
        obj->tuneId                      = description->tapTuneId;
        layout->container->obj.touchMask = (1 << TOUCHED);
        layout->container->obj.touchId   = WHOLE_SCREEN_ID;

        nbgl_layoutFooter_t footerDesc;
        footerDesc.type                = FOOTER_SIMPLE_TEXT;
        footerDesc.separationLine      = false;
        footerDesc.simpleText.text     = PIC(description->tapActionText);
        footerDesc.simpleText.mutedOut = true;
        footerDesc.simpleText.token    = description->tapActionToken;
        footerDesc.simpleText.tuneId   = description->tapTuneId;
        nbgl_layoutAddExtendedFooter((nbgl_layout_t *) layout, &footerDesc);
    }

    return (nbgl_layout_t *) layout;
}

/**
 * @brief Creates a swipe interaction on the main container
 *
 * @param layout the current layout
 * @param swipesMask the type of swipes to be handled by the container
 * @param text the text in gray to display at bottom of the main container (can be NULL)
 * @param token the token that will be used as argument of the callback
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when button is pressed
 * @return >= 0 if OK
 */

int nbgl_layoutAddSwipe(nbgl_layout_t *layout,
                        uint16_t       swipesMask,
                        const char    *text,
                        uint8_t        token,
                        tune_index_e   tuneId)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSwipe():\n");
    if (layout == NULL) {
        return -1;
    }

    if (text) {
        // create 'tap to continue' text area
        layoutInt->tapText                  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, 0);
        layoutInt->tapText->localized       = false;
        layoutInt->tapText->text            = PIC(text);
        layoutInt->tapText->textColor       = DARK_GRAY;
        layoutInt->tapText->fontId          = SMALL_REGULAR_FONT;
        layoutInt->tapText->obj.area.width  = AVAILABLE_WIDTH;
        layoutInt->tapText->obj.area.height = nbgl_getFontLineHeight(layoutInt->tapText->fontId);
        layoutInt->tapText->textAlignment   = CENTER;
#ifdef TARGET_STAX
        layoutInt->tapText->obj.alignmentMarginY = BORDER_MARGIN;
#else   // TARGET_STAX
        layoutInt->tapText->obj.alignmentMarginY = 30;
#endif  // TARGET_STAX
        layoutInt->tapText->obj.alignment = BOTTOM_MIDDLE;
    }
    return addSwipeInternal(layoutInt, swipesMask, SWIPE_USAGE_CUSTOM, token, tuneId);
}

/**
 * @brief Creates a Top-right button in the top right corner of the top panel
 *
 * @param layout the current layout
 * @param icon icon configuration
 * @param token the token that will be used as argument of the callback
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when button is pressed
 * @return >= 0 if OK
 */
int nbgl_layoutAddTopRightButton(nbgl_layout_t             *layout,
                                 const nbgl_icon_details_t *icon,
                                 uint8_t                    token,
                                 tune_index_e               tuneId)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    layoutObj_t           *obj;
    nbgl_button_t         *button;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddTopRightButton():\n");
    if (layout == NULL) {
        return -1;
    }
    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
    obj    = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) button, token, tuneId);
    if (obj == NULL) {
        return -1;
    }

    button->obj.area.width       = BUTTON_DIAMETER;
    button->obj.area.height      = BUTTON_DIAMETER;
    button->radius               = BUTTON_RADIUS;
    button->obj.alignmentMarginX = BORDER_MARGIN;
    button->obj.alignmentMarginY = BORDER_MARGIN;
    button->foregroundColor      = BLACK;
    button->innerColor           = WHITE;
    button->borderColor          = LIGHT_GRAY;
    button->obj.touchMask        = (1 << TOUCHED);
    button->obj.touchId          = TOP_RIGHT_BUTTON_ID;
    button->icon                 = PIC(icon);
    button->obj.alignment        = TOP_RIGHT;

    // add to screen
    layoutInt->children[TOP_RIGHT_BUTTON_INDEX] = (nbgl_obj_t *) button;

    return 0;
}

/**
 * @brief Creates a navigation bar on bottom of main container
 *
 * @param layout the current layout
 * @param info structure giving the description of the navigation bar
 * @return the height of the control if OK
 */
int nbgl_layoutAddNavigationBar(nbgl_layout_t *layout, const nbgl_layoutNavigationBar_t *info)
{
    nbgl_layoutFooter_t footerDesc;
    footerDesc.type                         = FOOTER_NAV;
    footerDesc.separationLine               = info->withSeparationLine;
    footerDesc.navigation.activePage        = info->activePage;
    footerDesc.navigation.nbPages           = info->nbPages;
    footerDesc.navigation.withExitKey       = info->withExitKey;
    footerDesc.navigation.withBackKey       = info->withBackKey;
    footerDesc.navigation.withPageIndicator = false;
    footerDesc.navigation.token             = info->token;
    footerDesc.navigation.tuneId            = info->tuneId;
    return nbgl_layoutAddExtendedFooter(layout, &footerDesc);
}

/**
 * @brief Creates a centered button at bottom of main container
 * @brief incompatible with navigation bar
 *
 * @param layout the current layout
 * @param icon icon inside the round button
 * @param token used as parameter of userCallback when button is touched
 * @param separationLine if set to true, adds a light gray separation line on top of the container
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when button is pressed
 * @return >= 0 if OK
 */
int nbgl_layoutAddBottomButton(nbgl_layout_t             *layout,
                               const nbgl_icon_details_t *icon,
                               uint8_t                    token,
                               bool                       separationLine,
                               tune_index_e               tuneId)
{
    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddBottomButton():\n");
    nbgl_layoutFooter_t footerDesc;
    footerDesc.type                  = FOOTER_SIMPLE_BUTTON;
    footerDesc.separationLine        = separationLine;
    footerDesc.button.fittingContent = false;
    footerDesc.button.icon           = PIC(icon);
    footerDesc.button.text           = NULL;
    footerDesc.button.token          = token;
    footerDesc.button.tuneId         = tuneId;
    footerDesc.button.style          = WHITE_BACKGROUND;
    return nbgl_layoutAddExtendedFooter(layout, &footerDesc);
}

/**
 * @brief Creates a touchable bar in main panel
 *
 * @param layout the current layout
 * @param barLayout the properties of the bar
 * @return the height of the bar if OK
 */
int nbgl_layoutAddTouchableBar(nbgl_layout_t *layout, const nbgl_layoutBar_t *barLayout)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    listItem_t             itemDesc;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddTouchableBar():\n");
    if (layout == NULL) {
        return -1;
    }
    // main text is mandatory
    if (barLayout->text == NULL) {
        LOG_FATAL(LAYOUT_LOGGER, "nbgl_layoutAddTouchableBar(): main text is mandatory\n");
    }

    itemDesc.iconLeft  = barLayout->iconLeft;
    itemDesc.iconRight = barLayout->iconRight;
    itemDesc.text      = barLayout->text;
    itemDesc.subText   = barLayout->subText;
    itemDesc.token     = barLayout->token;
    itemDesc.tuneId    = barLayout->tuneId;
    itemDesc.state     = (barLayout->inactive) ? OFF_STATE : ON_STATE;
    itemDesc.large     = barLayout->large;
    itemDesc.type      = TOUCHABLE_BAR_ITEM;
    container          = addListItem(layoutInt, &itemDesc);

    if (container == NULL) {
        return -1;
    }
    return container->obj.area.height;
}

/**
 * @brief Creates a switch with the given text and its state
 *
 * @param layout the current layout
 * @param switchLayout description of the parameters of the switch
 * @return height of the control if OK
 */
int nbgl_layoutAddSwitch(nbgl_layout_t *layout, const nbgl_layoutSwitch_t *switchLayout)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    listItem_t             itemDesc;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSwitch():\n");
    if (layout == NULL) {
        return -1;
    }
    // main text is mandatory
    if (switchLayout->text == NULL) {
        LOG_FATAL(LAYOUT_LOGGER, "nbgl_layoutAddSwitch(): main text is mandatory\n");
    }

    itemDesc.iconLeft  = NULL;
    itemDesc.iconRight = NULL;
    itemDesc.text      = switchLayout->text;
    itemDesc.subText   = switchLayout->subText;
    itemDesc.token     = switchLayout->token;
    itemDesc.tuneId    = switchLayout->tuneId;
    itemDesc.state     = switchLayout->initState;
    itemDesc.large     = false;
    itemDesc.type      = SWITCH_ITEM;
    container          = addListItem(layoutInt, &itemDesc);

    if (container == NULL) {
        return -1;
    }
    return container->obj.area.height;
}

/**
 * @brief Creates an area with given text and sub text (in gray)
 *
 * @param layout the current layout
 * @param text main text (in small bold font), optional
 * @param subText description under main text (in small regular font), optional
 * @return height of the control if OK
 */
int nbgl_layoutAddText(nbgl_layout_t *layout, const char *text, const char *subText)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_text_area_t      *textArea;
    nbgl_text_area_t      *subTextArea;
    uint16_t               fullHeight = 0;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddText():\n");
    if (layout == NULL) {
        return -1;
    }
    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);

    // get container children
    container->children       = nbgl_containerPoolGet(2, layoutInt->layer);
    container->obj.area.width = AVAILABLE_WIDTH;

    if (text != NULL) {
        textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);

        textArea->textColor     = BLACK;
        textArea->text          = PIC(text);
        textArea->textAlignment = MID_LEFT;
        textArea->fontId        = SMALL_BOLD_FONT;
        textArea->style         = NO_STYLE;
        textArea->wrapping      = true;
        textArea->obj.alignment = NO_ALIGNMENT;
#ifdef TARGET_STAX
        textArea->obj.alignmentMarginY = 32;
#else   // TARGET_STAX
        textArea->obj.alignmentMarginY           = 28;
#endif  // TARGET_STAX
        textArea->obj.area.width  = container->obj.area.width;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;
        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    if (subText != NULL) {
        subTextArea            = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        subTextArea->textColor = BLACK;
        subTextArea->text      = PIC(subText);
        subTextArea->fontId    = SMALL_REGULAR_FONT;
        subTextArea->style     = NO_STYLE;
        subTextArea->wrapping  = true;
        subTextArea->obj.area.width  = container->obj.area.width;
        subTextArea->obj.area.height = nbgl_getTextHeightInWidth(subTextArea->fontId,
                                                                 subTextArea->text,
                                                                 subTextArea->obj.area.width,
                                                                 subTextArea->wrapping);
        subTextArea->textAlignment   = MID_LEFT;
        subTextArea->obj.alignment   = NO_ALIGNMENT;
        if (text != NULL) {
#ifdef TARGET_STAX
            subTextArea->obj.alignmentMarginY = 16;
            fullHeight += 28;  // under the subText
#else                          // TARGET_STAX
            subTextArea->obj.alignmentMarginY = 14;
            fullHeight += 26;  // under the subText
#endif                         // TARGET_STAX
        }
        else {
#ifdef TARGET_STAX
            subTextArea->obj.alignmentMarginY = BORDER_MARGIN;
            fullHeight += BORDER_MARGIN;
#else   // TARGET_STAX
            subTextArea->obj.alignmentMarginY = 26;
            fullHeight += 26;  // under the subText
#endif  // TARGET_STAX
        }
        container->children[container->nbChildren] = (nbgl_obj_t *) subTextArea;
        container->nbChildren++;
        fullHeight += subTextArea->obj.area.height + subTextArea->obj.alignmentMarginY;
    }
    else {
#ifdef TARGET_STAX
        fullHeight += 32;
#else   // TARGET_STAX
        fullHeight += 28;
#endif  // TARGET_STAX
    }
    container->obj.area.height      = fullHeight;
    container->layout               = VERTICAL;
    container->obj.alignmentMarginX = BORDER_MARGIN;
    container->obj.alignment        = NO_ALIGNMENT;
    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    return container->obj.area.height;
}

/**
 * @brief Creates an area with given text in small regular font, under the header
 *
 * @param layout the current layout
 * @param text main text in small regular font
 * @return height of the control if OK
 */
int nbgl_layoutAddSubHeaderText(nbgl_layout_t *layout, const char *text)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSubHeaderText():\n");
    if (layout == NULL) {
        return -1;
    }
    textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);

    textArea->textColor            = BLACK;
    textArea->text                 = PIC(text);
    textArea->textAlignment        = MID_LEFT;
    textArea->fontId               = SMALL_REGULAR_FONT;
    textArea->style                = NO_STYLE;
    textArea->wrapping             = true;
    textArea->obj.alignment        = NO_ALIGNMENT;
    textArea->obj.alignmentMarginX = BORDER_MARGIN;
    textArea->obj.area.width       = AVAILABLE_WIDTH;
    textArea->obj.area.height      = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
#ifdef TARGET_STAX
    textArea->obj.area.height += 2 * 24;
#else   // TARGET_STAX
    textArea->obj.area.height += 2 * 28;
#endif  // TARGET_STAX

    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    return textArea->obj.area.height;
}

/**
 * @brief Creates an area with given text in 32px font (in Black)
 *
 * @param layout the current layout
 * @param text text to be displayed (auto-wrap)
 * @return >= 0 if OK
 */
int nbgl_layoutAddLargeCaseText(nbgl_layout_t *layout, const char *text)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddLargeCaseText():\n");
    if (layout == NULL) {
        return -1;
    }
    textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);

    textArea->textColor       = BLACK;
    textArea->text            = PIC(text);
    textArea->textAlignment   = MID_LEFT;
    textArea->fontId          = LARGE_MEDIUM_FONT;
    textArea->obj.area.width  = AVAILABLE_WIDTH;
    textArea->wrapping        = true;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    textArea->style                = NO_STYLE;
    textArea->obj.alignment        = NO_ALIGNMENT;
    textArea->obj.alignmentMarginX = BORDER_MARGIN;
#ifdef TARGET_STAX
    // if first object of container, increase the margin from top
    if (layoutInt->container->nbChildren == 0) {
        textArea->obj.alignmentMarginY += BORDER_MARGIN;
    }
#endif  // TARGET_STAX

    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    return 0;
}

/**
 * @brief Creates in the main container three text areas:
 * - a first one in black large case, with title param
 * - a second one under it, in black small case, with description param
 * - a last one at the bottom of the container, in gray, with info param
 *
 * @param layout the current layout
 * @param title main text (in large bold font)
 * @param description description under main text (in small regular font)
 * @param info description at bottom (in small gray)
 * @return height of the control if OK
 */
int nbgl_layoutAddTextContent(nbgl_layout_t *layout,
                              const char    *title,
                              const char    *description,
                              const char    *info)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddTextContent():\n");
    if (layout == NULL) {
        return -1;
    }

    // create title
    textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor     = BLACK;
    textArea->text          = PIC(title);
    textArea->textAlignment = MID_LEFT;
    textArea->fontId        = LARGE_MEDIUM_FONT;
    textArea->style         = NO_STYLE;
    textArea->wrapping      = true;
    textArea->obj.alignment = NO_ALIGNMENT;
    textArea->obj.alignmentMarginX = BORDER_MARGIN;
#ifdef TARGET_STAX
    textArea->obj.alignmentMarginY = 24;
#else   // TARGET_STAX
    textArea->obj.alignmentMarginY = 16;
#endif  // TARGET_STAX
    textArea->obj.area.width  = AVAILABLE_WIDTH;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    // create description
    textArea                  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor       = BLACK;
    textArea->text            = PIC(description);
    textArea->fontId          = SMALL_REGULAR_FONT;
    textArea->style           = NO_STYLE;
    textArea->wrapping        = true;
    textArea->obj.area.width  = AVAILABLE_WIDTH;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    textArea->textAlignment        = MID_LEFT;
    textArea->obj.alignment        = NO_ALIGNMENT;
    textArea->obj.alignmentMarginX = BORDER_MARGIN;
#ifdef TARGET_STAX
    textArea->obj.alignmentMarginY = 16;
#else   // TARGET_STAX
    textArea->obj.alignmentMarginY = 24;
#endif  // TARGET_STAX
    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    // create info on the bottom
    textArea                  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor       = DARK_GRAY;
    textArea->text            = PIC(info);
    textArea->fontId          = SMALL_REGULAR_FONT;
    textArea->style           = NO_STYLE;
    textArea->wrapping        = true;
    textArea->obj.area.width  = AVAILABLE_WIDTH;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    textArea->textAlignment        = MID_LEFT;
    textArea->obj.alignment        = BOTTOM_LEFT;
    textArea->obj.alignmentMarginX = BORDER_MARGIN;
    textArea->obj.alignmentMarginY = 40;
    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    return layoutInt->container->obj.area.height;
}

/**
 * @brief Creates a list of radio buttons (on the right)
 *
 * @param layout the current layout
 * @param choices structure giving the list of choices and the current selected one
 * @return >= 0 if OK
 */
int nbgl_layoutAddRadioChoice(nbgl_layout_t *layout, const nbgl_layoutRadioChoice_t *choices)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    layoutObj_t           *obj;
    uint8_t                i;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddRadioChoice():\n");
    if (layout == NULL) {
        return -1;
    }
    for (i = 0; i < choices->nbChoices; i++) {
        nbgl_container_t *container;
        nbgl_text_area_t *textArea;
        nbgl_radio_t     *button;
        nbgl_line_t      *line;

        container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
        textArea  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        button    = (nbgl_radio_t *) nbgl_objPoolGet(RADIO_BUTTON, layoutInt->layer);

        obj = layoutAddCallbackObj(
            layoutInt, (nbgl_obj_t *) container, choices->token, choices->tuneId);
        if (obj == NULL) {
            return -1;
        }

        // get container children (max 2)
        container->nbChildren      = 2;
        container->children        = nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
        container->obj.area.width  = AVAILABLE_WIDTH;
        container->obj.area.height = RADIO_CHOICE_HEIGHT;
        container->obj.alignment   = NO_ALIGNMENT;
        container->obj.alignmentMarginX = BORDER_MARGIN;
        container->obj.alignTo          = (nbgl_obj_t *) NULL;

        // init button for this choice
        button->activeColor    = BLACK;
        button->borderColor    = LIGHT_GRAY;
        button->obj.alignTo    = (nbgl_obj_t *) container;
        button->obj.alignment  = MID_RIGHT;
        button->state          = OFF_STATE;
        container->children[1] = (nbgl_obj_t *) button;

        // init text area for this choice
        if (choices->localized == true) {
            textArea->localized = true;
#ifdef HAVE_LANGUAGE_PACK
            textArea->textId = choices->nameIds[i];
#endif  // HAVE_LANGUAGE_PACK
        }
        else {
            textArea->text = PIC(choices->names[i]);
        }
        textArea->textAlignment  = MID_LEFT;
        textArea->obj.area.width = container->obj.area.width - RADIO_WIDTH;
        textArea->style          = NO_STYLE;
        textArea->obj.alignment  = MID_LEFT;
        textArea->obj.alignTo    = (nbgl_obj_t *) container;
        container->children[0]   = (nbgl_obj_t *) textArea;

        // whole container should be touchable
        container->obj.touchMask = (1 << TOUCHED);
        container->obj.touchId   = CONTROLS_ID + nbTouchableControls;
        nbTouchableControls++;

        // highlight init choice
        if (i == choices->initChoice) {
            button->state       = ON_STATE;
            textArea->textColor = BLACK;
            textArea->fontId    = SMALL_BOLD_FONT;
        }
        else {
            button->state       = OFF_STATE;
            textArea->textColor = DARK_GRAY;
            textArea->fontId    = SMALL_REGULAR_FONT;
        }
        textArea->obj.area.height = nbgl_getFontHeight(textArea->fontId);

        line                       = createHorizontalLine(layoutInt->layer);
        line->obj.alignmentMarginY = -4;
        line->offset               = 3;

        // set these new objs as child of main container
        layoutAddObject(layoutInt, (nbgl_obj_t *) container);
        layoutAddObject(layoutInt, (nbgl_obj_t *) line);
    }

    return 0;
}

/**
 * @brief Creates an area on the center of the main panel, with a possible icon/image,
 * a possible text in black under it, and a possible text in gray under it
 *
 * @param layout the current layout
 * @param info structure giving the description of buttons (texts, icons, layout)
 * @return >= 0 if OK
 */
int nbgl_layoutAddCenteredInfo(nbgl_layout_t *layout, const nbgl_layoutCenteredInfo_t *info)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_contentCenter_t   centeredInfo = {.icon        = info->icon,
                                           .title       = NULL,
                                           .smallTitle  = NULL,
                                           .description = NULL,
                                           .subText     = NULL,
                                           .iconHug     = 0,
                                           .padding     = false};

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddCenteredInfo():\n");
    if (layout == NULL) {
        return -1;
    }

    if (info->text1 != NULL) {
        if (info->style != NORMAL_INFO) {
            centeredInfo.title = info->text1;
        }
        else {
            centeredInfo.smallTitle = info->text1;
        }
    }
    if (info->text2 != NULL) {
        if (info->style != LARGE_CASE_BOLD_INFO) {
            centeredInfo.description = info->text2;
        }
        else {
            centeredInfo.smallTitle = info->text2;
        }
    }
    if (info->text3 != NULL) {
        if (info->style == LARGE_CASE_GRAY_INFO) {
            centeredInfo.subText = info->text3;
        }
        else {
            centeredInfo.description = info->text3;
        }
    }
    container = addContentCenter(layoutInt, &centeredInfo);

    if (info->onTop) {
        container->obj.alignmentMarginX = BORDER_MARGIN;
        container->obj.alignmentMarginY = BORDER_MARGIN + info->offsetY;
        container->obj.alignment        = NO_ALIGNMENT;
    }
    else {
        container->obj.alignmentMarginY = info->offsetY;
    }

    return container->obj.area.height;
}

/**
 * @brief Creates an area on the center of the main panel, with a possible icon,
 * and possible texts under it
 *
 * @param layout the current layout
 * @param info structure giving the description of the Content Center
 * @return the size of the area if OK
 */
int nbgl_layoutAddContentCenter(nbgl_layout_t *layout, const nbgl_contentCenter_t *info)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddContentCenter():\n");
    if (layout == NULL) {
        return -1;
    }

    container = addContentCenter(layoutInt, info);

    return container->obj.area.height;
}

#ifdef NBGL_QRCODE
/**
 * @brief Creates an area on the center of the main panel, with a QRCode,
 * a possible text in black (bold) under it, and a possible text in black under it
 *
 * @param layout the current layout
 * @param info structure giving the description of buttons (texts, icons, layout)
 * @return height of the control if OK
 */
int nbgl_layoutAddQRCode(nbgl_layout_t *layout, const nbgl_layoutQRCode_t *info)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_text_area_t      *textArea = NULL;
    nbgl_qrcode_t         *qrcode;
    uint16_t               fullHeight = 0;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddQRCode():\n");
    if (layout == NULL) {
        return -1;
    }

    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);

    // get container children (max 2 (QRCode + text1 + text2))
    container->children   = nbgl_containerPoolGet(3, layoutInt->layer);
    container->nbChildren = 0;

    qrcode = (nbgl_qrcode_t *) nbgl_objPoolGet(QR_CODE, layoutInt->layer);
    // version is forced to V10 if url is longer than 62 characters
    if (strlen(PIC(info->url)) > 62) {
        qrcode->version = QRCODE_V10;
    }
    else {
        qrcode->version = QRCODE_V4;
    }
    qrcode->foregroundColor = BLACK;
    // in QR V4, we use 8*8 screen pixels for one QR pixel
    // in QR V10, we use 4*4 screen pixels for one QR pixel
    qrcode->obj.area.width
        = (qrcode->version == QRCODE_V4) ? (QR_V4_NB_PIX_SIZE * 8) : (QR_V10_NB_PIX_SIZE * 4);
    qrcode->obj.area.height = qrcode->obj.area.width;
    qrcode->text            = PIC(info->url);
    qrcode->obj.area.bpp    = NBGL_BPP_1;
    qrcode->obj.alignment   = TOP_MIDDLE;

    fullHeight += qrcode->obj.area.height;
    container->children[container->nbChildren] = (nbgl_obj_t *) qrcode;
    container->nbChildren++;

    if (info->text1 != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = BLACK;
        textArea->text          = PIC(info->text1);
        textArea->textAlignment = CENTER;
        textArea->fontId   = (info->largeText1 == true) ? LARGE_MEDIUM_FONT : SMALL_REGULAR_FONT;
        textArea->wrapping = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
        textArea->obj.alignment = BOTTOM_MIDDLE;
        textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
        textArea->obj.alignmentMarginY = 24;

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    if (info->text2 != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = DARK_GRAY;
        textArea->text          = PIC(info->text2);
        textArea->textAlignment = CENTER;
        textArea->fontId        = SMALL_REGULAR_FONT;
        textArea->wrapping      = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
        textArea->obj.alignment = BOTTOM_MIDDLE;
        textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
        if (info->text1 != NULL) {
#ifdef TARGET_STAX
            textArea->obj.alignmentMarginY = 40;
#else   // TARGET_STAX
            textArea->obj.alignmentMarginY = 28;
#endif  // TARGET_STAX
        }
        else {
            textArea->obj.alignmentMarginY = 32;
            fullHeight += 8;
        }

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    // ensure that fullHeight is fitting in main container height (with 16 px margin)
    if ((fullHeight >= (layoutInt->container->obj.area.height - 16))
        && (qrcode->version == QRCODE_V4)) {
        qrcode->version = QRCODE_V4_SMALL;
        // in QR V4 small, we use 4*4 screen pixels for one QR pixel
        qrcode->obj.area.width  = QR_V4_NB_PIX_SIZE * 4;
        qrcode->obj.area.height = qrcode->obj.area.width;
        fullHeight -= QR_V4_NB_PIX_SIZE * 4;
    }
    container->obj.area.height = fullHeight;
    container->layout          = VERTICAL;
    if (info->centered) {
        container->obj.alignment = CENTER;
    }
    else {
        container->obj.alignment = BOTTOM_MIDDLE;
        container->obj.alignTo
            = layoutInt->container->children[layoutInt->container->nbChildren - 1];
    }
    container->obj.alignmentMarginY = info->offsetY;

    container->obj.area.width = AVAILABLE_WIDTH;

    // set this new container as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    return fullHeight;
}
#endif  // NBGL_QRCODE

/**
 * @brief Creates two buttons to make a choice. Both buttons are mandatory.
 *        Both buttons are full width, one under the other
 *
 * @param layout the current layout
 * @param info structure giving the description of buttons (texts, icons, layout)
 * @return >= 0 if OK
 */
int nbgl_layoutAddChoiceButtons(nbgl_layout_t *layout, const nbgl_layoutChoiceButtons_t *info)
{
    nbgl_layoutFooter_t footerDesc;
    footerDesc.type                     = FOOTER_CHOICE_BUTTONS;
    footerDesc.separationLine           = false;
    footerDesc.choiceButtons.bottomText = info->bottomText;
    footerDesc.choiceButtons.token      = info->token;
    footerDesc.choiceButtons.topText    = info->topText;
    footerDesc.choiceButtons.style      = info->style;
    footerDesc.choiceButtons.tuneId     = info->tuneId;
    return nbgl_layoutAddExtendedFooter(layout, &footerDesc);
}

/**
 * @brief Creates two buttons to make a choice. Both buttons are mandatory
 *        The left one contains only an icon and is round, the other contains only
 *        a text
 *
 * @param layout the current layout
 * @param info structure giving the description of buttons (text, icon, tokens)
 * @return >= 0 if OK
 */
int nbgl_layoutAddHorizontalButtons(nbgl_layout_t                        *layout,
                                    const nbgl_layoutHorizontalButtons_t *info)
{
    layoutObj_t           *obj;
    nbgl_button_t         *button;
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddHorizontalButtons():\n");
    if (layout == NULL) {
        return -1;
    }

    // icon & text cannot be NULL
    if ((info->leftIcon == NULL) || (info->rightText == NULL)) {
        return -1;
    }

    // create left button (in white) at first
    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
    obj    = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) button, info->leftToken, info->tuneId);
    if (obj == NULL) {
        return -1;
    }
    // associate with with index 1
    obj->index                   = 1;
    button->obj.alignment        = BOTTOM_LEFT;
    button->obj.alignmentMarginX = BORDER_MARGIN;
#ifdef TARGET_STAX
    button->obj.alignmentMarginY = 20;  // 20 pixels from screen bottom
#else                                   // TARGET_STAX
    button->obj.alignmentMarginY   = 24;  // 24 pixels from screen bottom
#endif                                  // TARGET_STAX
    button->borderColor     = LIGHT_GRAY;
    button->innerColor      = WHITE;
    button->foregroundColor = BLACK;
    button->obj.area.width  = BUTTON_DIAMETER;
    button->obj.area.height = BUTTON_DIAMETER;
    button->radius          = BUTTON_RADIUS;
    button->icon            = PIC(info->leftIcon);
    button->fontId          = SMALL_BOLD_FONT;
    button->obj.touchMask   = (1 << TOUCHED);
    button->obj.touchId     = CHOICE_2_ID;
    // set this new button as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) button);

    // then black button, on right
    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
    obj    = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) button, info->rightToken, info->tuneId);
    if (obj == NULL) {
        return -1;
    }
    // associate with with index 0
    obj->index                   = 0;
    button->obj.alignment        = BOTTOM_RIGHT;
    button->obj.alignmentMarginX = BORDER_MARGIN;
    button->obj.alignmentMarginY = 24;  // 24 pixels from screen bottom
    button->innerColor           = BLACK;
    button->borderColor          = BLACK;
    button->foregroundColor      = WHITE;
    button->obj.area.width       = AVAILABLE_WIDTH - BUTTON_DIAMETER - 16;
    button->obj.area.height      = BUTTON_DIAMETER;
    button->radius               = BUTTON_RADIUS;
    button->text                 = PIC(info->rightText);
    button->fontId               = SMALL_BOLD_FONT;
    button->obj.touchMask        = (1 << TOUCHED);
    button->obj.touchId          = CHOICE_1_ID;
    // set this new button as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) button);

    return 0;
}

/**
 * @brief Creates a list of [tag,value] pairs
 *
 * @param layout the current layout
 * @param list structure giving the list of [tag,value] pairs
 * @return >= 0 if OK
 */
int nbgl_layoutAddTagValueList(nbgl_layout_t *layout, const nbgl_layoutTagValueList_t *list)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *itemTextArea;
    nbgl_text_area_t      *valueTextArea;
    nbgl_container_t      *container;
    uint8_t                i;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddTagValueList():\n");
    if (layout == NULL) {
        return -1;
    }

    for (i = 0; i < list->nbPairs; i++) {
        const nbgl_layoutTagValue_t *pair;
        uint16_t                     fullHeight = 0;
        const nbgl_icon_details_t   *valueIcon  = NULL;

        if (list->pairs != NULL) {
            pair = &list->pairs[i];
        }
        else {
            pair = list->callback(list->startIndex + i);
        }

        container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);

        // get container children (max 3 if a valueIcon, otherwise 2)
        container->children
            = nbgl_containerPoolGet((pair->valueIcon != NULL) ? 3 : 2, layoutInt->layer);

        itemTextArea  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        valueTextArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);

        // init text area for this choice
        itemTextArea->textColor       = DARK_GRAY;
        itemTextArea->text            = PIC(pair->item);
        itemTextArea->textAlignment   = MID_LEFT;
        itemTextArea->fontId          = SMALL_REGULAR_FONT;
        itemTextArea->wrapping        = true;
        itemTextArea->obj.area.width  = AVAILABLE_WIDTH;
        itemTextArea->obj.area.height = nbgl_getTextHeightInWidth(
            itemTextArea->fontId, itemTextArea->text, AVAILABLE_WIDTH, itemTextArea->wrapping);
        itemTextArea->style                        = NO_STYLE;
        itemTextArea->obj.alignment                = NO_ALIGNMENT;
        itemTextArea->obj.alignmentMarginX         = 0;
        itemTextArea->obj.alignmentMarginY         = 0;
        itemTextArea->obj.alignTo                  = NULL;
        container->children[container->nbChildren] = (nbgl_obj_t *) itemTextArea;
        container->nbChildren++;

        fullHeight += itemTextArea->obj.area.height;

        // init button for this choice
        valueTextArea->textColor     = BLACK;
        valueTextArea->text          = PIC(pair->value);
        valueTextArea->textAlignment = MID_LEFT;
        if (list->smallCaseForValue) {
            valueTextArea->fontId = SMALL_BOLD_FONT;
        }
        else {
            valueTextArea->fontId = LARGE_MEDIUM_FONT;
        }
        if ((pair->aliasValue == 0) && (pair->valueIcon == NULL)) {
            valueTextArea->obj.area.width = AVAILABLE_WIDTH;
        }
        else {
            if (pair->aliasValue) {
                // if the value is an alias, we automatically display a (>) icon
                valueIcon = &MINI_PUSH_ICON;
            }
            else {
                // otherwise use the provided icon
                valueIcon = PIC(pair->valueIcon);
            }
            // decrease the available width for value text
            valueTextArea->obj.area.width = AVAILABLE_WIDTH - valueIcon->width - 12;
        }

        // handle the nbMaxLinesForValue parameter, used to automatically keep only
        // nbMaxLinesForValue lines
        uint16_t nbLines = nbgl_getTextNbLinesInWidth(valueTextArea->fontId,
                                                      valueTextArea->text,
                                                      valueTextArea->obj.area.width,
                                                      list->wrapping);
        // use this nbMaxLinesForValue parameter only if >0
        if ((list->nbMaxLinesForValue > 0) && (nbLines > list->nbMaxLinesForValue)) {
            nbLines                   = list->nbMaxLinesForValue;
            valueTextArea->nbMaxLines = list->nbMaxLinesForValue;
        }
        const nbgl_font_t *font                    = nbgl_getFont(valueTextArea->fontId);
        valueTextArea->obj.area.height             = nbLines * font->line_height;
        valueTextArea->style                       = NO_STYLE;
        valueTextArea->obj.alignment               = BOTTOM_LEFT;
        valueTextArea->obj.alignmentMarginY        = 4;
        valueTextArea->obj.alignTo                 = (nbgl_obj_t *) itemTextArea;
        valueTextArea->wrapping                    = list->wrapping;
        container->children[container->nbChildren] = (nbgl_obj_t *) valueTextArea;
        container->nbChildren++;

        fullHeight += valueTextArea->obj.area.height + valueTextArea->obj.alignmentMarginY;
        if (valueIcon != NULL) {
            nbgl_image_t *image = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
            layoutObj_t  *obj   = layoutAddCallbackObj(
                layoutInt, (nbgl_obj_t *) image, list->token, TUNE_TAP_CASUAL);
            obj->index                  = i;
            image->foregroundColor      = BLACK;
            image->buffer               = valueIcon;
            image->obj.alignment        = RIGHT_TOP;
            image->obj.alignmentMarginX = 12;
            image->obj.alignTo          = (nbgl_obj_t *) valueTextArea;
            image->obj.touchMask        = (1 << TOUCHED);
            image->obj.touchId          = VALUE_BUTTON_1_ID + i;

            container->children[container->nbChildren] = (nbgl_obj_t *) image;
            container->nbChildren++;
        }

        container->obj.area.width       = AVAILABLE_WIDTH;
        container->obj.area.height      = fullHeight;
        container->layout               = VERTICAL;
        container->obj.alignmentMarginX = BORDER_MARGIN;
#ifdef TARGET_STAX
        // On Stax, 12 px between each tag/value pair
        if (i > 0) {
            container->obj.alignmentMarginY = 12;
        }
        else {
            container->obj.alignmentMarginY = 24;
        }
#else   // TARGET_STAX
        // On Flex, 24 px between each tag/value pair
        if (i > 0) {
            container->obj.alignmentMarginY = 24;
        }
#endif  // TARGET_STAX
        container->obj.alignment = NO_ALIGNMENT;

        layoutAddObject(layoutInt, (nbgl_obj_t *) container);
    }

    return 0;
}

/**
 * @brief Creates an area in main panel to display a progress bar, with a title text and a
 * description under the progress
 *
 * @param layout the current layout
 * @param barLayout structure giving the description of progress bar
 * @return >= 0 if OK
 */
int nbgl_layoutAddProgressBar(nbgl_layout_t *layout, const nbgl_layoutProgressBar_t *barLayout)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_progress_bar_t   *progress;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddProgressBar():\n");
    if (layout == NULL) {
        return -1;
    }
    if (barLayout->text != NULL) {
        nbgl_text_area_t *textArea;

        textArea                  = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA,
                                                        ((nbgl_layoutInternal_t *) layout)->layer);
        textArea->textColor       = BLACK;
        textArea->text            = PIC(barLayout->text);
        textArea->textAlignment   = MID_LEFT;
        textArea->fontId          = SMALL_REGULAR_FONT;
        textArea->wrapping        = true;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = nbgl_getTextHeightInWidth(
            textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
        textArea->style                = NO_STYLE;
        textArea->obj.alignment        = NO_ALIGNMENT;
        textArea->obj.alignmentMarginX = BORDER_MARGIN;
        textArea->obj.alignmentMarginY = BORDER_MARGIN;
        layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);
    }
    progress                       = (nbgl_progress_bar_t *) nbgl_objPoolGet(PROGRESS_BAR,
                                                       ((nbgl_layoutInternal_t *) layout)->layer);
    progress->foregroundColor      = BLACK;
    progress->withBorder           = true;
    progress->state                = barLayout->percentage;
    progress->obj.area.width       = 120;
    progress->obj.area.height      = 12;
    progress->obj.alignment        = NO_ALIGNMENT;
    progress->obj.alignmentMarginX = (AVAILABLE_WIDTH - progress->obj.area.width) / 2;
    progress->obj.alignmentMarginY = BORDER_MARGIN;
    layoutAddObject(layoutInt, (nbgl_obj_t *) progress);

    if (barLayout->subText != NULL) {
        nbgl_text_area_t *subTextArea;

        subTextArea = (nbgl_text_area_t *) nbgl_objPoolGet(
            TEXT_AREA, ((nbgl_layoutInternal_t *) layout)->layer);
        subTextArea->textColor            = LIGHT_GRAY;
        subTextArea->text                 = PIC(barLayout->subText);
        subTextArea->textAlignment        = MID_LEFT;
        subTextArea->fontId               = SMALL_REGULAR_FONT;
        subTextArea->wrapping             = true;
        subTextArea->obj.area.width       = AVAILABLE_WIDTH;
        subTextArea->obj.area.height      = nbgl_getTextHeightInWidth(subTextArea->fontId,
                                                                 subTextArea->text,
                                                                 subTextArea->obj.area.width,
                                                                 subTextArea->wrapping);
        subTextArea->style                = NO_STYLE;
        subTextArea->obj.alignment        = NO_ALIGNMENT;
        subTextArea->obj.alignmentMarginX = BORDER_MARGIN;
        subTextArea->obj.alignmentMarginY = BORDER_MARGIN;
        layoutAddObject(layoutInt, (nbgl_obj_t *) subTextArea);
    }

    return 0;
}

/**
 * @brief adds a separation line on bottom of the last added item
 *
 * @param layout the current layout
 * @return >= 0 if OK
 */
int nbgl_layoutAddSeparationLine(nbgl_layout_t *layout)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_line_t           *line;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSeparationLine():\n");
    line                       = createHorizontalLine(layoutInt->layer);
    line->obj.alignmentMarginY = -4;
    line->offset               = 3;
    layoutAddObject(layoutInt, (nbgl_obj_t *) line);
    return 0;
}

/**
 * @brief Creates a rounded button in the main container.
 *
 * @param layout the current layout
 * @param buttonInfo structure giving the description of button (text, icon, layout)
 * @return >= 0 if OK
 */
int nbgl_layoutAddButton(nbgl_layout_t *layout, const nbgl_layoutButton_t *buttonInfo)
{
    layoutObj_t           *obj;
    nbgl_button_t         *button;
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddButton():\n");
    if (layout == NULL) {
        return -1;
    }

    // Add in footer if matching
    if ((buttonInfo->onBottom) && (!buttonInfo->fittingContent)
        && (layoutInt->footerContainer == NULL)) {
        nbgl_layoutFooter_t footerDesc;
        footerDesc.type           = FOOTER_SIMPLE_BUTTON;
        footerDesc.separationLine = false;
        footerDesc.button.text    = buttonInfo->text;
        footerDesc.button.token   = buttonInfo->token;
        footerDesc.button.tuneId  = buttonInfo->tuneId;
        footerDesc.button.icon    = buttonInfo->icon;
        footerDesc.button.style   = buttonInfo->style;
        return nbgl_layoutAddExtendedFooter(layout, &footerDesc);
    }

    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
    obj    = layoutAddCallbackObj(
        layoutInt, (nbgl_obj_t *) button, buttonInfo->token, buttonInfo->tuneId);
    if (obj == NULL) {
        return -1;
    }

    if (buttonInfo->onBottom != true) {
        button->obj.alignmentMarginX = BORDER_MARGIN;
        button->obj.alignmentMarginY = 12;
        button->obj.alignment        = NO_ALIGNMENT;
    }
    else {
        button->obj.alignment = BOTTOM_MIDDLE;
#ifdef TARGET_STAX
        button->obj.alignmentMarginY = 20;
#else   // TARGET_STAX
        button->obj.alignmentMarginY = 24;
#endif  // TARGET_STAX
    }
    if (buttonInfo->style == BLACK_BACKGROUND) {
        button->innerColor      = BLACK;
        button->foregroundColor = WHITE;
    }
    else {
        button->innerColor      = WHITE;
        button->foregroundColor = BLACK;
    }
    if (buttonInfo->style == NO_BORDER) {
        button->borderColor = WHITE;
    }
    else {
        if (buttonInfo->style == BLACK_BACKGROUND) {
            button->borderColor = BLACK;
        }
        else {
            button->borderColor = LIGHT_GRAY;
        }
    }
    button->text   = PIC(buttonInfo->text);
    button->fontId = SMALL_BOLD_FONT;
    button->icon   = PIC(buttonInfo->icon);
    if (buttonInfo->fittingContent == true) {
        button->obj.area.width = nbgl_getTextWidth(button->fontId, button->text)
                                 + SMALL_BUTTON_HEIGHT
                                 + ((button->icon) ? (button->icon->width + 12) : 0);
        button->obj.area.height = SMALL_BUTTON_HEIGHT;
        button->radius          = RADIUS_32_PIXELS;
        if (buttonInfo->onBottom != true) {
            button->obj.alignmentMarginX
                += (SCREEN_WIDTH - 2 * BORDER_MARGIN - button->obj.area.width) / 2;
        }
    }
    else {
        button->obj.area.width  = AVAILABLE_WIDTH;
        button->obj.area.height = BUTTON_DIAMETER;
        button->radius          = BUTTON_RADIUS;
    }
    button->obj.alignTo   = NULL;
    button->obj.touchMask = (1 << TOUCHED);
    button->obj.touchId   = (buttonInfo->fittingContent) ? EXTRA_BUTTON_ID : SINGLE_BUTTON_ID;
    // set this new button as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) button);

    return 0;
}

/**
 * @brief Creates a long press button in the main container.
 *
 * @param layout the current layout
 * @param text text of the button button
 * @param token token attached to actionCallback when long time of press is elapsed
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when button is long pressed
 * @return >= 0 if OK
 */
int nbgl_layoutAddLongPressButton(nbgl_layout_t *layout,
                                  const char    *text,
                                  uint8_t        token,
                                  tune_index_e   tuneId)
{
    layoutObj_t           *obj;
    nbgl_button_t         *button;
    nbgl_text_area_t      *textArea;
    nbgl_progress_bar_t   *progressBar;
    nbgl_container_t      *container;
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_line_t           *line;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddLongPressButton():\n");
    if (layout == NULL) {
        return -1;
    }

    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    obj       = layoutAddCallbackObj(layoutInt, (nbgl_obj_t *) container, token, tuneId);
    if (obj == NULL) {
        return -1;
    }

    container->obj.area.width  = SCREEN_WIDTH;
    container->obj.area.height = LONG_PRESS_BUTTON_HEIGHT;
    container->layout          = VERTICAL;
    container->nbChildren      = 4;  // progress-bar + text + line + button
    container->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
    container->obj.alignment = BOTTOM_MIDDLE;
    container->obj.touchId   = LONG_PRESS_BUTTON_ID;
    container->obj.touchMask = ((1 << TOUCHING) | (1 << TOUCH_RELEASED) | (1 << OUT_OF_TOUCH)
                                | (1 << SWIPED_LEFT) | (1 << SWIPED_RIGHT));

    button                       = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
    button->obj.alignmentMarginX = BORDER_MARGIN;
    button->obj.alignment        = MID_RIGHT;
    button->innerColor           = BLACK;
    button->foregroundColor      = WHITE;
    button->borderColor          = BLACK;
    button->obj.area.width       = BUTTON_DIAMETER;
    button->obj.area.height      = BUTTON_DIAMETER;
    button->radius               = BUTTON_RADIUS;
    button->icon                 = PIC(&VALIDATE_ICON);
    container->children[0]       = (nbgl_obj_t *) button;

    textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor     = BLACK;
    textArea->text          = PIC(text);
    textArea->textAlignment = MID_LEFT;
    textArea->fontId        = LARGE_MEDIUM_FONT;
    textArea->wrapping      = true;
    textArea->obj.area.width
        = container->obj.area.width - 3 * BORDER_MARGIN - button->obj.area.width;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    textArea->style                = NO_STYLE;
    textArea->obj.alignment        = MID_LEFT;
    textArea->obj.alignmentMarginX = BORDER_MARGIN;
    container->children[1]         = (nbgl_obj_t *) textArea;

    line                   = createHorizontalLine(layoutInt->layer);
    line->offset           = 3;
    line->obj.alignment    = TOP_MIDDLE;
    container->children[2] = (nbgl_obj_t *) line;

    progressBar = (nbgl_progress_bar_t *) nbgl_objPoolGet(PROGRESS_BAR, layoutInt->layer);
    progressBar->withBorder           = false;
    progressBar->obj.area.width       = container->obj.area.width;
    progressBar->obj.area.height      = 8;
    progressBar->obj.alignment        = TOP_MIDDLE;
    progressBar->obj.alignmentMarginY = 4;
    progressBar->obj.alignTo          = NULL;
    container->children[3]            = (nbgl_obj_t *) progressBar;

    // set this new container as child of the main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    return container->obj.area.height;
}

/**
 * @brief Creates a touchable text at the footer of the screen, separated with a thin line from the
 * rest of the screen.
 *
 * @param layout the current layout
 * @param text text to used in the footer
 * @param token token to use when the footer is touched
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when button is long pressed
 * @return height of the control if OK
 */
int nbgl_layoutAddFooter(nbgl_layout_t *layout,
                         const char    *text,
                         uint8_t        token,
                         tune_index_e   tuneId)
{
    nbgl_layoutFooter_t footerDesc;
    footerDesc.type                = FOOTER_SIMPLE_TEXT;
    footerDesc.separationLine      = true;
    footerDesc.simpleText.text     = text;
    footerDesc.simpleText.mutedOut = false;
    footerDesc.simpleText.token    = token;
    footerDesc.simpleText.tuneId   = tuneId;
    return nbgl_layoutAddExtendedFooter(layout, &footerDesc);
}

/**
 * @brief Creates 2 touchable texts at the footer of the screen, separated with a thin line from the
 * rest of the screen, and from each other.
 *
 * @param layout the current layout
 * @param leftText text to used in the left part of footer
 * @param leftToken token to use when the left part of footer is touched
 * @param rightText text to used in the right part of footer
 * @param rightToken token to use when the right part of footer is touched
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when button is long pressed
 * @return height of the control if OK
 */
int nbgl_layoutAddSplitFooter(nbgl_layout_t *layout,
                              const char    *leftText,
                              uint8_t        leftToken,
                              const char    *rightText,
                              uint8_t        rightToken,
                              tune_index_e   tuneId)
{
    nbgl_layoutFooter_t footerDesc;
    footerDesc.type                  = FOOTER_DOUBLE_TEXT;
    footerDesc.separationLine        = true;
    footerDesc.doubleText.leftText   = leftText;
    footerDesc.doubleText.leftToken  = leftToken;
    footerDesc.doubleText.rightText  = rightText;
    footerDesc.doubleText.rightToken = rightToken;
    footerDesc.doubleText.tuneId     = tuneId;
    return nbgl_layoutAddExtendedFooter(layout, &footerDesc);
}

/**
 * @brief Creates a touchable (or not) area at the header of the screen, containing various
 * controls, described in the given structure. This header is not part of the main container
 *
 * @param layout the current layout
 * @param headerDesc description of the header to add
 * @return height of the control if OK
 */
int nbgl_layoutAddHeader(nbgl_layout_t *layout, const nbgl_layoutHeader_t *headerDesc)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    layoutObj_t           *obj;
    nbgl_text_area_t      *textArea;
    nbgl_line_t           *line, *separationLine = NULL;
    ;
    nbgl_button_t *button;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddHeader():\n");
    if (layout == NULL) {
        return -1;
    }
    if ((headerDesc == NULL) || (headerDesc->type >= NB_HEADER_TYPES)) {
        return -2;
    }

    layoutInt->headerContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    layoutInt->headerContainer->obj.area.width = SCREEN_WIDTH;
    layoutInt->headerContainer->layout         = VERTICAL;
    layoutInt->headerContainer->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(5, layoutInt->layer);
    layoutInt->headerContainer->obj.alignment = TOP_MIDDLE;

    switch (headerDesc->type) {
        case HEADER_EMPTY: {
            layoutInt->headerContainer->obj.area.height = headerDesc->emptySpace.height;
            break;
        }
        case HEADER_BACK_AND_TEXT:
        case HEADER_EXTENDED_BACK: {
            const char *text = (headerDesc->type == HEADER_EXTENDED_BACK)
                                   ? PIC(headerDesc->extendedBack.text)
                                   : PIC(headerDesc->backAndText.text);
            // add back button
            button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
            obj    = layoutAddCallbackObj(
                layoutInt,
                (nbgl_obj_t *) button,
                (headerDesc->type == HEADER_EXTENDED_BACK) ? headerDesc->extendedBack.backToken
                                                              : headerDesc->backAndText.token,
                (headerDesc->type == HEADER_EXTENDED_BACK) ? headerDesc->extendedBack.tuneId
                                                              : headerDesc->backAndText.tuneId);
            if (obj == NULL) {
                return -1;
            }

            button->obj.alignment   = MID_LEFT;
            button->innerColor      = WHITE;
            button->foregroundColor = BLACK;
            button->borderColor     = WHITE;
            button->obj.area.width  = BACK_KEY_WIDTH;
            button->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
            button->text            = NULL;
            button->icon            = PIC(&LEFT_ARROW_ICON);
            button->obj.touchMask   = (1 << TOUCHED);
            button->obj.touchId     = BACK_BUTTON_ID;
            layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                = (nbgl_obj_t *) button;
            layoutInt->headerContainer->nbChildren++;

            // add optional text if needed
            if (text != NULL) {
                textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
                if ((headerDesc->type == HEADER_EXTENDED_BACK)
                    && (headerDesc->extendedBack.textToken != NBGL_INVALID_TOKEN)) {
                    obj = layoutAddCallbackObj(layoutInt,
                                               (nbgl_obj_t *) textArea,
                                               headerDesc->extendedBack.textToken,
                                               headerDesc->extendedBack.tuneId);
                    if (obj == NULL) {
                        return -1;
                    }
                    textArea->obj.touchMask = (1 << TOUCHED);
                }
                textArea->obj.alignment = CENTER;
                textArea->textColor     = BLACK;
                textArea->obj.area.width
                    = layoutInt->headerContainer->obj.area.width - 2 * BACK_KEY_WIDTH;
                textArea->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
                textArea->text            = text;
                textArea->fontId          = SMALL_BOLD_FONT;
                textArea->textAlignment   = CENTER;
                textArea->wrapping        = true;
                // ensure that text fits on 2 lines maximum
                if (nbgl_getTextNbLinesInWidth(textArea->fontId,
                                               textArea->text,
                                               textArea->obj.area.width,
                                               textArea->wrapping)
                    > 2) {
                    LOG_WARN(LAYOUT_LOGGER,
                             "nbgl_layoutAddHeader: text [%s] is too long for header\n",
                             text);
                }
                layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                    = (nbgl_obj_t *) textArea;
                layoutInt->headerContainer->nbChildren++;
            }
            // add action key if the type is HEADER_EXTENDED_BACK
            if ((headerDesc->type == HEADER_EXTENDED_BACK)
                && (headerDesc->extendedBack.actionIcon)) {
                button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
                // if token is valid
                if (headerDesc->extendedBack.actionToken != NBGL_INVALID_TOKEN) {
                    obj = layoutAddCallbackObj(layoutInt,
                                               (nbgl_obj_t *) button,
                                               headerDesc->extendedBack.actionToken,
                                               headerDesc->extendedBack.tuneId);
                    if (obj == NULL) {
                        return -1;
                    }
                    button->obj.touchMask = (1 << TOUCHED);
                }

                button->obj.alignment = MID_RIGHT;
                button->innerColor    = WHITE;
                button->foregroundColor
                    = (headerDesc->extendedBack.actionToken != NBGL_INVALID_TOKEN) ? BLACK
                                                                                   : LIGHT_GRAY;
                button->borderColor     = WHITE;
                button->obj.area.width  = BACK_KEY_WIDTH;
                button->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
                button->text            = NULL;
                button->icon            = PIC(headerDesc->extendedBack.actionIcon);
                button->obj.touchId     = EXTRA_BUTTON_ID;
                layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                    = (nbgl_obj_t *) button;
                layoutInt->headerContainer->nbChildren++;
            }

            layoutInt->headerContainer->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
            break;
        }
        case HEADER_BACK_AND_PROGRESS: {
#ifdef TARGET_STAX
            // add optional back button
            if (headerDesc->progressAndBack.withBack) {
                button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
                obj    = layoutAddCallbackObj(layoutInt,
                                           (nbgl_obj_t *) button,
                                           headerDesc->progressAndBack.token,
                                           headerDesc->progressAndBack.tuneId);
                if (obj == NULL) {
                    return -1;
                }

                button->obj.alignment   = MID_LEFT;
                button->innerColor      = WHITE;
                button->foregroundColor = BLACK;
                button->borderColor     = WHITE;
                button->obj.area.width  = BACK_KEY_WIDTH;
                button->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
                button->text            = NULL;
                button->icon            = PIC(&LEFT_ARROW_ICON);
                button->obj.touchMask   = (1 << TOUCHED);
                button->obj.touchId     = BACK_BUTTON_ID;
                // add to container
                layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                    = (nbgl_obj_t *) button;
                layoutInt->headerContainer->nbChildren++;
            }

            // add progress indicator
            if (headerDesc->progressAndBack.nbPages > 1
                && headerDesc->progressAndBack.nbPages != NBGL_NO_PROGRESS_INDICATOR) {
                nbgl_page_indicator_t *progress;

                progress
                    = (nbgl_page_indicator_t *) nbgl_objPoolGet(PAGE_INDICATOR, layoutInt->layer);
                progress->activePage     = headerDesc->progressAndBack.activePage;
                progress->nbPages        = headerDesc->progressAndBack.nbPages;
                progress->obj.area.width = 224;
                progress->obj.alignment  = CENTER;
                // add to container
                layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                    = (nbgl_obj_t *) progress;
                layoutInt->headerContainer->nbChildren++;
            }
            layoutInt->activePage                       = headerDesc->progressAndBack.activePage;
            layoutInt->nbPages                          = headerDesc->progressAndBack.nbPages;
            layoutInt->headerContainer->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
#endif  // TARGET_STAX
            break;
        }
        case HEADER_TITLE: {
            textArea            = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            textArea->textColor = BLACK;
            textArea->obj.area.width  = AVAILABLE_WIDTH;
            textArea->obj.area.height = TOUCHABLE_HEADER_BAR_HEIGHT;
            textArea->text            = PIC(headerDesc->title.text);
            textArea->fontId          = SMALL_BOLD_FONT;
            textArea->textAlignment   = CENTER;
            textArea->wrapping        = true;
            layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                = (nbgl_obj_t *) textArea;
            layoutInt->headerContainer->nbChildren++;
            layoutInt->headerContainer->obj.area.height = textArea->obj.area.height;
            break;
        }
        case HEADER_RIGHT_TEXT: {
            textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            obj      = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) textArea,
                                       headerDesc->rightText.token,
                                       headerDesc->rightText.tuneId);
            if (obj == NULL) {
                return -1;
            }
            textArea->obj.alignment        = MID_RIGHT;
            textArea->obj.alignmentMarginX = BORDER_MARGIN;
            textArea->textColor            = BLACK;
            textArea->obj.area.width       = AVAILABLE_WIDTH;
            textArea->obj.area.height      = TOUCHABLE_HEADER_BAR_HEIGHT;
            textArea->text                 = PIC(headerDesc->rightText.text);
            textArea->fontId               = SMALL_BOLD_FONT;
            textArea->textAlignment        = MID_RIGHT;
            textArea->obj.touchMask        = (1 << TOUCHED);
            textArea->obj.touchId          = TOP_RIGHT_BUTTON_ID;
            // add to bottom container
            layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
                = (nbgl_obj_t *) textArea;
            layoutInt->headerContainer->nbChildren++;
            layoutInt->headerContainer->obj.area.height = textArea->obj.area.height;
            break;
        }
        default:
            return -2;
    }

    if (headerDesc->separationLine) {
        line                = createHorizontalLine(layoutInt->layer);
        line->obj.alignment = BOTTOM_MIDDLE;
        line->offset        = 3;
        layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
            = (nbgl_obj_t *) line;
        layoutInt->headerContainer->nbChildren++;
    }
    if (separationLine != NULL) {
        layoutInt->headerContainer->children[layoutInt->headerContainer->nbChildren]
            = (nbgl_obj_t *) separationLine;
        layoutInt->headerContainer->nbChildren++;
    }
    // header must be the first child
    layoutInt->children[HEADER_INDEX] = (nbgl_obj_t *) layoutInt->headerContainer;

    // subtract header height from main container height
    layoutInt->container->obj.area.height -= layoutInt->headerContainer->obj.area.height;
    layoutInt->container->obj.alignTo   = (nbgl_obj_t *) layoutInt->headerContainer;
    layoutInt->container->obj.alignment = BOTTOM_LEFT;

    layoutInt->headerType = headerDesc->type;

    return layoutInt->headerContainer->obj.area.height;
}

/**
 * @brief Creates a touchable area at the footer of the screen, containing various controls,
 * described in the given structure. This footer is not part of the main container
 *
 * @param layout the current layout
 * @param footerDesc if not @ref NBGL_NO_TUNE, a tune will be played when button is long pressed
 * @return height of the control if OK
 */
int nbgl_layoutAddExtendedFooter(nbgl_layout_t *layout, const nbgl_layoutFooter_t *footerDesc)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    layoutObj_t           *obj;
    nbgl_text_area_t      *textArea;
    nbgl_line_t           *line, *separationLine = NULL;
    nbgl_button_t         *button;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddExtendedFooter():\n");
    if (layout == NULL) {
        return -1;
    }
    if ((footerDesc == NULL) || (footerDesc->type >= NB_FOOTER_TYPES)) {
        return -2;
    }

    layoutInt->footerContainer = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
    layoutInt->footerContainer->obj.area.width = AVAILABLE_WIDTH;
    layoutInt->footerContainer->layout         = VERTICAL;
    layoutInt->footerContainer->children
        = (nbgl_obj_t **) nbgl_containerPoolGet(5, layoutInt->layer);
    layoutInt->footerContainer->obj.alignment = BOTTOM_MIDDLE;

    switch (footerDesc->type) {
        case FOOTER_EMPTY: {
            layoutInt->footerContainer->obj.area.height = footerDesc->emptySpace.height;
            break;
        }
        case FOOTER_SIMPLE_TEXT: {
            textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            obj      = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) textArea,
                                       footerDesc->simpleText.token,
                                       footerDesc->simpleText.tuneId);
            if (obj == NULL) {
                return -1;
            }

            textArea->obj.alignment  = BOTTOM_MIDDLE;
            textArea->textColor      = (footerDesc->simpleText.mutedOut) ? DARK_GRAY : BLACK;
            textArea->obj.area.width = AVAILABLE_WIDTH;
            textArea->obj.area.height
                = (footerDesc->simpleText.mutedOut) ? SMALL_FOOTER_HEIGHT : SIMPLE_FOOTER_HEIGHT;
            textArea->text = PIC(footerDesc->simpleText.text);
            textArea->fontId
                = (footerDesc->simpleText.mutedOut) ? SMALL_REGULAR_FONT : SMALL_BOLD_FONT;
            textArea->textAlignment = CENTER;
            textArea->obj.touchMask = (1 << TOUCHED);
            textArea->obj.touchId   = BOTTOM_BUTTON_ID;
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) textArea;
            layoutInt->footerContainer->nbChildren++;
            layoutInt->footerContainer->obj.area.height = textArea->obj.area.height;
            break;
        }
        case FOOTER_DOUBLE_TEXT: {
            textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            obj      = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) textArea,
                                       footerDesc->doubleText.leftToken,
                                       footerDesc->doubleText.tuneId);
            if (obj == NULL) {
                return -1;
            }
            textArea->obj.alignment   = BOTTOM_LEFT;
            textArea->textColor       = BLACK;
            textArea->obj.area.width  = AVAILABLE_WIDTH / 2;
            textArea->obj.area.height = SIMPLE_FOOTER_HEIGHT;
            textArea->text            = PIC(footerDesc->doubleText.leftText);
            textArea->fontId          = SMALL_BOLD_FONT;
            textArea->textAlignment   = CENTER;
            textArea->obj.touchMask   = (1 << TOUCHED);
            textArea->obj.touchId     = BOTTOM_BUTTON_ID;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) textArea;
            layoutInt->footerContainer->nbChildren++;

            // create right touchable text
            textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            obj      = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) textArea,
                                       footerDesc->doubleText.rightToken,
                                       footerDesc->doubleText.tuneId);
            if (obj == NULL) {
                return -1;
            }

            textArea->obj.alignment   = BOTTOM_RIGHT;
            textArea->textColor       = BLACK;
            textArea->obj.area.width  = AVAILABLE_WIDTH / 2;
            textArea->obj.area.height = SIMPLE_FOOTER_HEIGHT;
            textArea->text            = PIC(footerDesc->doubleText.rightText);
            textArea->fontId          = SMALL_BOLD_FONT;
            textArea->textAlignment   = CENTER;
            textArea->obj.touchMask   = (1 << TOUCHED);
            textArea->obj.touchId     = RIGHT_BUTTON_ID;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) textArea;
            layoutInt->footerContainer->nbChildren++;
            layoutInt->footerContainer->obj.area.height = textArea->obj.area.height;

            // create vertical line separating texts
            separationLine            = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
            separationLine->lineColor = LIGHT_GRAY;
            separationLine->obj.area.width       = 1;
            separationLine->obj.area.height      = layoutInt->footerContainer->obj.area.height;
            separationLine->direction            = VERTICAL;
            separationLine->thickness            = 1;
            separationLine->obj.alignment        = MID_LEFT;
            separationLine->obj.alignTo          = (nbgl_obj_t *) textArea;
            separationLine->obj.alignmentMarginX = -1;
            break;
        }
        case FOOTER_TEXT_AND_NAV: {
            layoutInt->footerContainer->obj.area.width  = SCREEN_WIDTH;
            layoutInt->footerContainer->obj.area.height = SIMPLE_FOOTER_HEIGHT;
            // add touchable text on the left
            textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            obj      = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) textArea,
                                       footerDesc->textAndNav.token,
                                       footerDesc->textAndNav.tuneId);
            if (obj == NULL) {
                return -1;
            }
            textArea->obj.alignment = BOTTOM_LEFT;
            textArea->textColor     = BLACK;
#ifdef TARGET_STAX
            textArea->obj.area.width = 160;
#else   // TARGET_STAX
            textArea->obj.area.width                    = 192;
#endif  // TARGET_STAX
            textArea->obj.area.height = SIMPLE_FOOTER_HEIGHT;
            textArea->text            = PIC(footerDesc->textAndNav.text);
            textArea->fontId          = SMALL_BOLD_FONT;
            textArea->textAlignment   = CENTER;
            textArea->obj.touchMask   = (1 << TOUCHED);
            textArea->obj.touchId     = BOTTOM_BUTTON_ID;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) textArea;
            layoutInt->footerContainer->nbChildren++;

            // add navigation on the right
            nbgl_container_t *navContainer
                = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);
            navContainer->obj.area.width = AVAILABLE_WIDTH;
            navContainer->layout         = VERTICAL;
            navContainer->nbChildren     = 4;
            navContainer->children
                = (nbgl_obj_t **) nbgl_containerPoolGet(navContainer->nbChildren, layoutInt->layer);
            navContainer->obj.alignment   = BOTTOM_RIGHT;
            navContainer->obj.area.width  = SCREEN_WIDTH - textArea->obj.area.width;
            navContainer->obj.area.height = SIMPLE_FOOTER_HEIGHT;
            layoutNavigationPopulate(navContainer, &footerDesc->navigation, layoutInt->layer);
            obj = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) navContainer,
                                       footerDesc->textAndNav.navigation.token,
                                       footerDesc->textAndNav.navigation.tuneId);
            if (obj == NULL) {
                return -1;
            }

            // create vertical line separating text from nav
            separationLine            = (nbgl_line_t *) nbgl_objPoolGet(LINE, layoutInt->layer);
            separationLine->lineColor = LIGHT_GRAY;
            separationLine->obj.area.width       = 1;
            separationLine->obj.area.height      = layoutInt->footerContainer->obj.area.height;
            separationLine->direction            = VERTICAL;
            separationLine->thickness            = 1;
            separationLine->obj.alignment        = MID_LEFT;
            separationLine->obj.alignTo          = (nbgl_obj_t *) navContainer;
            separationLine->obj.alignmentMarginX = -1;

            layoutInt->activePage = footerDesc->textAndNav.navigation.activePage;
            layoutInt->nbPages    = footerDesc->textAndNav.navigation.nbPages;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) navContainer;
            layoutInt->footerContainer->nbChildren++;
            break;
        }
        case FOOTER_NAV: {
            layoutInt->footerContainer->obj.area.width  = SCREEN_WIDTH;
            layoutInt->footerContainer->obj.area.height = SIMPLE_FOOTER_HEIGHT;
            layoutNavigationPopulate(
                layoutInt->footerContainer, &footerDesc->navigation, layoutInt->layer);
            layoutInt->footerContainer->nbChildren = 4;
            obj                                    = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) layoutInt->footerContainer,
                                       footerDesc->navigation.token,
                                       footerDesc->navigation.tuneId);
            if (obj == NULL) {
                return -1;
            }

            layoutInt->activePage = footerDesc->navigation.activePage;
            layoutInt->nbPages    = footerDesc->navigation.nbPages;
            break;
        }
        case FOOTER_SIMPLE_BUTTON: {
            button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
            obj    = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) button,
                                       footerDesc->button.token,
                                       footerDesc->button.tuneId);
            if (obj == NULL) {
                return -1;
            }

            button->obj.alignment = CENTER;
            if (footerDesc->button.style == BLACK_BACKGROUND) {
                button->innerColor      = BLACK;
                button->foregroundColor = WHITE;
            }
            else {
                button->innerColor      = WHITE;
                button->foregroundColor = BLACK;
            }

            if (footerDesc->button.style == NO_BORDER) {
                button->borderColor = WHITE;
            }
            else {
                if (footerDesc->button.style == BLACK_BACKGROUND) {
                    button->borderColor = BLACK;
                }
                else {
                    button->borderColor = LIGHT_GRAY;
                }
            }
            button->text            = PIC(footerDesc->button.text);
            button->fontId          = SMALL_BOLD_FONT;
            button->icon            = PIC(footerDesc->button.icon);
            button->radius          = BUTTON_RADIUS;
            button->obj.area.height = BUTTON_DIAMETER;
#ifdef TARGET_STAX
            layoutInt->footerContainer->obj.area.height = 128;
#else   // TARGET_STAX
            layoutInt->footerContainer->obj.area.height = 136;
#endif  // TARGET_STAX
            if (footerDesc->button.text == NULL) {
                button->obj.area.width = BUTTON_DIAMETER;
            }
            else {
                button->obj.area.width = AVAILABLE_WIDTH;
            }
            button->obj.touchMask = (1 << TOUCHED);
            button->obj.touchId   = button->text ? SINGLE_BUTTON_ID : BOTTOM_BUTTON_ID;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) button;
            layoutInt->footerContainer->nbChildren++;
            break;
        }
        case FOOTER_CHOICE_BUTTONS: {
            // texts cannot be NULL
            if ((footerDesc->choiceButtons.bottomText == NULL)
                || (footerDesc->choiceButtons.topText == NULL)) {
                return -1;
            }

            // create bottomButton (in white) at first
            button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
            obj    = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) button,
                                       footerDesc->choiceButtons.token,
                                       footerDesc->choiceButtons.tuneId);
            if (obj == NULL) {
                return -1;
            }
            // associate with with index 1
            obj->index = 1;
            // put at the bottom of the container
            button->obj.alignment = BOTTOM_MIDDLE;
            if (footerDesc->choiceButtons.style == ROUNDED_AND_FOOTER_STYLE) {
                button->obj.alignmentMarginY = 4;  // 4 pixels from screen bottom
                button->borderColor          = WHITE;
            }
            else if (footerDesc->choiceButtons.style == BOTH_ROUNDED_STYLE) {
                button->obj.alignmentMarginY = 4;      // 4 pixels from screen bottom
                button->borderColor          = WHITE;  // not a real round button on Flex
            }
            button->innerColor      = WHITE;
            button->foregroundColor = BLACK;
            button->obj.area.width  = AVAILABLE_WIDTH;
            button->obj.area.height = BUTTON_DIAMETER;
            button->radius          = BUTTON_RADIUS;
            button->text            = PIC(footerDesc->choiceButtons.bottomText);
            button->fontId          = SMALL_BOLD_FONT;
            button->obj.touchMask   = (1 << TOUCHED);
            button->obj.touchId     = CHOICE_2_ID;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) button;
            layoutInt->footerContainer->nbChildren++;

            // add line if needed
            if (footerDesc->choiceButtons.style == BOTH_ROUNDED_STYLE) {
                line                       = createHorizontalLine(layoutInt->layer);
                line->obj.alignment        = TOP_MIDDLE;
                line->obj.alignmentMarginY = 4;
                line->obj.alignTo          = (nbgl_obj_t *) button;
                layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                    = (nbgl_obj_t *) line;
                layoutInt->footerContainer->nbChildren++;
            }

            // then black button, on top of it
            button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
            obj    = layoutAddCallbackObj(layoutInt,
                                       (nbgl_obj_t *) button,
                                       footerDesc->choiceButtons.token,
                                       footerDesc->choiceButtons.tuneId);
            if (obj == NULL) {
                return -1;
            }
            // associate with with index 0
            obj->index                   = 0;
            button->obj.alignment        = TOP_MIDDLE;
            button->obj.alignmentMarginY = 24;  // 12 pixels from bottom button
            if (footerDesc->choiceButtons.style == BOTH_ROUNDED_STYLE) {
                button->innerColor      = WHITE;
                button->borderColor     = LIGHT_GRAY;
                button->foregroundColor = BLACK;
            }
            else {
                button->innerColor      = BLACK;
                button->borderColor     = BLACK;
                button->foregroundColor = WHITE;
            }
            button->obj.area.width  = AVAILABLE_WIDTH;
            button->obj.area.height = BUTTON_DIAMETER;
            button->radius          = BUTTON_RADIUS;
            button->text            = PIC(footerDesc->choiceButtons.topText);
            button->fontId          = SMALL_BOLD_FONT;
            button->obj.touchMask   = (1 << TOUCHED);
            button->obj.touchId     = CHOICE_1_ID;
            // add to bottom container
            layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
                = (nbgl_obj_t *) button;
            layoutInt->footerContainer->nbChildren++;

#ifdef TARGET_STAX
            if (footerDesc->choiceButtons.style == BOTH_ROUNDED_STYLE) {
                layoutInt->footerContainer->obj.area.height = 232;
            }
            else {
                layoutInt->footerContainer->obj.area.height = 192;
            }
#else   // TARGET_STAX
            if (footerDesc->choiceButtons.style == BOTH_ROUNDED_STYLE) {
                layoutInt->footerContainer->obj.area.height = 232;
            }
            else {
                layoutInt->footerContainer->obj.area.height = 208;
            }
#endif  // TARGET_STAX

            break;
        }
        default:
            return -2;
    }

    // add swipable feature for navigation
    if ((footerDesc->type == FOOTER_NAV) || (footerDesc->type == FOOTER_TEXT_AND_NAV)) {
        addSwipeInternal(layoutInt,
                         ((1 << SWIPED_LEFT) | (1 << SWIPED_RIGHT)),
                         SWIPE_USAGE_NAVIGATION,
                         (footerDesc->type == FOOTER_NAV) ? footerDesc->navigation.token
                                                          : footerDesc->textAndNav.navigation.token,
                         (footerDesc->type == FOOTER_NAV)
                             ? footerDesc->navigation.tuneId
                             : footerDesc->textAndNav.navigation.tuneId);
    }

    if (footerDesc->separationLine) {
        line                = createHorizontalLine(layoutInt->layer);
        line->obj.alignment = TOP_MIDDLE;
        layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
            = (nbgl_obj_t *) line;
        layoutInt->footerContainer->nbChildren++;
    }
    if (separationLine != NULL) {
        layoutInt->footerContainer->children[layoutInt->footerContainer->nbChildren]
            = (nbgl_obj_t *) separationLine;
        layoutInt->footerContainer->nbChildren++;
    }

    layoutInt->children[FOOTER_INDEX] = (nbgl_obj_t *) layoutInt->footerContainer;

    // subtract footer height from main container height
    layoutInt->container->obj.area.height -= layoutInt->footerContainer->obj.area.height;

    layoutInt->footerType = footerDesc->type;

    return layoutInt->footerContainer->obj.area.height;
}

/**
 * @deprecated
 * @brief Creates a kind of navigation bar with an optional <- arrow on the left. This widget is
 * placed on top of the main container
 *
 * @param layout the current layout
 * @param activePage current page [O,(nbPages-1)]
 * @param nbPages number of pages
 * @param withBack if true, the back arrow is drawn
 * @param backToken token used with actionCallback is withBack is true
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when back button is pressed
 * @return the height of the control if OK
 */
int nbgl_layoutAddProgressIndicator(nbgl_layout_t *layout,
                                    uint8_t        activePage,
                                    uint8_t        nbPages,
                                    bool           withBack,
                                    uint8_t        backToken,
                                    tune_index_e   tuneId)
{
    nbgl_layoutHeader_t headerDesc = {.type                        = HEADER_BACK_AND_PROGRESS,
                                      .separationLine              = false,
                                      .progressAndBack.activePage  = activePage,
                                      .progressAndBack.nbPages     = nbPages,
                                      .progressAndBack.token       = backToken,
                                      .progressAndBack.tuneId      = tuneId,
                                      .progressAndBack.withBack    = withBack,
                                      .progressAndBack.actionIcon  = NULL,
                                      .progressAndBack.actionToken = NBGL_INVALID_TOKEN};
    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddProgressIndicator():\n");

    return nbgl_layoutAddHeader(layout, &headerDesc);
}

/**
 * @brief Creates a centered (vertically & horizontally) spinner with a text under it
 *
 * @param layout the current layout
 * @param text text to draw under the spinner
 * @param fixed if set to true, the spinner won't spin and be entirely black
 * @return >= 0 if OK
 */
int nbgl_layoutAddSpinner(nbgl_layout_t *layout, const char *text, bool fixed)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_text_area_t      *textArea;
    nbgl_spinner_t        *spinner;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSpinner():\n");
    if (layout == NULL) {
        return -1;
    }

    // create spinner
    spinner                       = (nbgl_spinner_t *) nbgl_objPoolGet(SPINNER, layoutInt->layer);
    spinner->position             = fixed ? 0xFF : 0;
    spinner->obj.alignmentMarginY = -20;
    spinner->obj.alignTo          = NULL;
    spinner->obj.alignment        = CENTER;
    // set this new spinner as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) spinner);

    // create text area
    textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor     = BLACK;
    textArea->text          = PIC(text);
    textArea->textAlignment = CENTER;
    textArea->fontId        = SMALL_REGULAR_FONT;
    textArea->wrapping      = true;
#ifdef TARGET_STAX
    textArea->obj.alignmentMarginY = 20;
#else   // TARGET_STAX
    textArea->obj.alignmentMarginY = 24;
#endif  // TARGET_STAX
    textArea->obj.alignTo     = (nbgl_obj_t *) spinner;
    textArea->obj.alignment   = BOTTOM_MIDDLE;
    textArea->obj.area.width  = AVAILABLE_WIDTH;
    textArea->obj.area.height = nbgl_getTextHeightInWidth(
        textArea->fontId, textArea->text, textArea->obj.area.width, textArea->wrapping);
    textArea->style = NO_STYLE;

    // center spinner + text vertically
    spinner->obj.alignmentMarginY
        = -(textArea->obj.alignmentMarginY + textArea->obj.area.height) / 2;

    // set this new spinner as child of the container
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    if (!fixed) {
        // update ticker to update the spinner periodically
        nbgl_screenTickerConfiguration_t tickerCfg;

        tickerCfg.tickerIntervale = SPINNER_REFRESH_PERIOD;  // ms
        tickerCfg.tickerValue     = SPINNER_REFRESH_PERIOD;  // ms
        tickerCfg.tickerCallback  = &spinnerTickerCallback;
        nbgl_screenUpdateTicker(layoutInt->layer, &tickerCfg);
    }

    return 0;
}

/**
 * @brief Applies given layout. The screen will be redrawn
 *
 * @param layoutParam layout to redraw
 * @return a pointer to the corresponding layout
 */
int nbgl_layoutDraw(nbgl_layout_t *layoutParam)
{
    nbgl_layoutInternal_t *layout = (nbgl_layoutInternal_t *) layoutParam;

    if (layout == NULL) {
        return -1;
    }
    LOG_DEBUG(LAYOUT_LOGGER,
              "nbgl_layoutDraw(): container.nbChildren =%d, layout->nbChildren = %d\n",
              layout->container->nbChildren,
              layout->nbChildren);
    if (layout->tapText) {
        // set this new container as child of main container
        layoutAddObject(layout, (nbgl_obj_t *) layout->tapText);
    }
    if (layout->withLeftBorder == true) {
        // draw now the line
        nbgl_line_t *line                   = createLeftVerticalLine(layout->layer);
        layout->children[LEFT_BORDER_INDEX] = (nbgl_obj_t *) line;
    }
    nbgl_screenRedraw();

    return 0;
}

/**
 * @brief Release the layout obtained with @ref nbgl_layoutGet()
 *
 * @param layoutParam layout to release
 * @return >= 0 if OK
 */
int nbgl_layoutRelease(nbgl_layout_t *layoutParam)
{
    nbgl_layoutInternal_t *layout = (nbgl_layoutInternal_t *) layoutParam;
    LOG_DEBUG(PAGE_LOGGER, "nbgl_layoutRelease(): \n");
    if (layout == NULL) {
        return -1;
    }
    // if modal
    if (layout->modal) {
        nbgl_screenPop(layout->layer);
    }
    layout->nbChildren = 0;
    return 0;
}

#endif  // HAVE_SE_TOUCH
