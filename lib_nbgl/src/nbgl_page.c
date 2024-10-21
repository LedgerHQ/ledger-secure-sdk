/**
 * @file nbgl_page.c
 * @brief Implementation of predefined pages management for Applications
 */

#ifdef NBGL_PAGE
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include "nbgl_debug.h"
#include "nbgl_page.h"
#include "glyphs.h"
#include "os_pic.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void addEmptyHeader(nbgl_layout_t *layout, uint16_t height)
{
    nbgl_layoutHeader_t headerDesc
        = {.type = HEADER_EMPTY, .separationLine = false, .emptySpace.height = height};
    nbgl_layoutAddHeader(layout, &headerDesc);
}

static void addContent(nbgl_pageContent_t *content,
                       nbgl_layout_t      *layout,
                       uint16_t            availableHeight,
                       bool                headerAdded)
{
    if (content->title != NULL) {
        nbgl_layoutHeader_t headerDesc = {.type               = HEADER_BACK_AND_TEXT,
                                          .separationLine     = true,
                                          .backAndText.token  = content->titleToken,
                                          .backAndText.tuneId = content->tuneId,
                                          .backAndText.text   = content->title};
        nbgl_layoutAddHeader(layout, &headerDesc);
        headerAdded = true;
    }
    if (content->topRightIcon != NULL) {
        nbgl_layoutAddTopRightButton(
            layout, content->topRightIcon, content->topRightToken, content->tuneId);
    }
    switch (content->type) {
        case INFO_LONG_PRESS: {
            nbgl_contentCenter_t centeredInfo;
            centeredInfo.icon        = content->infoLongPress.icon;
            centeredInfo.title       = content->infoLongPress.text;
            centeredInfo.smallTitle  = NULL;
            centeredInfo.description = NULL;
            centeredInfo.iconHug     = 0;
            centeredInfo.subText     = NULL;
            centeredInfo.padding     = false;
            nbgl_layoutAddContentCenter(layout, &centeredInfo);
            nbgl_layoutAddLongPressButton(layout,
                                          content->infoLongPress.longPressText,
                                          content->infoLongPress.longPressToken,
                                          content->infoLongPress.tuneId);
            break;
        }
        case INFO_BUTTON: {
            nbgl_contentCenter_t centeredInfo;
            nbgl_layoutButton_t  buttonInfo;

            centeredInfo.icon        = content->infoButton.icon;
            centeredInfo.title       = content->infoButton.text;
            centeredInfo.smallTitle  = NULL;
            centeredInfo.description = NULL;
            centeredInfo.iconHug     = 0;
            centeredInfo.subText     = NULL;
            centeredInfo.padding     = false;
            nbgl_layoutAddContentCenter(layout, &centeredInfo);

            buttonInfo.fittingContent = false;
            buttonInfo.icon           = NULL;
            buttonInfo.onBottom       = true;
            buttonInfo.style          = BLACK_BACKGROUND;
            buttonInfo.text           = content->infoButton.buttonText;
            buttonInfo.token          = content->infoButton.buttonToken;
            buttonInfo.tuneId         = content->infoButton.tuneId;
            nbgl_layoutAddButton(layout, &buttonInfo);
            break;
        }
        case CENTERED_INFO:
            if (!headerAdded) {
                addEmptyHeader(layout, SMALL_CENTERING_HEADER);
            }
            nbgl_layoutAddCenteredInfo(layout, &content->centeredInfo);
            break;

        case EXTENDED_CENTER:
            if ((!headerAdded) && (content->extendedCenter.tipBox.text == NULL)) {
                addEmptyHeader(layout, SMALL_CENTERING_HEADER);
            }
            nbgl_layoutAddContentCenter(layout, &content->extendedCenter.contentCenter);
            if (content->extendedCenter.tipBox.text != NULL) {
                nbgl_layoutUpFooter_t upFooterDesc
                    = {.type          = UP_FOOTER_TIP_BOX,
                       .tipBox.text   = content->extendedCenter.tipBox.text,
                       .tipBox.icon   = content->extendedCenter.tipBox.icon,
                       .tipBox.token  = content->extendedCenter.tipBox.token,
                       .tipBox.tuneId = content->extendedCenter.tipBox.tuneId};
                nbgl_layoutAddUpFooter(layout, &upFooterDesc);
            }
            break;

        case TAG_VALUE_LIST:
            // add a space of 32/40px if no header already added
            if (!headerAdded) {
                addEmptyHeader(layout, SMALL_CENTERING_HEADER);
            }
            nbgl_layoutAddTagValueList(layout, &content->tagValueList);
            break;
        case TAG_VALUE_DETAILS: {
            // add a space of 32/40px if no header already added
            if (!headerAdded) {
                addEmptyHeader(layout, SMALL_CENTERING_HEADER);
            }
            // display a button under tag/value
            nbgl_layoutButton_t buttonInfo;
            content->tagValueDetails.tagValueList.nbMaxLinesForValue -= 3;
            nbgl_layoutAddTagValueList(layout, &content->tagValueDetails.tagValueList);
            buttonInfo.fittingContent = true;
            buttonInfo.icon           = content->tagValueDetails.detailsButtonIcon;
            buttonInfo.style          = WHITE_BACKGROUND;
            buttonInfo.text           = content->tagValueDetails.detailsButtonText;
            buttonInfo.token          = content->tagValueDetails.detailsButtonToken;
            buttonInfo.tuneId         = content->tagValueDetails.tuneId;
            buttonInfo.onBottom       = false;
            nbgl_layoutAddButton(layout, &buttonInfo);
            break;
        }
        case TAG_VALUE_CONFIRM: {
            nbgl_layoutButton_t buttonInfo;
            if (!headerAdded) {
                addEmptyHeader(layout, SMALL_CENTERING_HEADER);
            }
            nbgl_layoutAddTagValueList(layout, &content->tagValueConfirm.tagValueList);
            // Always display the details button as a normal button (full width),
            // even if "Confirm" button is on the same page
            if (content->tagValueConfirm.detailsButtonText != NULL) {
                buttonInfo.fittingContent = true;
                buttonInfo.icon           = content->tagValueConfirm.detailsButtonIcon;
                buttonInfo.style          = WHITE_BACKGROUND;
                buttonInfo.text           = content->tagValueConfirm.detailsButtonText;
                buttonInfo.token          = content->tagValueConfirm.detailsButtonToken;
                buttonInfo.tuneId         = content->tagValueConfirm.tuneId;
                buttonInfo.onBottom       = false;
                nbgl_layoutAddButton(layout, &buttonInfo);
            }
            else if ((content->tagValueConfirm.detailsButtonIcon != NULL)
                     && (content->tagValueConfirm.confirmationText != NULL)) {
                // On Flex, a small button with only the icon is displayed on the left
                // of "Confirm"
                nbgl_layoutHorizontalButtons_t choice
                    = {.leftIcon   = content->tagValueConfirm.detailsButtonIcon,
                       .rightText  = content->tagValueConfirm.confirmationText,
                       .leftToken  = content->tagValueConfirm.detailsButtonToken,
                       .rightToken = content->tagValueConfirm.confirmationToken,
                       .tuneId     = content->tagValueConfirm.tuneId};
                nbgl_layoutAddHorizontalButtons(layout, &choice);
                break;
            }
            if (content->tagValueConfirm.confirmationText != NULL) {
                buttonInfo.fittingContent = false;
                buttonInfo.icon           = NULL;
                buttonInfo.style          = BLACK_BACKGROUND;
                buttonInfo.text           = content->tagValueConfirm.confirmationText;
                buttonInfo.token          = content->tagValueConfirm.confirmationToken;
                buttonInfo.tuneId         = content->tagValueConfirm.tuneId;
                buttonInfo.onBottom       = true;
                nbgl_layoutAddButton(layout, &buttonInfo);
            }
            break;
        }
        case SWITCHES_LIST: {
            uint8_t i;
            for (i = 0; i < content->switchesList.nbSwitches; i++) {
                availableHeight -= nbgl_layoutAddSwitch(layout, &content->switchesList.switches[i]);
                // do not draw a separation line if too low in the container
                if (availableHeight > 10) {
                    nbgl_layoutAddSeparationLine(layout);
                }
            }
            break;
        }
        case INFOS_LIST: {
            uint8_t i;
            for (i = 0; i < content->infosList.nbInfos; i++) {
                // if the extension is valid for this index, use a Text with Alias
                if ((content->infosList.withExtensions == true)
                    && (content->infosList.infoExtensions != NULL)
                    && (content->infosList.infoExtensions[i].fullValue != NULL)) {
                    availableHeight
                        -= nbgl_layoutAddTextWithAlias(layout,
                                                       content->infosList.infoTypes[i],
                                                       content->infosList.infoContents[i],
                                                       content->infosList.token,
                                                       i);
                }
                else {
                    availableHeight -= nbgl_layoutAddText(layout,
                                                          content->infosList.infoTypes[i],
                                                          content->infosList.infoContents[i]);
                }
                // do not draw a separation line if too low in the container
                if (availableHeight > 10) {
                    nbgl_layoutAddSeparationLine(layout);
                }
            }
            break;
        }
        case CHOICES_LIST:
            nbgl_layoutAddRadioChoice(layout, &content->choicesList);
            break;
        case BARS_LIST: {
            uint8_t i;
            for (i = 0; i < content->barsList.nbBars; i++) {
                nbgl_layoutBar_t bar;
                bar.text      = content->barsList.barTexts[i];
                bar.subText   = NULL;
                bar.iconRight = &PUSH_ICON;
                bar.iconLeft  = NULL;
                bar.token     = content->barsList.tokens[i];
                bar.tuneId    = content->barsList.tuneId;
                bar.large     = false;
                bar.inactive  = false;
                availableHeight -= nbgl_layoutAddTouchableBar(layout, &bar);
                // do not draw a separation line if too low in the container
                if (availableHeight > 10) {
                    nbgl_layoutAddSeparationLine(layout);
                }
            }
            break;
        }
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief draw a page with a centered text in large case, with a round check icon
 *
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param text text in LEDGER style (UPPER CASE only)
 * @param tapActionToken token passed to onActionCallback, on a tap event
 * @return the page context (or NULL if error)
 */
nbgl_page_t *nbgl_pageDrawLedgerInfo(nbgl_layoutTouchCallback_t              onActionCallback,
                                     const nbgl_screenTickerConfiguration_t *ticker,
                                     const char                             *text,
                                     int                                     tapActionToken)
{
    nbgl_layoutDescription_t  layoutDescription;
    nbgl_layout_t            *layout;
    nbgl_layoutCenteredInfo_t centeredInfo = {.text1   = text,
                                              .text2   = NULL,
                                              .text3   = NULL,
                                              .style   = LARGE_CASE_INFO,
                                              .icon    = &C_Check_Circle_64px,
                                              .offsetY = 0};

    layoutDescription.modal          = false;
    layoutDescription.withLeftBorder = true;

    layoutDescription.onActionCallback = onActionCallback;
    layoutDescription.tapActionText    = "";
    layoutDescription.tapActionToken   = tapActionToken;
    layoutDescription.tapTuneId        = TUNE_TAP_CASUAL;

    layoutDescription.ticker.tickerCallback  = ticker->tickerCallback;
    layoutDescription.ticker.tickerIntervale = ticker->tickerIntervale;
    layoutDescription.ticker.tickerValue     = ticker->tickerValue;
    layout                                   = nbgl_layoutGet(&layoutDescription);

    addEmptyHeader(layout, SIMPLE_FOOTER_HEIGHT);
    nbgl_layoutAddCenteredInfo(layout, &centeredInfo);

    nbgl_layoutDraw(layout);

    return (nbgl_page_t *) layout;
}

/**
 * @brief draw a spinner page with the given parameters. The spinner will "rotate" automatically
 * every 800 ms
 *
 * @param onActionCallback common callback for all actions on this page (unused, so set to NULL)
 * @param text text to use under spinner
 * @return the page context (or NULL if error)
 */
nbgl_page_t *nbgl_pageDrawSpinner(nbgl_layoutTouchCallback_t onActionCallback, const char *text)
{
    nbgl_layoutDescription_t layoutDescription;
    nbgl_layout_t           *layout;

    layoutDescription.modal          = false;
    layoutDescription.withLeftBorder = true;

    layoutDescription.onActionCallback = onActionCallback;
    layoutDescription.tapActionText    = NULL;

    layoutDescription.ticker.tickerCallback = NULL;
    layout                                  = nbgl_layoutGet(&layoutDescription);

    nbgl_layoutAddSpinner(layout, text, false);

    nbgl_layoutDraw(layout);

    return (nbgl_page_t *) layout;
}

/**
 * @brief draw a page with a centered info (icon and/or texts) with a touchable footer,
 * in a potential "tapable" area, with an optional top-right button, with an optional bottom button
 *
 * @param onActionCallback common callback for all actions on this page
 * @param ticker ticker configuration, set to NULL to disable it
 * @param info structure describing the centered info and other controls of this page
 * @return the page context (or NULL if error)
 */
nbgl_page_t *nbgl_pageDrawInfo(nbgl_layoutTouchCallback_t              onActionCallback,
                               const nbgl_screenTickerConfiguration_t *ticker,
                               const nbgl_pageInfoDescription_t       *info)
{
    nbgl_layoutDescription_t layoutDescription = {0};
    nbgl_layout_t           *layout;

    layoutDescription.modal          = false;
    layoutDescription.withLeftBorder = true;

    layoutDescription.onActionCallback = onActionCallback;
    if (!info->isSwipeable) {
        layoutDescription.tapActionText  = info->tapActionText;
        layoutDescription.tapActionToken = info->tapActionToken;
        layoutDescription.tapTuneId      = info->tuneId;
    }

    if (ticker != NULL) {
        layoutDescription.ticker.tickerCallback  = ticker->tickerCallback;
        layoutDescription.ticker.tickerIntervale = ticker->tickerIntervale;
        layoutDescription.ticker.tickerValue     = ticker->tickerValue;
    }
    else {
        layoutDescription.ticker.tickerCallback = NULL;
    }
    layout = nbgl_layoutGet(&layoutDescription);
    if (info->isSwipeable) {
        nbgl_layoutAddSwipe(layout,
                            ((1 << SWIPED_LEFT) | (1 << SWIPED_RIGHT)),
                            info->tapActionText,
                            info->tapActionToken,
                            info->tuneId);
    }
    // add an empty header if a top-right button is used
    if (info->topRightStyle != NO_BUTTON_STYLE) {
        addEmptyHeader(layout, SMALL_CENTERING_HEADER);
    }
    nbgl_layoutAddCenteredInfo(layout, &info->centeredInfo);

    // if action button but not QUIT_APP_TEXT bottom button, use a small black button
    if ((info->actionButtonText != NULL) && (info->bottomButtonStyle != QUIT_APP_TEXT)) {
        nbgl_layoutButton_t buttonInfo = {.fittingContent = true,
                                          .icon           = info->actionButtonIcon,
                                          .onBottom       = false,
                                          .style          = info->actionButtonStyle,
                                          .text           = info->actionButtonText,
                                          .token          = info->bottomButtonsToken,
                                          .tuneId         = info->tuneId};
        nbgl_layoutAddButton(layout, &buttonInfo);
    }

    if (info->footerText != NULL) {
        nbgl_layoutAddFooter(layout, PIC(info->footerText), info->footerToken, info->tuneId);
    }
    if (info->topRightStyle != NO_BUTTON_STYLE) {
        const nbgl_icon_details_t *icon;
        if (info->topRightStyle == SETTINGS_ICON) {
            icon = &WHEEL_ICON;
        }
        else if (info->topRightStyle == INFO_ICON) {
            icon = &INFO_I_ICON;
        }
        else if (info->topRightStyle == QUIT_ICON) {
            icon = &CLOSE_ICON;
        }
        else {
            return NULL;
        }
        nbgl_layoutAddTopRightButton(layout, PIC(icon), info->topRightToken, info->tuneId);
    }
    if (info->bottomButtonStyle == QUIT_APP_TEXT) {
        // if action button and QUIT_APP_TEXT bottom button, use a pair of choice buttons
        if ((info->actionButtonText != NULL)) {
            nbgl_layoutChoiceButtons_t buttonsInfo = {.topText    = info->actionButtonText,
                                                      .bottomText = "Quit app",
                                                      .token      = info->bottomButtonsToken,
                                                      .style      = BOTH_ROUNDED_STYLE,
                                                      .tuneId     = info->tuneId,
                                                      .topIcon    = info->actionButtonIcon};
            buttonsInfo.style                      = (info->actionButtonStyle == BLACK_BACKGROUND)
                                                         ? STRONG_ACTION_AND_FOOTER_STYLE
                                                         : SOFT_ACTION_AND_FOOTER_STYLE;
            nbgl_layoutAddChoiceButtons(layout, &buttonsInfo);
        }
        else {
            nbgl_layoutAddFooter(layout, "Quit app", info->bottomButtonsToken, info->tuneId);
        }
    }
    else if (info->bottomButtonStyle != NO_BUTTON_STYLE) {
        const nbgl_icon_details_t *icon;
        if (info->bottomButtonStyle == SETTINGS_ICON) {
            icon = &WHEEL_ICON;
        }
        else if (info->bottomButtonStyle == INFO_ICON) {
            icon = &INFO_I_ICON;
        }
        else if (info->bottomButtonStyle == QUIT_ICON) {
            icon = &CLOSE_ICON;
        }
        else {
            return NULL;
        }
        nbgl_layoutAddBottomButton(
            layout, PIC(icon), info->bottomButtonsToken, false, info->tuneId);
    }
    nbgl_layoutDraw(layout);

    return (nbgl_page_t *) layout;
}

/**
 * @brief draw a confirmation page, with a centered info (icon and/or text), a button to confirm and
 * a footer to cancel
 *
 * @param onActionCallback common callback for all actions on this page
 * @param info structure describing the centered info and other controls of this page
 * @return the page context (or NULL if error)
 */
nbgl_page_t *nbgl_pageDrawConfirmation(nbgl_layoutTouchCallback_t                onActionCallback,
                                       const nbgl_pageConfirmationDescription_t *info)
{
    nbgl_layoutDescription_t   layoutDescription;
    nbgl_layout_t             *layout;
    nbgl_layoutChoiceButtons_t buttonsInfo
        = {.bottomText = (info->cancelText != NULL) ? PIC(info->cancelText) : "Cancel",
           .token      = info->confirmationToken,
           .topText    = PIC(info->confirmationText),
           .style      = ROUNDED_AND_FOOTER_STYLE,
           .tuneId     = info->tuneId};

    layoutDescription.modal          = info->modal;
    layoutDescription.withLeftBorder = true;

    layoutDescription.onActionCallback = onActionCallback;
    layoutDescription.tapActionText    = NULL;

    layoutDescription.ticker.tickerCallback = NULL;
    layout                                  = nbgl_layoutGet(&layoutDescription);

    addEmptyHeader(layout, MEDIUM_CENTERING_HEADER);
    nbgl_layoutAddChoiceButtons(layout, &buttonsInfo);

    nbgl_layoutAddCenteredInfo(layout, &info->centeredInfo);

    nbgl_layoutDraw(layout);

    return (nbgl_page_t *) layout;
}

/**
 * @brief draw a generic content page, with the given content, and if nav parameter is not NULL,
 * with the given navigation controls (either with navigation bar or with "tap" and "back")
 *
 * @param onActionCallback common callback for all actions on this page
 * @param nav structure describing the navigation controls of this page (no navigation if NULL)
 * @param content structure describing the main content of this page
 * @param modal set to true to draw as a modal
 * @return the page context (or NULL if error)
 */
nbgl_page_t *nbgl_pageDrawGenericContentExt(nbgl_layoutTouchCallback_t       onActionCallback,
                                            const nbgl_pageNavigationInfo_t *nav,
                                            nbgl_pageContent_t              *content,
                                            bool                             modal)
{
    nbgl_layoutDescription_t layoutDescription;
    nbgl_layout_t           *layout;
    uint16_t                 availableHeight = SCREEN_HEIGHT;
    bool                     headerAdded     = false;

    layoutDescription.modal                 = modal;
    layoutDescription.withLeftBorder        = true;
    layoutDescription.onActionCallback      = onActionCallback;
    layoutDescription.ticker.tickerCallback = NULL;

    if ((nav != NULL) && (nav->navType == NAV_WITH_TAP)) {
        layoutDescription.tapActionText  = nav->navWithTap.nextPageText;
        layoutDescription.tapActionToken = nav->navWithTap.nextPageToken;
        layoutDescription.tapTuneId      = nav->tuneId;
    }
    else {
        layoutDescription.tapActionText = NULL;
    }

    layout = nbgl_layoutGet(&layoutDescription);
    if (nav != NULL) {
        if (nav->navType == NAV_WITH_TAP) {
            if (nav->skipText == NULL) {
                availableHeight -= nbgl_layoutAddFooter(
                    layout, nav->navWithTap.quitText, nav->quitToken, nav->tuneId);
            }
            else {
                availableHeight -= nbgl_layoutAddSplitFooter(layout,
                                                             nav->navWithTap.quitText,
                                                             nav->quitToken,
                                                             nav->skipText,
                                                             nav->skipToken,
                                                             nav->tuneId);
            }
            if (nav->progressIndicator) {
                availableHeight -= nbgl_layoutAddProgressIndicator(layout,
                                                                   nav->activePage,
                                                                   nav->nbPages,
                                                                   nav->navWithTap.backButton,
                                                                   nav->navWithTap.backToken,
                                                                   nav->tuneId);
                headerAdded = true;
            }
        }
        else if (nav->navType == NAV_WITH_BUTTONS) {
            nbgl_layoutFooter_t footerDesc;
            bool                drawFooter = true;

            if (nav->skipText != NULL) {
                nbgl_layoutHeader_t headerDesc = {.type             = HEADER_RIGHT_TEXT,
                                                  .separationLine   = false,
                                                  .rightText.text   = nav->skipText,
                                                  .rightText.token  = nav->skipToken,
                                                  .rightText.tuneId = nav->tuneId};
                availableHeight -= nbgl_layoutAddHeader(layout, &headerDesc);
                headerAdded = true;
            }
            footerDesc.separationLine = true;
            if (nav->nbPages > 1) {
                if (nav->navWithButtons.quitText == NULL) {
                    footerDesc.type                         = FOOTER_NAV;
                    footerDesc.navigation.activePage        = nav->activePage;
                    footerDesc.navigation.nbPages           = nav->nbPages;
                    footerDesc.navigation.withExitKey       = nav->navWithButtons.quitButton;
                    footerDesc.navigation.withBackKey       = nav->navWithButtons.backButton;
                    footerDesc.navigation.withPageIndicator = false;
                    footerDesc.navigation.token             = nav->navWithButtons.navToken;
                    footerDesc.navigation.tuneId            = nav->tuneId;
                }
                else {
                    footerDesc.type                              = FOOTER_TEXT_AND_NAV;
                    footerDesc.textAndNav.text                   = nav->navWithButtons.quitText;
                    footerDesc.textAndNav.tuneId                 = nav->tuneId;
                    footerDesc.textAndNav.token                  = nav->quitToken;
                    footerDesc.textAndNav.navigation.activePage  = nav->activePage;
                    footerDesc.textAndNav.navigation.nbPages     = nav->nbPages;
                    footerDesc.textAndNav.navigation.withExitKey = false;
                    footerDesc.textAndNav.navigation.withBackKey = nav->navWithButtons.backButton;
                    footerDesc.textAndNav.navigation.visibleIndicator
                        = nav->navWithButtons.visiblePageIndicator;
                    footerDesc.textAndNav.navigation.withPageIndicator = true;
                    footerDesc.textAndNav.navigation.token  = nav->navWithButtons.navToken;
                    footerDesc.textAndNav.navigation.tuneId = nav->tuneId;
                }
            }
            else if (nav->navWithButtons.quitText != NULL) {
                // simple footer
                footerDesc.type                = FOOTER_SIMPLE_TEXT;
                footerDesc.simpleText.text     = nav->navWithButtons.quitText;
                footerDesc.simpleText.mutedOut = false;
                footerDesc.simpleText.token    = nav->quitToken;
                footerDesc.simpleText.tuneId   = nav->tuneId;
            }
            else {
                drawFooter = false;
            }
            if (drawFooter) {
                availableHeight -= nbgl_layoutAddExtendedFooter(layout, &footerDesc);
            }

#ifdef TARGET_STAX
            if (nav->progressIndicator) {
                availableHeight -= nbgl_layoutAddProgressIndicator(
                    layout, nav->activePage, nav->nbPages, false, 0, nav->tuneId);
                headerAdded = true;
            }
#endif  // TARGET_STAX
        }
    }
    addContent(content, layout, availableHeight, headerAdded);
    nbgl_layoutDraw(layout);

    return (nbgl_page_t *) layout;
}

/**
 * @brief draw a generic content page, with the given content, and if nav parameter is not NULL,
 * with the given navigation controls (either with navigation bar or with "tap" and "back")
 *
 * @param onActionCallback common callback for all actions on this page
 * @param nav structure describing the navigation controls of this page (no navigation if NULL)
 * @param content structure describing the main content of this page
 * @return the page context (or NULL if error)
 */
nbgl_page_t *nbgl_pageDrawGenericContent(nbgl_layoutTouchCallback_t       onActionCallback,
                                         const nbgl_pageNavigationInfo_t *nav,
                                         nbgl_pageContent_t              *content)
{
    return nbgl_pageDrawGenericContentExt(onActionCallback, nav, content, false);
}

/**
 * @brief Release the page obtained with any of the nbgl_pageDrawXXX() functions
 *
 * @param page page to release
 * @return >= 0 if OK
 */
int nbgl_pageRelease(nbgl_page_t *page)
{
    int ret;

    LOG_DEBUG(PAGE_LOGGER, "nbgl_pageRelease(): \n");
    ret = nbgl_layoutRelease((nbgl_layout_t *) page);

    return ret;
}
#endif  // NBGL_PAGE
