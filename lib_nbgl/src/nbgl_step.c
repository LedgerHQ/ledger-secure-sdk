/**
 * @file nbgl_step.c
 * @brief Implementation of predefined pages management for Applications
 */

#ifdef NBGL_STEP
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include "nbgl_debug.h"
#include "nbgl_step_internal.h"
#include "nbgl_layout_internal_nanos.h"
#include "glyphs.h"
#include "os_pic.h"
#include "os_print.h"
#ifdef BUILD_SCREENSHOTS
#include "json_scenario.h"
#endif  // BUILD_SCREENSHOTS

/*********************
 *      DEFINES
 *********************/
// string to store the title for a multi-pages text+subtext
#define TMP_STRING_MAX_LEN 24

///< Maximum number of layers for steps, cannot be greater than max number of layout layers
#define NB_MAX_LAYERS 3

/**********************
 *      TYPEDEFS
 **********************/
/**
 * type of step
 */
typedef enum {
    TEXT_STEP = 0,       ///< for a simple text step
    CENTERED_INFO_STEP,  ///< for a centered info step
    MENU_LIST_STEP       ///< for a menu list step
} StepStype_t;

/**
 * definition of context for a @ref TEXT_STEP or @ref CENTERED_INFO_STEP step
 */
typedef struct TextContext_s {
    uint16_t nbPages;  ///< number of pages for this text step (a page contains at least one byte
                       ///< and text lengths are 16 bits, so it cannot overflow)
    uint16_t    currentPage;    ///< current page for this text step
    const char *txtStart;       ///< pointer on the start point of text (first page)
    const char *nextPageStart;  ///< pointer on the start point of text at the next page
    const char *subTxtStart;    ///< pointer on the start point of sub-text (first page)

    nbgl_stepPosition_t pos;  ///< position of the step within a flow (used for navigation arrows)
    bool                actionOnAnyButton;       ///< if true, action applies on any button
    nbgl_stepButtonCallback_t onActionCallback;  ///< function called when key actions done on this
                                                 ///< step (other than internal navigation)
    char tmpString[TMP_STRING_MAX_LEN];          ///< temporary string used for title when text +
                                                 ///< multi-pages subText
    nbgl_contentCenteredInfoStyle_t
            style;                 ///< style to be used with a @ref CENTERED_INFO_STEP step
    uint8_t nbMaxLinesForSubText;  ///< if > 0, sub-text is truncated to this number of lines (or
                                   ///< buttons) in a single page
} TextContext_t;

/**
 * definition of context for a @ref MENU_LIST_STEP step
 */
typedef struct MenuListContext_s {
    nbgl_stepMenuListCallback_t
        selectedCallback;  ///< function to call when a menu list item is selected
    nbgl_layoutMenuList_t
        list;  ///< structure to store the number of menu items and the way to select them
} MenuListContext_t;

typedef struct StepContext_s {
    union {
        TextContext_t     textContext;  ///< if type is @ref TEXT_STEP or @ref CENTERED_INFO_STEP
        MenuListContext_t menuListContext;  ///< if type is @ref MENU_LIST_STEP
    };
    nbgl_screenTickerConfiguration_t ticker;  ///< structure containing information about ticker
    nbgl_layout_t                   *layout;  ///< handler of the used layout
    StepStype_t                      type;    ///< type of step
    bool                             modal;   ///< true if modal
} StepContext_t;

/**********************
 *  STATIC VARIABLES
 **********************/
///< array of step contexts. Index 0 is reserved for background
static StepContext_t contexts[NB_MAX_LAYERS];

/**********************
 *  VARIABLES
 **********************/
#ifdef BUILD_SCREENSHOTS
// Variables used to store important values (nb lines, bold state etc)
extern uint16_t last_nb_lines;
extern uint16_t last_nb_pages;
extern bool     last_bold_state;
#endif  // BUILD_SCREENSHOTS

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void actionCallback(nbgl_layout_t *layout, nbgl_buttonEvent_t event);
static void menuListActionCallback(nbgl_layout_t *layout, nbgl_buttonEvent_t event);

// returns a non-used step context from the contexts[] array, or NULL if not found
static StepContext_t *getFreeContext(StepStype_t type, bool modal)
{
    StepContext_t *ctx = NULL;

    if (!modal) {
        // Index 0 is reserved for background
        ctx = &contexts[0];
    }
    else {
        uint32_t i = 1;
        while (i < NB_MAX_LAYERS) {
            if (contexts[i].layout == NULL) {
                ctx = &contexts[i];
                break;
            }
            i++;
        }
    }
    if (ctx == NULL) {
        LOG_FATAL(STEP_LOGGER, "getFreeContext(): no available context\n");
    }
    else {
        memset(ctx, 0, sizeof(StepContext_t));
        ctx->type  = type;
        ctx->modal = modal;
    }
    return ctx;
}

// returns the step context from the contexts[] array matching with the given layout handler, or
// NULL if not found
static StepContext_t *getContextFromLayout(nbgl_layout_t layout)
{
    StepContext_t *ctx = NULL;
    uint32_t       i   = 0;
    while (i < NB_MAX_LAYERS) {
        if (contexts[i].layout == layout) {
            ctx = &contexts[i];
            break;
        }
        i++;
    }
    if (ctx == NULL) {
        LOG_WARN(STEP_LOGGER, "getContextFromLayout(): no matching context\n");
    }
    return ctx;
}

// (re)acquires the layout of the given step context. When re-rendering a step (internal page
// navigation in a text step, selection change in a menu list), the layout currently held by the
// context must be released first: for a modal step, nbgl_layoutGet() pushes a new screen and
// consumes one of the few modal layout slots, so not releasing it would leak a screen layer on
// every page, until the pool is exhausted.
static bool getStepLayout(StepContext_t *ctx, const nbgl_layoutDescription_t *layoutDescription)
{
    if (ctx->layout != NULL) {
        nbgl_layoutRelease((nbgl_layout_t *) ctx->layout);
        ctx->layout = NULL;
    }
    ctx->layout = nbgl_layoutGet(layoutDescription);
    if (ctx->layout == NULL) {
        LOG_WARN(STEP_LOGGER, "getStepLayout(): impossible to get a layout\n");
        return false;
    }
    return true;
}

// from the current details context, return the number of bytes of the given subText displayed in
// a page
static uint16_t getSubTextPageLen(StepContext_t *ctx, const char *subText)
{
    uint16_t len = 0;
    if (ctx->textContext.style == BUTTON_INFO) {
        // use exactly the same split as the buttons drawn by the layout
        uint8_t row = 0;
        while ((row < NB_MAX_BUTTON_ROWS) && (subText[len] != '\0')) {
            uint16_t rowLen, rowWidth;
            len += layoutGetButtonRowLen(&subText[len], &rowLen, &rowWidth);
            row++;
        }
    }
    else {
        nbgl_getTextMaxLenInNbLines(BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp,
                                    subText,
                                    AVAILABLE_WIDTH,
                                    NB_MAX_LINES - 1,
                                    &len,
                                    true);
    }
    return len;
}

// from the current details context, return the number of bytes of the given page start of text (or
// of subText if any) displayed in this page, plus the '\n' ending it if any. Used both to move
// forward and to recompute the pages from the beginning, so that they always start at the same char
static uint16_t getPageLen(StepContext_t *ctx, const char *txt)
{
    uint16_t len;
    if (ctx->textContext.subTxtStart == NULL) {
        nbgl_getTextMaxLenInNbLines(
            BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp, txt, AVAILABLE_WIDTH, NB_MAX_LINES, &len, true);
    }
    else {
        len = getSubTextPageLen(ctx, txt);
    }
    // if the next char is '\n', skip it to avoid starting the next page with an empty line
    // (not for the buttons of subText, for which getSubTextPageLen() already handles '\n')
    if (((ctx->textContext.subTxtStart == NULL) || (ctx->textContext.style != BUTTON_INFO))
        && (txt[len] == '\n')) {
        len++;
    }
    return len;
}

// from the current details context, return a pointer on the text (or on subText if any) at the
// given page
static const char *getPageAt(StepContext_t *ctx, uint16_t textPage)
{
    uint16_t    page        = 0;
    const char *currentChar = (ctx->textContext.subTxtStart == NULL) ? ctx->textContext.txtStart
                                                                     : ctx->textContext.subTxtStart;
    while (page < textPage) {
        if (page < (ctx->textContext.nbPages - 1)) {
            currentChar = currentChar + getPageLen(ctx, currentChar);
        }
        page++;
    }
    return currentChar;
}

// utility function to compute navigation arrows
static nbgl_layoutNavIndication_t getNavigationInfo(nbgl_stepPosition_t pos,
                                                    uint16_t            nbPages,
                                                    uint16_t            currentPage)
{
    nbgl_layoutNavIndication_t indication = NO_ARROWS;

    if (nbPages > 1) {
        if (currentPage > 0) {
            indication |= LEFT_ARROW;
        }
        if (currentPage < (nbPages - 1)) {
            indication |= RIGHT_ARROW;
        }
    }
    if (pos == FIRST_STEP) {
        indication |= RIGHT_ARROW;
    }
    else if (pos == LAST_STEP) {
        indication |= LEFT_ARROW;
    }
    else if (pos == NEITHER_FIRST_NOR_LAST_STEP) {
        indication |= RIGHT_ARROW | LEFT_ARROW;
    }
    return indication;
}

// function used to display the current page in details review mode
static void displayTextPage(StepContext_t *ctx, uint16_t textPage)
{
    const char *txt;
    char        intermediateString[36];  // a bit bigger but we know that one line cannot contain
                                         // more than 23 chars

    // if move backward or first page
    if (textPage <= ctx->textContext.currentPage) {
        // recompute current start from beginning
        txt = getPageAt(ctx, textPage);
    }
    // else move forward
    else {
        txt = ctx->textContext.nextPageStart;
    }
    ctx->textContext.currentPage = textPage;

    if (ctx->textContext.currentPage < (ctx->textContext.nbPages - 1)) {
        // memorize next position to save processing
        ctx->textContext.nextPageStart = txt + getPageLen(ctx, txt);
    }
    else {
        ctx->textContext.nextPageStart = NULL;
    }
    nbgl_layoutDescription_t layoutDescription = {0};
    nbgl_layoutNavigation_t  navInfo           = {
        .direction = HORIZONTAL_NAV,
    };

    layoutDescription.modal                  = ctx->modal;
    layoutDescription.onActionCallback       = actionCallback;
    layoutDescription.ticker.tickerCallback  = ctx->ticker.tickerCallback;
    layoutDescription.ticker.tickerIntervale = ctx->ticker.tickerIntervale;
    layoutDescription.ticker.tickerValue     = ctx->ticker.tickerValue;
    if (!getStepLayout(ctx, &layoutDescription)) {
        return;
    }

    navInfo.indication = getNavigationInfo(
        ctx->textContext.pos, ctx->textContext.nbPages, ctx->textContext.currentPage);

    if (ctx->textContext.subTxtStart == NULL) {
        nbgl_layoutAddText(ctx->layout, txt, NULL, ctx->textContext.style);
    }
    else {
        if (ctx->textContext.nbPages == 1) {
            // truncate title to fit in one line, if necessary
            if (nbgl_getTextNbLinesInWidth(BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp,
                                           ctx->textContext.txtStart,
                                           AVAILABLE_WIDTH,
                                           false)
                > 1) {
                nbgl_textReduceOnNbLines(BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp,
                                         ctx->textContext.txtStart,
                                         AVAILABLE_WIDTH,
                                         1,
                                         ctx->textContext.tmpString,
                                         TMP_STRING_MAX_LEN);
            }
            else {
                if (ctx->textContext.txtStart != NULL) {
                    // simply copy (without reading after the end of a shorter text)
                    strlcpy(
                        ctx->textContext.tmpString, ctx->textContext.txtStart, TMP_STRING_MAX_LEN);
                }
                else {
                    ctx->textContext.tmpString[0] = 0;
                }
            }
        }
        else {
            SPRINTF(intermediateString,
                    "%s (%d/%d)",
                    ctx->textContext.txtStart,
                    (int) (ctx->textContext.currentPage + 1),
                    (int) ctx->textContext.nbPages);
            // truncate title to fit in one line, if necessary
            if (nbgl_getTextNbLinesInWidth(BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp,
                                           intermediateString,
                                           AVAILABLE_WIDTH,
                                           false)
                > 1) {
                nbgl_textReduceOnNbLines(BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp,
                                         intermediateString,
                                         AVAILABLE_WIDTH,
                                         1,
                                         ctx->textContext.tmpString,
                                         TMP_STRING_MAX_LEN);
            }
            else {
                // simply copy
                memcpy(ctx->textContext.tmpString, intermediateString, TMP_STRING_MAX_LEN - 1);
                ctx->textContext.tmpString[TMP_STRING_MAX_LEN - 1] = 0;
            }
        }
        // if the sub text has a max number of lines, it is displayed in a single page, truncated
        // with an ellipsis if it doesn't fit. Otherwise, the text of the current page fills the
        // lines (or buttons), the remaining text is displayed in the next pages, so no ellipsis
        layoutAddText(ctx->layout,
                      ctx->textContext.tmpString,
                      txt,
                      ctx->textContext.style,
                      ctx->textContext.nbMaxLinesForSubText,
                      ctx->textContext.nbMaxLinesForSubText > 0);
    }
    if (navInfo.indication != NO_ARROWS) {
        nbgl_layoutAddNavigation(ctx->layout, &navInfo);
    }
    nbgl_layoutDraw(ctx->layout);
    nbgl_refresh();
}

// callback on key touch
static void actionCallback(nbgl_layout_t *layout, nbgl_buttonEvent_t event)
{
    StepContext_t *ctx = getContextFromLayout(layout);

    if (!ctx) {
        LOG_WARN(STEP_LOGGER, "actionCallback(): ctx not found\n");
        return;
    }
    if (event == BUTTON_LEFT_PRESSED) {
        if (ctx->textContext.currentPage > 0) {
            displayTextPage(ctx, ctx->textContext.currentPage - 1);
            return;
        }
        else if ((ctx->textContext.pos == LAST_STEP)
                 || (ctx->textContext.pos == NEITHER_FIRST_NOR_LAST_STEP)
                 || (ctx->textContext.actionOnAnyButton)) {
            if (ctx->textContext.onActionCallback != NULL) {
                ctx->textContext.onActionCallback((nbgl_step_t) ctx, event);
            }
        }
    }
    else if (event == BUTTON_RIGHT_PRESSED) {
        if (ctx->textContext.currentPage < (ctx->textContext.nbPages - 1)) {
            displayTextPage(ctx, ctx->textContext.currentPage + 1);
            return;
        }
        else if ((ctx->textContext.pos == FIRST_STEP)
                 || (ctx->textContext.pos == NEITHER_FIRST_NOR_LAST_STEP)
                 || (ctx->textContext.actionOnAnyButton)) {
            if (ctx->textContext.onActionCallback != NULL) {
                ctx->textContext.onActionCallback((nbgl_step_t) ctx, event);
            }
        }
    }
    else if (event == BUTTON_BOTH_PRESSED) {
        if (ctx->textContext.onActionCallback != NULL) {
            ctx->textContext.onActionCallback((nbgl_step_t) ctx, event);
        }
    }
}

static void displayMenuList(StepContext_t *ctx)
{
    nbgl_layoutDescription_t layoutDescription
        = {.modal = ctx->modal, .onActionCallback = menuListActionCallback};
    nbgl_layoutMenuList_t *list = &ctx->menuListContext.list;

    layoutDescription.ticker.tickerCallback  = ctx->ticker.tickerCallback;
    layoutDescription.ticker.tickerIntervale = ctx->ticker.tickerIntervale;
    layoutDescription.ticker.tickerValue     = ctx->ticker.tickerValue;

    if (!getStepLayout(ctx, &layoutDescription)) {
        return;
    }
    nbgl_layoutAddMenuList(ctx->layout, list);
    if (list->nbChoices > 1) {
        nbgl_layoutNavigation_t navInfo = {.direction = VERTICAL_NAV};
        navInfo.indication              = 0;
        if (list->selectedChoice > 0) {
            navInfo.indication |= LEFT_ARROW;
        }
        if (list->selectedChoice < (list->nbChoices - 1)) {
            navInfo.indication |= RIGHT_ARROW;
        }

        if (navInfo.indication != NO_ARROWS) {
            nbgl_layoutAddNavigation(ctx->layout, &navInfo);
        }
    }
    nbgl_layoutDraw(ctx->layout);
    nbgl_refresh();
}

// callback on key touch
static void menuListActionCallback(nbgl_layout_t *layout, nbgl_buttonEvent_t event)
{
    StepContext_t *ctx = getContextFromLayout(layout);
    if (!ctx) {
        LOG_WARN(STEP_LOGGER, "menuListActionCallback(): ctx not found\n");
        return;
    }

    if (event == BUTTON_LEFT_PRESSED) {
        if (ctx->menuListContext.list.selectedChoice > 0) {
            ctx->menuListContext.list.selectedChoice--;
            displayMenuList(ctx);
        }
    }
    else if (event == BUTTON_RIGHT_PRESSED) {
        if (ctx->menuListContext.list.selectedChoice < (ctx->menuListContext.list.nbChoices - 1)) {
            ctx->menuListContext.list.selectedChoice++;
            displayMenuList(ctx);
        }
    }
    else if (event == BUTTON_BOTH_PRESSED) {
        ctx->menuListContext.selectedCallback(ctx->menuListContext.list.selectedChoice);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief draws a text type step, that can be multi-pages, depending of the length of text and
 * subText. The navigation arrows are displayed depending of the given position of this step in a
 * flow, and depending of the page in case of multi-pages
 *
 * @param pos position of this step in the flow (first, last, single, not_first_nor_last)
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param text text to display (depending of style)
 * @param subText text to display under text (depending of style)
 * @param style style to use for text and subText
 * @param modal if true, means this step shall be displayed on top of existing one
 * @return the step context (or NULL if error)
 */
nbgl_step_t nbgl_stepDrawText(nbgl_stepPosition_t               pos,
                              nbgl_stepButtonCallback_t         onActionCallback,
                              nbgl_screenTickerConfiguration_t *ticker,
                              const char                       *text,
                              const char                       *subText,
                              nbgl_contentCenteredInfoStyle_t   style,
                              bool                              modal)
{
    return stepDrawText(pos, onActionCallback, ticker, text, subText, style, modal, 0);
}

/**
 * @brief Same as @ref nbgl_stepDrawText(), with the possibility to truncate subText
 *
 * @param pos position of this step in the flow (first, last, single, not_first_nor_last)
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param text text to display (depending of style)
 * @param subText text to display under text (depending of style)
 * @param style style to use for text and subText
 * @param modal if true, means this step shall be displayed on top of existing one
 * @param nbMaxLinesForSubText if > 0, subText is displayed in a single page, in at most this
 * number of lines (or buttons in @ref BUTTON_INFO style), the last one ending with "..." if it
 * doesn't fit. Otherwise, subText is displayed in as many pages as necessary
 * @return the step context (or NULL if error)
 */
nbgl_step_t stepDrawText(nbgl_stepPosition_t               pos,
                         nbgl_stepButtonCallback_t         onActionCallback,
                         nbgl_screenTickerConfiguration_t *ticker,
                         const char                       *text,
                         const char                       *subText,
                         nbgl_contentCenteredInfoStyle_t   style,
                         bool                              modal,
                         uint8_t                           nbMaxLinesForSubText)
{
#ifdef BUILD_SCREENSHOTS
    // Initialize a default area
    nbgl_area_t area = {0};
    area.x0 = area.y0 = 0;
    area.width        = AVAILABLE_WIDTH;
    area.height = NB_MAX_LINES * nbgl_getFontLineHeight(BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp);
#endif  // BUILD_SCREENSHOTS
    StepContext_t *ctx = getFreeContext(TEXT_STEP, modal);
    if (!ctx) {
        LOG_WARN(STEP_LOGGER, "nbgl_stepDrawText(): no free context\n");
        return NULL;
    }
    // the texts are read here (to count and split the pages) and when displaying the pages, so
    // they must always be accessed with the same (PIC) address (PIC(NULL) is NULL)
    text    = PIC(text);
    subText = PIC(subText);
    // initialize context (already set to 0 by getFreeContext())
    ctx->textContext.onActionCallback = onActionCallback;
    // style is needed to compute the number of pages of subText
    ctx->textContext.style = style;
    if (ticker) {
        ctx->ticker.tickerCallback  = ticker->tickerCallback;
        ctx->ticker.tickerIntervale = ticker->tickerIntervale;
        ctx->ticker.tickerValue     = ticker->tickerValue;
    }

    // if no subText, get the number of pages for main text
    if (subText == NULL) {
        ctx->textContext.nbPages = nbgl_getTextNbPagesInWidth(
            BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp, text, NB_MAX_LINES, AVAILABLE_WIDTH);
#ifdef BUILD_SCREENSHOTS
        store_string_infos(text,
                           BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp,
                           &area,
                           true,
                           last_nb_lines,
                           last_nb_pages,
                           last_bold_state);
#endif  // BUILD_SCREENSHOTS
    }
    else {
#ifdef BUILD_SCREENSHOTS
        uint16_t nb_lines_title;
        // Call this function to get correct nb_lines/nb_pages for text field.
        nbgl_getTextNbPagesInWidth(
            BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp, text, NB_MAX_LINES, AVAILABLE_WIDTH);
        nb_lines_title = last_nb_lines;
        // There is a sub text, so no more than 3 lines for title...
        if (nb_lines_title > 3) {
            nb_lines_title = 3;
        }
        area.height
            = nb_lines_title * nbgl_getFontLineHeight(BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp);
        store_string_infos(text,
                           BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp,
                           &area,
                           true,
                           last_nb_lines,
                           last_nb_pages,
                           last_bold_state);
#endif  // BUILD_SCREENSHOTS
        if (nbMaxLinesForSubText > 0) {
            // subText is truncated to fit in a single page
            ctx->textContext.nbMaxLinesForSubText = nbMaxLinesForSubText;
            ctx->textContext.nbPages              = 1;
        }
        else if (style == BUTTON_INFO) {
            // count the pages with exactly the same split as the one used to display them. Each
            // page contains at least one char, so all the text is always reachable
            const char *pageStart = subText;
            do {
                pageStart += getSubTextPageLen(ctx, pageStart);
                ctx->textContext.nbPages++;
            } while (*pageStart != '\0');
        }
        else {
            // NB_MAX_LINES-1 because first line is for main text
            ctx->textContext.nbPages = nbgl_getTextNbPagesInWidth(
                BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp, subText, NB_MAX_LINES - 1, AVAILABLE_WIDTH);
        }
#ifdef BUILD_SCREENSHOTS
        if ((nbMaxLinesForSubText > 0) || (style == BUTTON_INFO)) {
            // Call this function to get correct nb_lines for subText (its pages are counted above)
            nbgl_getTextNbPagesInWidth(
                BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp, subText, NB_MAX_LINES - 1, AVAILABLE_WIDTH);
        }
        area.height = (NB_MAX_LINES - nb_lines_title)
                      * nbgl_getFontLineHeight(BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp);
        // If subTxtid is not valid, we'll use txtId for nb_lines/nb_pages values
        // (in BUTTON_INFO style or if truncated, the pages are the ones really displayed, and if
        // truncated in a text style, only nbMaxLinesForSubText lines are displayed. In
        // BUTTON_INFO style, nbMaxLinesForSubText is a number of buttons, not of lines)
        store_string_infos(subText,
                           BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp,
                           &area,
                           true,
                           ((style != BUTTON_INFO) && (nbMaxLinesForSubText > 0)
                            && (last_nb_lines > nbMaxLinesForSubText))
                               ? nbMaxLinesForSubText
                               : last_nb_lines,
                           ((style == BUTTON_INFO) || (nbMaxLinesForSubText > 0))
                               ? ctx->textContext.nbPages
                               : last_nb_pages,
                           last_bold_state);
#endif  // BUILD_SCREENSHOTS
    }
    LOG_DEBUG(STEP_LOGGER,
              "nbgl_stepDrawText: ctx = %p, nbPages = %d, pos = 0x%X\n",
              ctx,
              (int) ctx->textContext.nbPages,
              pos);
    if (pos & BACKWARD_DIRECTION) {
        // start with last page
        ctx->textContext.currentPage = ctx->textContext.nbPages - 1;
    }
    ctx->textContext.txtStart    = text;
    ctx->textContext.subTxtStart = subText;
    // keep only direction part of position
    ctx->textContext.pos               = pos & STEP_POSITION_MASK;
    ctx->textContext.actionOnAnyButton = (pos & ACTION_ON_ANY_BUTTON) != 0;
    displayTextPage(ctx, ctx->textContext.currentPage);

    return (nbgl_step_t) ctx;
}

/**
 * @brief draw a step with a centered info (icon + text). This is always a single page step
 *
 * @param pos position of this step in the flow (first, last, single, not_first_nor_last)
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param info all information about the centered info to be displayed
 * @param modal if true, means this step shall be displayed on top of existing one
 * @return the step context (or NULL if error)
 */
nbgl_step_t nbgl_stepDrawCenteredInfo(nbgl_stepPosition_t               pos,
                                      nbgl_stepButtonCallback_t         onActionCallback,
                                      nbgl_screenTickerConfiguration_t *ticker,
                                      nbgl_layoutCenteredInfo_t        *info,
                                      bool                              modal)
{
    nbgl_layoutDescription_t layoutDescription
        = {.modal = modal, .onActionCallback = (nbgl_layoutButtonCallback_t) actionCallback};
    nbgl_layoutNavigation_t navInfo = {
        .direction = HORIZONTAL_NAV,
    };
    StepContext_t *ctx = getFreeContext(CENTERED_INFO_STEP, modal);
    if (!ctx) {
        LOG_WARN(STEP_LOGGER, "nbgl_stepDrawCenteredInfo(): no free context\n");
        return NULL;
    }

    // initialize context (already set to 0 by getFreeContext())
    ctx->textContext.onActionCallback = onActionCallback;
    if (ticker) {
        ctx->ticker.tickerCallback               = ticker->tickerCallback;
        ctx->ticker.tickerIntervale              = ticker->tickerIntervale;
        ctx->ticker.tickerValue                  = ticker->tickerValue;
        layoutDescription.ticker.tickerCallback  = ticker->tickerCallback;
        layoutDescription.ticker.tickerIntervale = ticker->tickerIntervale;
        layoutDescription.ticker.tickerValue     = ticker->tickerValue;
    }

    ctx->textContext.nbPages = 1;
    // keep only direction part of position
    ctx->textContext.pos = pos & (RIGHT_ARROW | LEFT_ARROW);
    navInfo.indication   = getNavigationInfo(
        ctx->textContext.pos, ctx->textContext.nbPages, ctx->textContext.currentPage);

    ctx->layout = nbgl_layoutGet(&layoutDescription);
    nbgl_layoutAddCenteredInfo(ctx->layout, info);
    if (navInfo.indication != NO_ARROWS) {
        nbgl_layoutAddNavigation(ctx->layout, &navInfo);
    }
    nbgl_layoutDraw(ctx->layout);
    nbgl_refresh();

    LOG_DEBUG(STEP_LOGGER, "nbgl_stepDrawCenteredInfo(): step = %p\n", ctx);
    return (nbgl_step_t) ctx;
}

/**
 * @brief draw a step page with a menu list and navigation arrows to parse it. This step must be
 * alone
 *
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param list configuration of the menu list
 * @param modal if true, means this step shall be displayed on top of existing one
 * @return the step context (or NULL if error)
 */
nbgl_step_t nbgl_stepDrawMenuList(nbgl_stepMenuListCallback_t       onActionCallback,
                                  nbgl_screenTickerConfiguration_t *ticker,
                                  nbgl_layoutMenuList_t            *list,
                                  bool                              modal)
{
    StepContext_t *ctx = getFreeContext(MENU_LIST_STEP, modal);
    if (!ctx) {
        LOG_WARN(STEP_LOGGER, "nbgl_stepDrawMenuList(): no free context\n");
        return NULL;
    }

    // initialize context (already set to 0 by getFreeContext())
    if (ticker) {
        ctx->ticker.tickerCallback  = ticker->tickerCallback;
        ctx->ticker.tickerIntervale = ticker->tickerIntervale;
        ctx->ticker.tickerValue     = ticker->tickerValue;
    }

    ctx->menuListContext.list.nbChoices      = list->nbChoices;
    ctx->menuListContext.list.selectedChoice = list->selectedChoice;
    ctx->menuListContext.list.callback       = list->callback;
    ctx->menuListContext.selectedCallback    = onActionCallback;

    displayMenuList(ctx);

    LOG_DEBUG(STEP_LOGGER, "nbgl_stepDrawMenuList(): step = %p\n", ctx);

    return (nbgl_step_t) ctx;
}

/**
 * @brief Get the index of the currently selected item in the menulist
 *
 * @param step step from which to get the current menulist choice
 * @return current menulist choice
 */
uint8_t nbgl_stepGetMenuListCurrent(nbgl_step_t step)
{
    StepContext_t *ctx = (StepContext_t *) step;
    if (!ctx) {
        return 0;
    }
    return (ctx->menuListContext.list.selectedChoice);
}

/**
 * @brief draw a step page with a switch and navigation arrows to navigate to other pages.
 *
 * @note Switch name must fit in one line, description on 2 lines max.
 *
 * @param pos position of this step in the flow (first, last, single, not_first_nor_last)
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param switchInfo all information about the switch to be displayed
 * @param modal if true, means this step shall be displayed on top of existing one
 * @return the step context (or NULL if error)
 */
nbgl_step_t nbgl_stepDrawSwitch(nbgl_stepPosition_t               pos,
                                nbgl_stepButtonCallback_t         onActionCallback,
                                nbgl_screenTickerConfiguration_t *ticker,
                                nbgl_layoutSwitch_t              *switchInfo,
                                bool                              modal)
{
    nbgl_layoutDescription_t layoutDescription
        = {.modal = modal, .onActionCallback = (nbgl_layoutButtonCallback_t) actionCallback};
    nbgl_layoutNavigation_t navInfo = {
        .direction = HORIZONTAL_NAV,
    };
    StepContext_t *ctx = getFreeContext(CENTERED_INFO_STEP, modal);
    if (!ctx) {
        LOG_WARN(STEP_LOGGER, "nbgl_stepDrawSwitch(): no free context\n");
        return NULL;
    }

    // initialize context (already set to 0 by getFreeContext())
    ctx->textContext.onActionCallback = onActionCallback;
    if (ticker) {
        ctx->ticker.tickerCallback               = ticker->tickerCallback;
        ctx->ticker.tickerIntervale              = ticker->tickerIntervale;
        ctx->ticker.tickerValue                  = ticker->tickerValue;
        layoutDescription.ticker.tickerCallback  = ticker->tickerCallback;
        layoutDescription.ticker.tickerIntervale = ticker->tickerIntervale;
        layoutDescription.ticker.tickerValue     = ticker->tickerValue;
    }

    ctx->textContext.nbPages = 1;
    // keep only direction part of position
    ctx->textContext.pos = pos & (RIGHT_ARROW | LEFT_ARROW);
    navInfo.indication   = getNavigationInfo(
        ctx->textContext.pos, ctx->textContext.nbPages, ctx->textContext.currentPage);

    ctx->layout = nbgl_layoutGet(&layoutDescription);
    nbgl_layoutAddSwitch(ctx->layout, switchInfo);
    if (navInfo.indication != NO_ARROWS) {
        nbgl_layoutAddNavigation(ctx->layout, &navInfo);
    }
    nbgl_layoutDraw(ctx->layout);
    nbgl_refresh();

    LOG_DEBUG(STEP_LOGGER, "nbgl_stepDrawSwitch(): ctx = %p\n", ctx);
    return (nbgl_step_t) ctx;
}

/**
 * @brief Release the step obtained with any of the nbgl_stepDrawXXX() functions
 *
 * @param step step to release
 * @return >= 0 if OK
 */
int nbgl_stepRelease(nbgl_step_t step)
{
    StepContext_t *ctx = (StepContext_t *) step;
    int            ret;

    LOG_DEBUG(STEP_LOGGER, "nbgl_stepRelease(): ctx = %p\n", ctx);
    if (!ctx) {
        return -1;
    }
    ret = nbgl_layoutRelease((nbgl_layout_t *) ctx->layout);

    ctx->layout = NULL;

    return ret;
}

#endif  // NBGL_STEP
