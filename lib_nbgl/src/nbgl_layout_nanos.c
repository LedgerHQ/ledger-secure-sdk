/**
 * @file nbgl_layout.c
 * @brief Implementation of predefined layouts management for Applications
 */

#ifndef HAVE_SE_TOUCH
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdlib.h>
#include "nbgl_debug.h"
#include "nbgl_front.h"
#include "nbgl_layout_internal_nanos.h"
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

#define NB_MAX_LAYOUTS 3

// used by screen
#define NB_MAX_SCREEN_CHILDREN 7

#define ELLIPSIS      "..."
#define ELLIPSIS_SIZE sizeof(ELLIPSIS)  // includes '\0'

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**
 * @brief array of layouts, if used by modal
 *
 */
static nbgl_layoutInternal_t gLayout[NB_MAX_LAYOUTS] = {0};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void buttonCallback(nbgl_screen_t *screen, nbgl_buttonEvent_t buttonEvent)
{
    uint8_t                i      = NB_MAX_LAYOUTS;
    nbgl_layoutInternal_t *layout = NULL;

    // parse all layouts (starting with modals) to find the object
    while (i > 0) {
        i--;
        if ((screen->index == gLayout[i].layer) && (gLayout[i].nbChildren > 0)) {
            // found
            layout = &gLayout[i];
            break;
        }
    }
    if (layout == NULL) {
        LOG_WARN(
            LAYOUT_LOGGER,
            "touchCallback(): screen->index = %d, buttonEvent = %d, no matching active layout\n",
            screen->index,
            buttonEvent);
        return;
    }

#ifdef NBGL_KEYPAD
    // special case of keypad
    nbgl_obj_t *kpd = nbgl_screenContainsObjType(screen, KEYPAD);
    if (kpd) {
        nbgl_keypadCallback(kpd, buttonEvent);
        return;
    }
    else
#endif  // NBGL_KEYPAD
#ifdef NBGL_KEYBOARD
    {
        nbgl_obj_t *kbd = nbgl_screenContainsObjType(screen, KEYBOARD);
        if (kbd) {
            nbgl_keyboardCallback(kbd, buttonEvent);
            return;
        }
    }
#endif  // NBGL_KEYBOARD
    if (layout->callback != NULL) {
        layout->callback((nbgl_layout_t *) layout, buttonEvent);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief adds the given obj to the layout
 *
 * @param layout
 * @param obj
 */
void layoutAddObject(nbgl_layoutInternal_t *layout, nbgl_obj_t *obj)
{
    if (layout->nbChildren >= NB_MAX_SCREEN_CHILDREN) {
        LOG_FATAL(LAYOUT_LOGGER, "layoutAddObject(): No more object\n");
        // LOG_FATAL is a no-op in release builds: never write past the array
        return;
    }
    layout->children[layout->nbChildren] = obj;
    layout->nbChildren++;
}

/**
 * @brief computes the part of the given text fitting in the given width and in the given number
 * of bytes, in the font of the buttons in @ref BUTTON_INFO style
 *
 * @param text text to display, starting at the beginning of the button
 * @param maxWidth max width in pixels of the text
 * @param maxLen max number of bytes of the text
 * @param len [out] number of bytes of text fitting
 * @param width [out] width in pixels of these bytes
 */
static void getButtonTextLen(const char *text,
                             uint16_t    maxWidth,
                             uint16_t    maxLen,
                             uint16_t   *len,
                             uint16_t   *width)
{
    nbgl_getTextMaxLenAndWidth(
        BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp, text, maxWidth, len, width, true);
    // a button cannot contain more than the buffer used to display it
    if (*len > maxLen) {
        *len = maxLen;
        // don't cut in the middle of an UTF-8 char
        while ((*len > 0) && ((text[*len] & 0xC0) == 0x80)) {
            (*len)--;
        }
        *width
            = nbgl_getSingleLineTextWidthInLen(BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp, text, *len);
    }
}

/**
 * @brief displays the given number of lines of the text of the given text area, the last line
 * ending with an ellipsis. The text is not copied: the first lines are displayed by the given text
 * area, the last line and the ellipsis by 2 other text areas, aligned under it
 *
 * @param layoutInt the current layout
 * @param container container of the text areas
 * @param textArea text area of the sub text, already set at index 1 of container
 * @param nbLines number of lines of the sub text
 * @param areaHeight height of the area in which the lines are vertically centered
 */
static void addTextWithEllipsis(nbgl_layoutInternal_t *layoutInt,
                                nbgl_container_t      *container,
                                nbgl_text_area_t      *textArea,
                                uint8_t                nbLines,
                                uint16_t               areaHeight)
{
    const char       *text          = textArea->text;
    const uint16_t    lineHeight    = nbgl_getFontLineHeight(textArea->fontId);
    const uint16_t    ellipsisWidth = nbgl_getTextWidth(textArea->fontId, ELLIPSIS);
    nbgl_text_area_t *lastLineArea;
    nbgl_text_area_t *ellipsisArea;
    uint16_t          firstLinesLen = 0;
    uint16_t          lastLineStart = 0;
    uint16_t          lastLineLen   = 0;
    uint16_t          lastLineWidth = 0;

    // all lines but the last one are fully displayed by the given text area (empty if a single
    // line), vertically centered with the last line in the area
    if (nbLines > 1) {
        nbgl_getTextMaxLenInNbLines(
            textArea->fontId, text, AVAILABLE_WIDTH, nbLines - 1, &firstLinesLen, true);
        // the '\n' ending these lines is not displayed
        lastLineStart = (text[firstLinesLen] == '\n') ? (firstLinesLen + 1) : firstLinesLen;
    }
    // a len of 0 would mean the whole text, so nothing to display is a NULL text
    if (firstLinesLen == 0) {
        textArea->text = NULL;
    }
    textArea->len             = firstLinesLen;
    textArea->nbMaxLines      = nbLines - 1;
    textArea->obj.area.height = (nbLines - 1) * lineHeight;
    textArea->obj.alignmentMarginY += (areaHeight - nbLines * lineHeight) / 2;

    // the last line is displayed by another text area, under the first lines
    lastLineArea              = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    lastLineArea->textColor   = WHITE;
    lastLineArea->fontId      = textArea->fontId;
    lastLineArea->obj.alignTo = (nbgl_obj_t *) textArea;
    container->children[2]    = (nbgl_obj_t *) lastLineArea;
    // the last line must leave room for the ellipsis
    nbgl_getTextMaxLenAndWidth(textArea->fontId,
                               &text[lastLineStart],
                               AVAILABLE_WIDTH - ellipsisWidth,
                               &lastLineLen,
                               &lastLineWidth,
                               true);
    // the last line may be empty (if cut at an empty line), then only the ellipsis is displayed
    lastLineArea->text            = (lastLineLen > 0) ? &text[lastLineStart] : NULL;
    lastLineArea->len             = lastLineLen;
    lastLineArea->nbMaxLines      = 1;
    lastLineArea->wrapping        = false;
    lastLineArea->textAlignment   = CENTER;
    lastLineArea->obj.area.width  = lastLineWidth;
    lastLineArea->obj.area.height = lineHeight;
    lastLineArea->obj.alignment   = BOTTOM_MIDDLE;
    // the last line and the ellipsis are horizontally centered together
    lastLineArea->obj.alignmentMarginX = -(ellipsisWidth / 2);

    // the ellipsis is on the right of the last line
    ellipsisArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    ellipsisArea->textColor     = WHITE;
    ellipsisArea->text          = (const char *) PIC(ELLIPSIS);
    ellipsisArea->fontId        = textArea->fontId;
    ellipsisArea->textAlignment = CENTER;
    ellipsisArea->obj.area.width  = ellipsisWidth;
    ellipsisArea->obj.area.height = lineHeight;
    ellipsisArea->obj.alignment   = MID_RIGHT;
    ellipsisArea->obj.alignTo     = (nbgl_obj_t *) lastLineArea;
    container->children[3]        = (nbgl_obj_t *) ellipsisArea;
}

/**
 * @brief computes the part of the given text displayed in a single button in @ref BUTTON_INFO
 * style. Used both to draw the buttons and to split the text in pages, so that they always match
 *
 * @param text text to display, starting at the beginning of the button
 * @param len [out] number of bytes of text displayed in the button
 * @param width [out] width in pixels of these bytes
 * @return the number of bytes of text consumed by the button (always > 0 if text is not empty)
 */
uint16_t layoutGetButtonRowLen(const char *text, uint16_t *len, uint16_t *width)
{
    uint16_t consumed;

    // the rest of the text goes in the next button
    getButtonTextLen(
        text, AVAILABLE_WIDTH - BUTTON_MARGIN_Y, NB_MAX_CHAR_IN_BUTTON_ROW - 1, len, width);
    consumed = *len;
    // a '\n' ending the button is consumed but not displayed
    if (text[consumed] == '\n') {
        consumed++;
    }
    // always consume at least one char, to be sure to progress in the text
    if ((consumed == 0) && (text[0] != '\0')) {
        consumed = 1;
    }
    return consumed;
}

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

    layout->callback = (nbgl_layoutButtonCallback_t) PIC(description->onActionCallback);
    layout->modal    = description->modal;
    if (description->modal) {
        layout->layer = nbgl_screenPush(&layout->children,
                                        NB_MAX_SCREEN_CHILDREN,
                                        &description->ticker,
                                        (nbgl_buttonCallback_t) buttonCallback);
    }
    else {
        nbgl_screenSet(&layout->children,
                       NB_MAX_SCREEN_CHILDREN,
                       &description->ticker,
                       (nbgl_buttonCallback_t) buttonCallback);
        layout->layer = 0;
    }

    return (nbgl_layout_t *) layout;
}

/**
 * @brief Creates navigation arrows on side(s) of the screen
 *
 * @param layout the current layout
 * @param info structure giving the description of the navigation
 * @return >= 0 if OK
 */
int nbgl_layoutAddNavigation(nbgl_layout_t *layout, nbgl_layoutNavigation_t *info)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddNavigation():\n");
    if (layout == NULL) {
        return -1;
    }

    nbgl_image_t *image;
    if (info->indication & LEFT_ARROW) {
        image                  = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        image->foregroundColor = WHITE;
        image->buffer          = (info->direction == HORIZONTAL_NAV) ? &C_icon_left : &C_icon_up;
        image->obj.area.bpp    = NBGL_BPP_1;
        image->obj.alignment   = MID_LEFT;
        layoutAddObject(layoutInt, (nbgl_obj_t *) image);
    }
    if (info->indication & RIGHT_ARROW) {
        image                  = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        image->foregroundColor = WHITE;
        image->buffer          = (info->direction == HORIZONTAL_NAV) ? &C_icon_right : &C_icon_down;
        image->obj.area.bpp    = NBGL_BPP_1;
        image->obj.alignment   = MID_RIGHT;
        layoutAddObject(layoutInt, (nbgl_obj_t *) image);
    }
    return 0;
}

/**
 * @brief Creates an area with given text and sub text, using the given style
 *
 * @param layout the current layout
 * @param text main text, must be single line if subText
 * @param subText description under main text (NULL terminated, in regular, 3 lines max, or 2
 * buttons max in @ref BUTTON_INFO style, may be null)
 * @param style if @ref REGULAR_INFO, use regular font for text, otherwise use bold font for text
 * @return >= 0 if OK
 */
int nbgl_layoutAddText(nbgl_layout_t                  *layout,
                       const char                     *text,
                       const char                     *subText,
                       nbgl_contentCenteredInfoStyle_t style)
{
    // In BUTTON_INFO style, the sub text is displayed in at most 2 buttons, the last one ending
    // with an ellipsis if it doesn't fit (the 3rd button is only used by steps, to split long texts
    // in pages). Other sub texts are simply cut.
    if (style == BUTTON_INFO) {
        return layoutAddText(layout, text, subText, style, NB_MAX_BUTTON_ROWS_PUBLIC, true);
    }
    return layoutAddText(layout, text, subText, style, 0, false);
}

/**
 * @brief Same as @ref nbgl_layoutAddText(), with the choice of the max number of lines (or buttons
 * in @ref BUTTON_INFO style) of the sub text, and of the behavior when the sub text doesn't fit in
 * them
 *
 * @param layout the current layout
 * @param text main text, must be single line if subText
 * @param subText description under main text (may be null)
 * @param style if @ref REGULAR_INFO, use regular font for text, otherwise use bold font for text
 * @param nbMaxLines max number of lines (or buttons in @ref BUTTON_INFO style) for the sub text. If
 * 0, the max displayable number is used
 * @param ellipsis if true, the last line (or button) ends with an ellipsis if the sub text doesn't
 * fit, otherwise the remaining text is simply not displayed (it is displayed by the caller in
 * another page)
 * @return >= 0 if OK
 */
int layoutAddText(nbgl_layout_t                  *layout,
                  const char                     *text,
                  const char                     *subText,
                  nbgl_contentCenteredInfoStyle_t style,
                  uint8_t                         nbMaxLines,
                  bool                            ellipsis)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_container_t      *container;
    nbgl_text_area_t      *textArea;
    uint16_t               fullHeight = 0;
    // max number of buttons in BUTTON_INFO style, or of lines in other styles, for the sub text
    uint8_t  subTextMaxLines = 0;
    uint16_t subTextNbLines  = 0;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddText():\n");
    if (layout == NULL) {
        return -1;
    }
    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);

    // get container children: main text, then sub text
    container->nbChildren = 1;
    if (subText != NULL) {
        if (style == BUTTON_INFO) {
            subTextMaxLines = ((nbMaxLines > 0) && (nbMaxLines < NB_MAX_BUTTON_ROWS))
                                  ? nbMaxLines
                                  : NB_MAX_BUTTON_ROWS;
            // possibly subTextMaxLines buttons
            container->nbChildren += subTextMaxLines;
        }
        else {
            // at most NB_MAX_LINES-1 lines are available on screen, under the main text
            subTextMaxLines = ((nbMaxLines > 0) && (nbMaxLines < (NB_MAX_LINES - 1)))
                                  ? nbMaxLines
                                  : (NB_MAX_LINES - 1);
            subTextNbLines  = nbgl_getTextNbLinesInWidth(
                BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp, PIC(subText), AVAILABLE_WIDTH, true);
            // a text area, plus 2 others (last line and ellipsis) if the sub text is cut with an
            // ellipsis
            container->nbChildren += ((subTextNbLines > subTextMaxLines) && ellipsis) ? 3 : 1;
        }
    }

    container->children       = nbgl_containerPoolGet(container->nbChildren, layoutInt->layer);
    container->obj.area.width = AVAILABLE_WIDTH;

    textArea                 = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor      = WHITE;
    textArea->text           = PIC(text);
    textArea->textAlignment  = CENTER;
    textArea->fontId         = (style == REGULAR_INFO) ? BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp
                                                       : BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
    textArea->obj.area.width = AVAILABLE_WIDTH;

    uint16_t nbLines
        = nbgl_getTextNbLinesInWidth(textArea->fontId, textArea->text, AVAILABLE_WIDTH, true);
    // if more than available lines on screen
    if (nbLines > NB_MAX_LINES) {
        uint16_t len;

        nbLines              = NB_MAX_LINES;
        textArea->nbMaxLines = NB_MAX_LINES;
        nbgl_getTextMaxLenInNbLines(
            textArea->fontId, textArea->text, AVAILABLE_WIDTH, nbLines, &len, true);
        textArea->len = len;
    }
    const nbgl_font_t *font   = nbgl_getFont(textArea->fontId);
    textArea->obj.area.height = nbLines * font->line_height;
    textArea->wrapping        = true;
    textArea->obj.alignment   = TOP_MIDDLE;
    fullHeight += textArea->obj.area.height;
    container->children[0] = (nbgl_obj_t *) textArea;

    if (subText != NULL) {
        if (style != BUTTON_INFO) {
            textArea            = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
            textArea->textColor = WHITE;
            textArea->text      = PIC(subText);
            textArea->wrapping  = true;
            textArea->fontId    = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
            textArea->obj.area.width = AVAILABLE_WIDTH;
            nbLines                  = subTextNbLines;
            // if more than available lines
            bool truncated = (nbLines > subTextMaxLines);
            if (truncated) {
                uint16_t len;
                nbLines              = subTextMaxLines;
                textArea->nbMaxLines = nbLines;
                nbgl_getTextMaxLenInNbLines(
                    textArea->fontId, textArea->text, AVAILABLE_WIDTH, nbLines, &len, true);
                textArea->len = len;
            }
            if (style == REGULAR_INFO) {
                textArea->obj.area.height = nbLines * font->line_height;
            }
            else {
                // the sub text must be vertically centered in a 3 lines container
                textArea->obj.area.height = 3 * font->line_height;
            }
            textArea->textAlignment        = CENTER;
            textArea->obj.alignment        = NO_ALIGNMENT;
            textArea->obj.alignmentMarginY = 2;
            fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;
            container->children[1] = (nbgl_obj_t *) textArea;
            if (truncated && ellipsis) {
                // the end of the last line is replaced by an ellipsis
                addTextWithEllipsis(
                    layoutInt, container, textArea, nbLines, textArea->obj.area.height);
            }
        }
        else {
            // vertical margin of each button, depending on the number of buttons
            static const int16_t marginsY[NB_MAX_BUTTON_ROWS][NB_MAX_BUTTON_ROWS] = {
                {8},
                {8 - 7, 8 + 7},
                {-8, 6, 20}
            };
            // the texts of the buttons are copied in the layout, so that they stay valid as long
            // as the layout is displayed, whatever the given subText becomes
            char(*buttonTexts)[NB_MAX_CHAR_IN_BUTTON_ROW] = layoutInt->buttonTexts;
            nbgl_button_t *buttons[NB_MAX_BUTTON_ROWS];
            const char    *rowText = (const char *) PIC(subText);
            uint8_t        nbRows  = 0;
            uint8_t        maxRows = subTextMaxLines;

            // draw buttons under each other, each with the text remaining from the previous one
            while ((rowText != NULL) && (nbRows < maxRows)) {
                nbgl_button_t *button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layoutInt->layer);
                uint16_t       len    = 0;
                uint16_t       width  = 0;
                uint16_t       consumed;

                button->foregroundColor = BLACK;
                button->innerColor      = WHITE;
                button->borderColor     = WHITE;
                button->radius          = RADIUS_3_PIXELS;
                button->text            = rowText;
                button->fontId          = BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
                button->obj.area.height = 14;
                button->obj.alignment   = CENTER;

                consumed = layoutGetButtonRowLen(rowText, &len, &width);
                if (ellipsis && (rowText[consumed] != '\0') && (nbRows == (maxRows - 1))) {
                    // text remaining after the last button: end it with an ellipsis
                    const uint16_t ellipsisWidth = nbgl_getTextWidth(button->fontId, ELLIPSIS);
                    getButtonTextLen(rowText,
                                     AVAILABLE_WIDTH - BUTTON_MARGIN_Y - ellipsisWidth,
                                     NB_MAX_CHAR_IN_BUTTON_ROW - ELLIPSIS_SIZE,
                                     &len,
                                     &width);
                    button->obj.area.width = width + ellipsisWidth + BUTTON_MARGIN_Y;
                    // copy the first 'len' chars then append ELLIPSIS (ELLIPSIS_SIZE includes '\0')
                    // (len is already limited by getButtonTextLen(), MIN() makes it explicit)
                    uint16_t copyLen = MIN(len, (NB_MAX_CHAR_IN_BUTTON_ROW - ELLIPSIS_SIZE));
                    memcpy(buttonTexts[nbRows], rowText, copyLen);
                    memcpy(buttonTexts[nbRows] + copyLen, ELLIPSIS, ELLIPSIS_SIZE);
                    button->text = PIC(buttonTexts[nbRows]);
                    rowText      = NULL;
                }
                else {
                    button->obj.area.width = width + BUTTON_MARGIN_Y;
                    // copy the 'len' chars displayed in this button
                    // (len is already limited by layoutGetButtonRowLen(), MIN() makes it explicit)
                    uint16_t copyLen = MIN(len, (NB_MAX_CHAR_IN_BUTTON_ROW - 1));
                    memcpy(buttonTexts[nbRows], rowText, copyLen);
                    // NULL termination
                    buttonTexts[nbRows][copyLen] = '\0';
                    button->text                 = PIC(buttonTexts[nbRows]);
                    // the remaining text goes in the next button
                    rowText = (rowText[consumed] != '\0') ? (rowText + consumed) : NULL;
                }
                buttons[nbRows]                 = button;
                container->children[1 + nbRows] = (nbgl_obj_t *) button;
                nbRows++;
            }
            for (uint8_t i = 0; i < nbRows; i++) {
                buttons[i]->obj.alignmentMarginY = marginsY[nbRows - 1][i];
            }

            fullHeight += (nbRows == NB_MAX_BUTTON_ROWS) ? 48 : 44;
        }
    }
    container->obj.area.height = fullHeight;
    container->layout          = VERTICAL;
    container->obj.alignment   = CENTER;
    // set this new obj as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

    return 0;
}

/**
 * @brief Creates a menu list (only for nanos) with the given parameters. The navigation (and
 * selection) must be handled by the caller
 *
 * @param layout the current layout
 * @param list structure giving the list of choices and the current selected one
 * @return >= 0 if OK
 */
int nbgl_layoutAddMenuList(nbgl_layout_t *layout, nbgl_layoutMenuList_t *list)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    uint8_t                i;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddMenuList():\n");
    if (layout == NULL) {
        return -1;
    }
    for (i = 0; i < list->nbChoices; i++) {
        nbgl_text_area_t *textArea;

        // check whether this object is visible or not
        // only the two objects above or below the selected one are visible
        if (((list->selectedChoice > 2) && (i < (list->selectedChoice - 2)))
            || (i > (list->selectedChoice + 2))) {
            continue;
        }

        textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);

        // init text area for this choice
        textArea->text                 = list->callback(i);
        textArea->textAlignment        = CENTER;
        textArea->obj.area.width       = AVAILABLE_WIDTH;
        textArea->obj.area.height      = 16;
        textArea->style                = NO_STYLE;
        textArea->obj.alignment        = CENTER;
        textArea->obj.alignmentMarginY = ((i - list->selectedChoice) * 16);
        textArea->textColor            = WHITE;

        // highlight init choice
        if (i == list->selectedChoice) {
            textArea->fontId = BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
        }
        else {
            textArea->fontId = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
        }

        // set this new obj as child of main container
        layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);
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
    nbgl_text_area_t      *textArea   = NULL;
    nbgl_image_t          *image      = NULL;
    uint16_t               fullHeight = 0;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddCenteredInfo():\n");
    if (layout == NULL) {
        return -1;
    }

    // When the caller asks for the icon to be at the screen bottom (and a
    // primary text is provided), render the texts via nbgl_layoutAddText so
    // the exact same geometry is used as on non-bottomIcon pages (sibling
    // CHOICES_LIST entries reach the layout through nbgl_stepDrawText, which
    // also calls nbgl_layoutAddText). The icon is then appended as a
    // standalone BOTTOM_MIDDLE element. Falling back to the regular path
    // when text1 is NULL keeps nbgl_layoutAddText's "main text mandatory"
    // contract.
    // Note: nbgl_layoutAddText always centers its container, so this branch
    // ignores info->onTop — the combination bottomIcon=true && onTop=true is
    // documented as incompatible on nbgl_contentCenteredInfo_t::bottomIcon.
    if ((info->icon != NULL) && info->bottomIcon && (info->text1 != NULL)) {
        // bottomIcon pages put the meaningful content in text1 (the icon is
        // only a contextual hint at the bottom — selection marker, sub-menu
        // chevron, ...). Force BOLD_TEXT1_INFO when only text1 is set so the
        // primary content is visually prominent; when text2 is also set, keep
        // the caller's style (BOLD_TEXT1_INFO is already implied by the
        // standard drawStep style selection in that case).
        nbgl_contentCenteredInfoStyle_t style = info->style;
        if ((style == REGULAR_INFO) && (info->text2 == NULL)) {
            style = BOLD_TEXT1_INFO;
        }
        nbgl_layoutAddText(layout, info->text1, info->text2, style);
        image                  = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        image->foregroundColor = WHITE;
        image->buffer          = PIC(info->icon);
        image->obj.area.bpp    = NBGL_BPP_1;
        image->obj.alignment   = BOTTOM_MIDDLE;
        image->obj.alignTo     = NULL;
        layoutAddObject(layoutInt, (nbgl_obj_t *) image);
        return 0;
    }

    container = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, layoutInt->layer);

    // 3 children at max
    container->children   = nbgl_containerPoolGet(3, layoutInt->layer);
    container->nbChildren = 0;
    if (info->icon != NULL) {
        image                  = (nbgl_image_t *) nbgl_objPoolGet(IMAGE, layoutInt->layer);
        image->foregroundColor = WHITE;
        image->buffer          = PIC(info->icon);
        image->obj.area.bpp    = NBGL_BPP_1;
        image->obj.alignment   = TOP_MIDDLE;
        image->obj.alignTo     = NULL;

        fullHeight += image->buffer->height;
        container->children[container->nbChildren] = (nbgl_obj_t *) image;
        container->nbChildren++;
    }
    if (info->text1 != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = WHITE;
        textArea->text          = PIC(info->text1);
        textArea->textAlignment = CENTER;
        textArea->fontId = (info->style == REGULAR_INFO) ? BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp
                                                         : BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
        textArea->obj.area.width = AVAILABLE_WIDTH;
        textArea->wrapping       = true;
        uint16_t nbLines
            = nbgl_getTextNbLinesInWidth(textArea->fontId, textArea->text, AVAILABLE_WIDTH, true);
        // if more than available lines on screen
        if (nbLines > NB_MAX_LINES) {
            uint16_t len;
            nbLines              = NB_MAX_LINES;
            textArea->nbMaxLines = NB_MAX_LINES;
            nbgl_getTextMaxLenInNbLines(
                textArea->fontId, textArea->text, AVAILABLE_WIDTH, nbLines, &len, true);
            textArea->len = len;
        }
        const nbgl_font_t *font   = nbgl_getFont(textArea->fontId);
        textArea->obj.area.height = nbLines * font->line_height;
        textArea->style           = NO_STYLE;
        if (info->icon != NULL) {
            textArea->obj.alignment = BOTTOM_MIDDLE;  // under icon
            textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
            textArea->obj.alignmentMarginY = (nbLines < 3) ? 4 : 0;
        }
        else if (info->text2 == NULL) {
            textArea->obj.alignment = CENTER;
            textArea->obj.alignTo   = NULL;
        }
        else {
            textArea->obj.alignment = TOP_MIDDLE;
            textArea->obj.alignTo   = NULL;
        }

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    if (info->text2 != NULL) {
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = WHITE;
        textArea->text          = PIC(info->text2);
        textArea->textAlignment = CENTER;
        textArea->fontId        = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
        textArea->obj.area.width = AVAILABLE_WIDTH;
        textArea->wrapping       = true;
        uint16_t nbLines
            = nbgl_getTextNbLinesInWidth(textArea->fontId, textArea->text, AVAILABLE_WIDTH, true);
        // if more than available lines on screen
        if (nbLines > (NB_MAX_LINES - 1)) {
            uint16_t len;
            nbLines              = NB_MAX_LINES - 1;
            textArea->nbMaxLines = nbLines;
            nbgl_getTextMaxLenInNbLines(
                textArea->fontId, textArea->text, AVAILABLE_WIDTH, nbLines, &len, true);
            textArea->len = len;
        }
        const nbgl_font_t *font   = nbgl_getFont(textArea->fontId);
        textArea->obj.area.height = nbLines * font->line_height;

        textArea->style         = NO_STYLE;
        textArea->obj.alignment = BOTTOM_MIDDLE;
        textArea->obj.alignTo   = (nbgl_obj_t *) container->children[container->nbChildren - 1];
        textArea->obj.alignmentMarginY = 2;

        fullHeight += textArea->obj.area.height + textArea->obj.alignmentMarginY;

        container->children[container->nbChildren] = (nbgl_obj_t *) textArea;
        container->nbChildren++;
    }
    container->obj.area.height      = fullHeight;
    container->layout               = VERTICAL;
    container->obj.alignmentMarginY = 0;
    if (info->onTop) {
        container->obj.alignment = TOP_MIDDLE;
    }
    else {
        container->obj.alignment = CENTER;
    }

    container->obj.area.width = AVAILABLE_WIDTH;

    // set this new container as child of main container
    layoutAddObject(layoutInt, (nbgl_obj_t *) container);

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

        textArea                       = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA,
                                                        ((nbgl_layoutInternal_t *) layout)->layer);
        textArea->textColor            = WHITE;
        textArea->text                 = PIC(barLayout->text);
        textArea->textAlignment        = CENTER;
        textArea->fontId               = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
        textArea->obj.area.width       = AVAILABLE_WIDTH;
        textArea->obj.area.height      = nbgl_getTextHeight(textArea->fontId, textArea->text);
        textArea->obj.alignment        = TOP_MIDDLE;
        textArea->obj.alignmentMarginX = 0;
        textArea->obj.alignmentMarginY = 16;  // 16 px from top
        layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);
    }
    progress                       = (nbgl_progress_bar_t *) nbgl_objPoolGet(PROGRESS_BAR,
                                                       ((nbgl_layoutInternal_t *) layout)->layer);
    progress->foregroundColor      = WHITE;
    progress->withBorder           = true;
    progress->state                = barLayout->percentage;
    progress->obj.area.width       = 102;
    progress->obj.area.height      = 14;
    progress->obj.alignment        = TOP_MIDDLE;
    progress->obj.alignmentMarginX = 0;
    progress->obj.alignmentMarginY = 33;  // 33px from top
    layoutAddObject(layoutInt, (nbgl_obj_t *) progress);

    if (barLayout->subText != NULL) {
        nbgl_text_area_t *subTextArea;

        subTextArea = (nbgl_text_area_t *) nbgl_objPoolGet(
            TEXT_AREA, ((nbgl_layoutInternal_t *) layout)->layer);
        subTextArea->textColor       = WHITE;
        subTextArea->text            = PIC(barLayout->subText);
        subTextArea->textAlignment   = CENTER;
        subTextArea->fontId          = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
        subTextArea->obj.area.width  = AVAILABLE_WIDTH;
        subTextArea->obj.area.height = nbgl_getTextHeight(subTextArea->fontId, subTextArea->text);
        subTextArea->obj.alignment   = BOTTOM_MIDDLE;
        subTextArea->obj.alignTo     = (nbgl_obj_t *) progress;
        subTextArea->obj.alignmentMarginX = 0;
        subTextArea->obj.alignmentMarginY = 4;
        layoutAddObject(layoutInt, (nbgl_obj_t *) subTextArea);
    }

    return 0;
}

/**
 * @brief Creates an area in main panel to display a button, with the given style
 *
 * @param layout the current layout
 * @param buttonInfo structure giving the description of button
 * @return >= 0 if OK
 */
int nbgl_layoutAddButton(nbgl_layout_t *layout, const nbgl_layoutButton_t *buttonInfo)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_button_t         *button;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddButton():\n");
    if (layout == NULL) {
        return -1;
    }
    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, ((nbgl_layoutInternal_t *) layout)->layer);
    button->foregroundColor = BLACK;
    button->innerColor      = WHITE;
    button->borderColor     = WHITE;
    button->radius          = RADIUS_3_PIXELS;
    button->text            = (const char *) PIC(buttonInfo->text);
    button->fontId          = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
    button->icon            = (const nbgl_icon_details_t *) PIC(buttonInfo->icon);
    button->obj.area.width  = nbgl_getTextWidth(button->fontId, button->text) + BUTTON_MARGIN_Y;
    button->obj.area.height = 14;
    layoutAddObject(layoutInt, (nbgl_obj_t *) button);

    return 0;
}

/**
 * @brief Creates an area in main panel to display a switch
 *
 * @param layout the current layout
 * @param switchLayout structure giving the description of switch
 * @return >= 0 if OK
 */
int nbgl_layoutAddSwitch(nbgl_layout_t *layout, const nbgl_layoutSwitch_t *switchLayout)
{
    nbgl_layoutInternal_t *layoutInt = (nbgl_layoutInternal_t *) layout;
    nbgl_button_t         *button;
    nbgl_text_area_t      *textArea;

    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutAddSwitch():\n");
    if (layout == NULL) {
        return -1;
    }
    // add switch name as title
    textArea                 = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
    textArea->textColor      = WHITE;
    textArea->text           = PIC(switchLayout->text);
    textArea->textAlignment  = CENTER;
    textArea->fontId         = BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
    textArea->obj.area.width = AVAILABLE_WIDTH;
    textArea->wrapping       = true;
    uint16_t nbLines
        = nbgl_getTextNbLinesInWidth(textArea->fontId, textArea->text, AVAILABLE_WIDTH, true);
    // if more than 1 line on screen
    if (nbLines > 1) {
        LOG_WARN(
            LAYOUT_LOGGER, "nbgl_layoutAddSwitch(): item text too long (%d lines > 1)\n", nbLines);
        return -1;
    }
    textArea->obj.area.height      = nbgl_getFontLineHeight(textArea->fontId);
    textArea->obj.alignment        = TOP_MIDDLE;
    textArea->obj.alignmentMarginY = 3;
    layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);

    if (switchLayout->subText != NULL) {
        // add switch description
        textArea                = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layoutInt->layer);
        textArea->textColor     = WHITE;
        textArea->text          = PIC(switchLayout->subText);
        textArea->textAlignment = CENTER;
        textArea->fontId        = BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp;
        textArea->obj.area.width  = AVAILABLE_WIDTH;
        textArea->obj.area.height = 2 * nbgl_getFontLineHeight(textArea->fontId);
        textArea->wrapping        = true;
        nbLines
            = nbgl_getTextNbLinesInWidth(textArea->fontId, textArea->text, AVAILABLE_WIDTH, true);
        // if more than 2 lines on screen
        if (nbLines > 2) {
            LOG_WARN(LAYOUT_LOGGER,
                     "nbgl_layoutAddSwitch(): sub text too long (%d lines > 2)\n",
                     nbLines);
            return -1;
        }
        textArea->obj.alignment        = CENTER;
        textArea->obj.alignmentMarginY = 1;  // not exactly centered
        layoutAddObject(layoutInt, (nbgl_obj_t *) textArea);
    }

    button = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, ((nbgl_layoutInternal_t *) layout)->layer);
    button->foregroundColor = BLACK;
    button->innerColor      = WHITE;
    button->borderColor     = WHITE;
    button->radius          = RADIUS_3_PIXELS;
    button->text            = (switchLayout->initState == ON_STATE) ? "Enabled" : "Disabled";
    button->fontId          = BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp;
    button->icon = (switchLayout->initState == ON_STATE) ? &C_Switch_On_8px : &C_Switch_Off_8px;
    // 2 pixels between icon & text, and 4 pixels on each side
    button->obj.area.width
        = nbgl_getTextWidth(button->fontId, button->text) + 2 + C_Switch_Off_8px.width + 8;
    button->obj.area.height      = 12;
    button->obj.alignment        = BOTTOM_MIDDLE;
    button->obj.alignmentMarginY = 3;
    layoutAddObject(layoutInt, (nbgl_obj_t *) button);

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
        LOG_WARN(LAYOUT_LOGGER, "nbgl_layoutDraw(): layout is NULL\n");
        return -1;
    }
    LOG_DEBUG(LAYOUT_LOGGER, "nbgl_layoutDraw(): layout->nbChildren = %d\n", layout->nbChildren);
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
