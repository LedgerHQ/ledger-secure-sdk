/**
 * @file nbgl_use_case_nanos.c
 * @brief Implementation of typical pages (or sets of pages) for Applications, for Nanos (X, SP)
 */

#ifdef NBGL_USE_CASE
#ifndef HAVE_SE_TOUCH
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdio.h>
#include "nbgl_debug.h"
#include "nbgl_use_case.h"
#include "glyphs.h"
#include "os_pic.h"
#include "os_helpers.h"
#include "ux.h"

/*********************
 *      DEFINES
 *********************/
#define WITH_HORIZONTAL_CHOICES_LIST
#define WITH_HORIZONTAL_BARS_LIST

/**********************
 *      TYPEDEFS
 **********************/

typedef struct ReviewContext_s {
    nbgl_choiceCallback_t             onChoice;
    const nbgl_contentTagValueList_t *tagValueList;
    const nbgl_icon_details_t        *icon;
    const char                       *reviewTitle;
    const char                       *reviewSubTitle;
    const char                       *address;  // for address confirmation review
} ReviewContext_t;

typedef struct ChoiceContext_s {
    const nbgl_icon_details_t *icon;
    const char                *message;
    const char                *subMessage;
    const char                *confirmText;
    const char                *cancelText;
    nbgl_choiceCallback_t      onChoice;
} ChoiceContext_t;

typedef struct ConfirmContext_s {
    const char     *message;
    const char     *subMessage;
    const char     *confirmText;
    const char     *cancelText;
    nbgl_callback_t onConfirm;
    nbgl_step_t     currentStep;
} ConfirmContext_t;

typedef struct ContentContext_s {
    const char                *title;  // For CHOICES_LIST /BARS_LIST
    nbgl_genericContents_t     genericContents;
    const char                *rejectText;
    nbgl_layoutTouchCallback_t controlsCallback;
    nbgl_callback_t            quitCallback;
} ContentContext_t;

typedef struct HomeContext_s {
    const char                   *appName;
    const nbgl_icon_details_t    *appIcon;
    const char                   *tagline;
    const nbgl_genericContents_t *settingContents;
    const nbgl_contentInfoList_t *infosList;
    const nbgl_homeAction_t      *homeAction;
    nbgl_callback_t               quitCallback;
} HomeContext_t;

typedef enum {
    NONE_USE_CASE,
    SPINNER_USE_CASE,
    REVIEW_USE_CASE,
    GENERIC_REVIEW_USE_CASE,
    REVIEW_BLIND_SIGN_USE_CASE,
    ADDRESS_REVIEW_USE_CASE,
    STREAMING_BLIND_SIGN_START_REVIEW_USE_CASE,
    STREAMING_START_REVIEW_USE_CASE,
    STREAMING_CONTINUE_REVIEW_USE_CASE,
    STREAMING_FINISH_REVIEW_USE_CASE,
    CHOICE_USE_CASE,
    STATUS_USE_CASE,
    CONFIRM_USE_CASE,
    HOME_USE_CASE,
    INFO_USE_CASE,
    SETTINGS_USE_CASE,
    GENERIC_SETTINGS,
    CONTENT_USE_CASE,
} ContextType_t;

typedef struct UseCaseContext_s {
    ContextType_t type;
    uint8_t       nbPages;
    int8_t        currentPage;
    nbgl_stepCallback_t
        stepCallback;  ///< if not NULL, function to be called on "double-key" action
    union {
        ReviewContext_t  review;
        ChoiceContext_t  choice;
        ConfirmContext_t confirm;
        HomeContext_t    home;
        ContentContext_t content;
    };
} UseCaseContext_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static UseCaseContext_t context;

/**********************
 *  STATIC FUNCTIONS
 **********************/
static void displayReviewPage(nbgl_stepPosition_t pos);
static void displayStreamingReviewPage(nbgl_stepPosition_t pos);
static void displayHomePage(nbgl_stepPosition_t pos);
static void displayInfoPage(nbgl_stepPosition_t pos);
static void displaySettingsPage(nbgl_stepPosition_t pos, bool toogle_state);
static void displayChoicePage(nbgl_stepPosition_t pos);
static void displayConfirm(nbgl_stepPosition_t pos);
static void displayContent(nbgl_stepPosition_t pos, bool toogle_state);
static void displaySpinner(const char *text);

static void startUseCaseHome(void);
static void startUseCaseInfo(void);
static void startUseCaseSettings(void);
static void startUseCaseSettingsAtPage(uint8_t initSettingPage);
static void startUseCaseContent(void);

static void statusTickerCallback(void);

// Simple helper to get the number of elements inside a nbgl_content_t
static uint8_t getContentNbElement(const nbgl_content_t *content)
{
    switch (content->type) {
        case CENTERED_INFO:
            return 1;
        case INFO_BUTTON:
            return 1;
        case TAG_VALUE_LIST:
            return content->content.tagValueList.nbPairs;
        case SWITCHES_LIST:
            return content->content.switchesList.nbSwitches;
        case INFOS_LIST:
            return content->content.infosList.nbInfos;
        case CHOICES_LIST:
            return content->content.choicesList.nbChoices;
        case BARS_LIST:
            return content->content.barsList.nbBars;
        default:
            return 0;
    }
}

// Helper to retrieve the content inside a nbgl_genericContents_t using
// either the contentsList or using the contentGetterCallback
static const nbgl_content_t *getContentAtIdx(const nbgl_genericContents_t *genericContents,
                                             int8_t                        contentIdx,
                                             nbgl_content_t               *content)
{
    if (contentIdx < 0 || contentIdx >= genericContents->nbContents) {
        LOG_DEBUG(USE_CASE_LOGGER, "No content available at %d\n", contentIdx);
        return NULL;
    }

    if (genericContents->callbackCallNeeded) {
        if (content == NULL) {
            LOG_DEBUG(USE_CASE_LOGGER, "Invalid content variable\n");
            return NULL;
        }
        // Retrieve content through callback, but first memset the content.
        memset(content, 0, sizeof(nbgl_content_t));
        genericContents->contentGetterCallback(contentIdx, content);
        return content;
    }
    else {
        // Retrieve content through list
        return PIC(&genericContents->contentsList[contentIdx]);
    }
}

// Helper to retrieve the content inside a nbgl_genericContents_t using
// either the contentsList or using the contentGetterCallback
static const nbgl_content_t *getContentElemAtIdx(uint8_t         elemIdx,
                                                 uint8_t        *elemContentIdx,
                                                 nbgl_content_t *content)
{
    const nbgl_genericContents_t *genericContents = NULL;
    const nbgl_content_t         *p_content       = NULL;
    uint8_t                       nbPages         = 0;
    uint8_t                       elemNbPages     = 0;

    switch (context.type) {
        case SETTINGS_USE_CASE:
        case HOME_USE_CASE:
        case GENERIC_SETTINGS:
            genericContents = context.home.settingContents;
            break;
        case CONTENT_USE_CASE:
        case GENERIC_REVIEW_USE_CASE:
            genericContents = &context.content.genericContents;
            break;
        default:
            return NULL;
    }
    for (int i = 0; i < genericContents->nbContents; i++) {
        p_content   = getContentAtIdx(genericContents, i, content);
        elemNbPages = getContentNbElement(p_content);
        if (nbPages + elemNbPages > elemIdx) {
            *elemContentIdx = context.currentPage - nbPages;
            break;
        }
        nbPages += elemNbPages;
    }

    return p_content;
}

static const char *getChoiceName(uint8_t choiceIndex)
{
    uint8_t                    elemIdx;
    uint8_t                    nbValues;
    const nbgl_content_t      *p_content      = NULL;
    nbgl_content_t             content        = {0};
    nbgl_contentRadioChoice_t *contentChoices = NULL;
    nbgl_contentBarsList_t    *contentBars    = NULL;
    char                     **names          = NULL;

    p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
    if (p_content == NULL) {
        return NULL;
    }
    switch (p_content->type) {
        case CHOICES_LIST:
            contentChoices = (nbgl_contentRadioChoice_t *) PIC(&p_content->content.choicesList);
            names          = (char **) PIC(contentChoices->names);
            nbValues       = contentChoices->nbChoices;
            break;
        case BARS_LIST:
            contentBars = ((nbgl_contentBarsList_t *) PIC(&p_content->content.barsList));
            names       = (char **) PIC(contentBars->barTexts);
            nbValues    = contentBars->nbBars;
            break;
        default:
            // Not supported as vertical MenuList
            return NULL;
    }
    if (choiceIndex >= nbValues) {
        // Last item is always "Back" button
        return "Back";
    }
    return (const char *) PIC(names[choiceIndex]);
}

static void onChoiceSelected(uint8_t choiceIndex)
{
    uint8_t                    elemIdx;
    uint8_t                    token          = 255;
    const nbgl_content_t      *p_content      = NULL;
    nbgl_content_t             content        = {0};
    nbgl_contentRadioChoice_t *contentChoices = NULL;
    nbgl_contentBarsList_t    *contentBars    = NULL;

    p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
    if (p_content == NULL) {
        return;
    }
    switch (p_content->type) {
        case CHOICES_LIST:
            contentChoices = (nbgl_contentRadioChoice_t *) PIC(&p_content->content.choicesList);
            if (choiceIndex < contentChoices->nbChoices) {
                token = contentChoices->token;
            }
            break;
        case BARS_LIST:
            contentBars = ((nbgl_contentBarsList_t *) PIC(&p_content->content.barsList));
            if (choiceIndex < contentBars->nbBars) {
                token = contentBars->tokens[choiceIndex];
            }
            break;
        default:
            // Not supported as vertical MenuList
            break;
    }
    if ((token != 255) && (context.content.controlsCallback != NULL)) {
        context.content.controlsCallback(token, 0);
    }
    else if (context.content.quitCallback != NULL) {
        context.content.quitCallback();
    }
}

static void getPairData(const nbgl_contentTagValueList_t *tagValueList,
                        uint8_t                           index,
                        const char                      **item,
                        const char                      **value)
{
    const nbgl_contentTagValue_t *pair;

    if (tagValueList->pairs != NULL) {
        pair = PIC(&tagValueList->pairs[index]);
    }
    else {
        pair = PIC(tagValueList->callback(index));
    }
    *item  = pair->item;
    *value = pair->value;
}

static void onReviewAccept(void)
{
    if (context.review.onChoice) {
        context.review.onChoice(true);
    }
}

static void onReviewReject(void)
{
    if (context.review.onChoice) {
        context.review.onChoice(false);
    }
}

static void onChoiceAccept(void)
{
    if (context.choice.onChoice) {
        context.choice.onChoice(true);
    }
}

static void onChoiceReject(void)
{
    if (context.choice.onChoice) {
        context.choice.onChoice(false);
    }
}

static void onConfirmAccept(void)
{
    if (context.confirm.currentStep) {
        nbgl_stepRelease(context.confirm.currentStep);
    }
    if (context.confirm.onConfirm) {
        context.confirm.onConfirm();
    }
}

static void onConfirmReject(void)
{
    if (context.confirm.currentStep) {
        nbgl_stepRelease(context.confirm.currentStep);
        nbgl_screenRedraw();
        nbgl_refresh();
    }
}

static void onSwitchAction(void)
{
    const nbgl_contentSwitch_t *contentSwitch = NULL;
    const nbgl_content_t       *p_content     = NULL;
    nbgl_content_t              content       = {0};
    uint8_t                     elemIdx;

    p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
    if ((p_content == NULL) || (p_content->type != SWITCHES_LIST)) {
        return;
    }
    contentSwitch
        = &((const nbgl_contentSwitch_t *) PIC(p_content->content.switchesList.switches))[elemIdx];
    switch (context.type) {
        case SETTINGS_USE_CASE:
        case HOME_USE_CASE:
        case GENERIC_SETTINGS:
            displaySettingsPage(FORWARD_DIRECTION, true);
            break;
        case CONTENT_USE_CASE:
        case GENERIC_REVIEW_USE_CASE:
            displayContent(FORWARD_DIRECTION, true);
            break;
        default:
            break;
    }
    if (p_content->contentActionCallback != NULL) {
        nbgl_contentActionCallback_t actionCallback = PIC(p_content->contentActionCallback);
        actionCallback(contentSwitch->token, 0, context.currentPage);
    }
    else if (context.content.controlsCallback != NULL) {
        context.content.controlsCallback(contentSwitch->token, 0);
    }
}

static void drawStep(nbgl_stepPosition_t        pos,
                     const nbgl_icon_details_t *icon,
                     const char                *txt,
                     const char                *subTxt,
                     nbgl_stepButtonCallback_t  onActionCallback,
                     bool                       modal)
{
    uint8_t                           elemIdx;
    nbgl_step_t                       newStep        = NULL;
    const nbgl_content_t             *p_content      = NULL;
    nbgl_content_t                    content        = {0};
    nbgl_contentRadioChoice_t        *contentChoices = NULL;
    nbgl_contentBarsList_t           *contentBars    = NULL;
    nbgl_screenTickerConfiguration_t *p_ticker       = NULL;
    nbgl_layoutMenuList_t             list           = {0};
    nbgl_screenTickerConfiguration_t  ticker         = {
                 .tickerCallback  = PIC(statusTickerCallback),
                 .tickerIntervale = 0,    // not periodic
                 .tickerValue     = 3000  // 3 seconds
    };

    pos |= GET_POS_OF_STEP(context.currentPage, context.nbPages);

    if ((context.type == STATUS_USE_CASE) || (context.type == SPINNER_USE_CASE)) {
        p_ticker = &ticker;
    }
    if ((context.type == CONFIRM_USE_CASE) && (context.confirm.currentStep != NULL)) {
        nbgl_stepRelease(context.confirm.currentStep);
    }

    if (txt == NULL) {
        p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
        if (p_content) {
            switch (p_content->type) {
                case CHOICES_LIST:
                    contentChoices
                        = ((nbgl_contentRadioChoice_t *) PIC(&p_content->content.choicesList));
                    list.nbChoices      = contentChoices->nbChoices + 1;  // For Back button
                    list.selectedChoice = contentChoices->initChoice;
                    list.callback       = getChoiceName;
                    newStep = nbgl_stepDrawMenuList(onChoiceSelected, p_ticker, &list, modal);
                    break;
                case BARS_LIST:
                    contentBars    = ((nbgl_contentBarsList_t *) PIC(&p_content->content.barsList));
                    list.nbChoices = contentBars->nbBars + 1;  // For Back button
                    list.selectedChoice = 0;
                    list.callback       = getChoiceName;
                    newStep = nbgl_stepDrawMenuList(onChoiceSelected, p_ticker, &list, modal);
                    break;
                default:
                    // Not supported as vertical MenuList
                    break;
            }
        }
    }
    else if (icon == NULL) {
        newStep = nbgl_stepDrawText(
            pos, onActionCallback, p_ticker, txt, subTxt, BOLD_TEXT1_INFO, modal);
    }
    else {
        nbgl_layoutCenteredInfo_t info;
        info.icon  = icon;
        info.text1 = txt;
        info.text2 = subTxt;
        info.onTop = false;
        info.style = BOLD_TEXT1_INFO;
        newStep    = nbgl_stepDrawCenteredInfo(pos, onActionCallback, p_ticker, &info, modal);
    }
    if (context.type == CONFIRM_USE_CASE) {
        context.confirm.currentStep = newStep;
    }
}

static bool buttonGenericCallback(nbgl_buttonEvent_t event, nbgl_stepPosition_t *pos)
{
    uint8_t               elemIdx;
    uint8_t               token     = 0;
    uint8_t               index     = 0;
    const nbgl_content_t *p_content = NULL;
    nbgl_content_t        content   = {0};

    if (event == BUTTON_LEFT_PRESSED) {
        if (context.currentPage > 0) {
            context.currentPage--;
        }
        else {
            // Drop the event
            return false;
        }
        *pos = BACKWARD_DIRECTION;
    }
    else if (event == BUTTON_RIGHT_PRESSED) {
        if (context.currentPage < (int) (context.nbPages - 1)) {
            context.currentPage++;
        }
        else {
            // Drop the event
            return false;
        }
        *pos = FORWARD_DIRECTION;
    }
    else {
        if (event == BUTTON_BOTH_PRESSED) {
            if (context.stepCallback != NULL) {
                context.stepCallback();
            }
            else if ((context.type == CONTENT_USE_CASE)
                     || (context.type == GENERIC_REVIEW_USE_CASE)) {
                p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
                if (p_content != NULL) {
                    switch (p_content->type) {
                        case CENTERED_INFO:
                            // No associated callback
                            return false;
                        case INFO_BUTTON:
                            token = p_content->content.infoButton.buttonToken;
                            break;
                        case SWITCHES_LIST:
                            token = p_content->content.switchesList.switches->token;
                            break;
                        case BARS_LIST:
                            token = p_content->content.barsList.tokens[context.currentPage];
                            break;
                        case CHOICES_LIST:
                            token = p_content->content.choicesList.token;
                            index = context.currentPage;
                            break;
                        default:
                            break;
                    }

                    if ((p_content) && (p_content->contentActionCallback != NULL)) {
                        p_content->contentActionCallback(token, 0, context.currentPage);
                    }
                    else if (context.content.controlsCallback != NULL) {
                        context.content.controlsCallback(token, index);
                    }
                }
            }
        }
        return false;
    }
    return true;
}

static void reviewCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayReviewPage(pos);
}

static void streamingReviewCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayStreamingReviewPage(pos);
}

static void settingsCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displaySettingsPage(pos, false);
}

static void infoCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayInfoPage(pos);
}

static void homeCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayHomePage(pos);
}

static void genericChoiceCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayChoicePage(pos);
}

static void genericConfirmCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayConfirm(pos);
}

static void statusButtonCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    if (event == BUTTON_BOTH_PRESSED) {
        if (context.stepCallback != NULL) {
            context.stepCallback();
        }
    }
}

static void contentCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }

    displayContent(pos, false);
}

// callback used for timeout
static void statusTickerCallback(void)
{
    if (context.stepCallback != NULL) {
        context.stepCallback();
    }
}

// function used to display the current page in review
static void displayReviewPage(nbgl_stepPosition_t pos)
{
    uint8_t                    reviewPages  = 0;
    uint8_t                    finalPages   = 0;
    uint8_t                    pairIndex    = 0;
    const char                *text         = NULL;
    const char                *subText      = NULL;
    const nbgl_icon_details_t *icon         = NULL;
    uint8_t                    currentIndex = 0;
    uint8_t                    warnIndex    = 255;
    uint8_t                    titleIndex   = 255;
    uint8_t                    subIndex     = 255;
    uint8_t                    approveIndex = 255;
    uint8_t                    rejectIndex  = 255;

    context.stepCallback = NULL;

    // Determine the 1st page to display tag/values
    if (context.type == REVIEW_BLIND_SIGN_USE_CASE) {
        // Warning page to display
        warnIndex = currentIndex++;
        reviewPages++;
    }
    // Title page to display
    titleIndex = currentIndex++;
    reviewPages++;
    if (context.review.reviewSubTitle) {
        // subtitle page to display
        subIndex = currentIndex++;
        reviewPages++;
    }
    approveIndex = context.nbPages - 2;
    rejectIndex  = context.nbPages - 1;
    finalPages   = approveIndex;

    // Determine which page to display
    if (context.currentPage >= finalPages) {
        if (context.currentPage == approveIndex) {
            // Approve page
            icon                 = &C_icon_validate_14;
            text                 = "Approve";
            context.stepCallback = onReviewAccept;
        }
        else if (context.currentPage == rejectIndex) {
            // Reject page
            icon                 = &C_icon_crossmark;
            text                 = "Reject";
            context.stepCallback = onReviewReject;
        }
    }
    else if (context.currentPage < reviewPages) {
        if (context.currentPage == warnIndex) {
            // Blind Signing Warning page
            icon = &C_icon_warning;
            text = "Blind\nsigning";
        }
        else if (context.currentPage == titleIndex) {
            // Title page
            icon = context.review.icon;
            text = context.review.reviewTitle;
        }
        else if (context.currentPage == subIndex) {
            // SubTitle page
            text = context.review.reviewSubTitle;
        }
    }
    else if ((context.review.address != NULL) && (context.currentPage == reviewPages)) {
        // address confirmation and 2nd page
        text    = "Address";
        subText = context.review.address;
    }
    else {
        pairIndex = context.currentPage - reviewPages;
        if (context.review.address != NULL) {
            pairIndex--;
        }
        getPairData(context.review.tagValueList, pairIndex, &text, &subText);
    }

    drawStep(pos, icon, text, subText, reviewCallback, false);
    nbgl_refresh();
}

// function used to display the current page in review
static void displayStreamingReviewPage(nbgl_stepPosition_t pos)
{
    const char                *text        = NULL;
    const char                *subText     = NULL;
    const nbgl_icon_details_t *icon        = NULL;
    uint8_t                    reviewPages = 0;
    uint8_t                    warnIndex   = 255;
    uint8_t                    titleIndex  = 255;
    uint8_t                    subIndex    = 255;

    context.stepCallback = NULL;

    switch (context.type) {
        case STREAMING_START_REVIEW_USE_CASE:
        case STREAMING_BLIND_SIGN_START_REVIEW_USE_CASE:
            if (context.type == STREAMING_START_REVIEW_USE_CASE) {
                // Title page to display
                titleIndex = reviewPages++;
                if (context.review.reviewSubTitle) {
                    // subtitle page to display
                    subIndex = reviewPages++;
                }
            }
            else {
                // warning page to display
                warnIndex = reviewPages++;
                // Title page to display
                titleIndex = reviewPages++;
                if (context.review.reviewSubTitle) {
                    // subtitle page to display
                    subIndex = reviewPages++;
                }
            }
            // Determine which page to display
            if (context.currentPage >= reviewPages) {
                displaySpinner("Processing");
                onReviewAccept();
                return;
            }
            // header page(s)
            if (context.currentPage == warnIndex) {
                // warning page
                icon = &C_icon_warning;
                text = "Blind\nsigning";
            }
            else if (context.currentPage == titleIndex) {
                // title page
                icon = context.review.icon;
                text = context.review.reviewTitle;
            }
            else if (context.currentPage == subIndex) {
                // subtitle page
                text = context.review.reviewSubTitle;
            }
            break;

        case STREAMING_CONTINUE_REVIEW_USE_CASE:
            if (context.currentPage >= context.review.tagValueList->nbPairs) {
                displaySpinner("Processing");
                onReviewAccept();
                return;
            }
            getPairData(context.review.tagValueList, context.currentPage, &text, &subText);
            break;

        case STREAMING_FINISH_REVIEW_USE_CASE:
        default:
            if (context.currentPage == 0) {
                // accept page
                icon                 = &C_icon_validate_14;
                text                 = "Approve";
                context.stepCallback = onReviewAccept;
            }
            else {
                // reject page
                icon                 = &C_icon_crossmark;
                text                 = "Reject";
                context.stepCallback = onReviewReject;
            }
            break;
    }

    drawStep(pos, icon, text, subText, streamingReviewCallback, false);
    nbgl_refresh();
}

// function used to display the current page in info
static void displayInfoPage(nbgl_stepPosition_t pos)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    if (context.currentPage < (context.nbPages - 1)) {
        text = PIC(
            ((const char *const *) PIC(context.home.infosList->infoTypes))[context.currentPage]);
        subText = PIC(
            ((const char *const *) PIC(context.home.infosList->infoContents))[context.currentPage]);
    }
    else {
        icon                 = &C_icon_back_x;
        text                 = "Back";
        context.stepCallback = startUseCaseHome;
    }

    drawStep(pos, icon, text, subText, infoCallback, false);
    nbgl_refresh();
}

// function used to get the current page content
static void getContentPage(bool                        toogle_state,
                           const char                **text,
                           const char                **subText,
                           const nbgl_icon_details_t **icon)
{
    static char           fullText[75];
    uint8_t               elemIdx;
    nbgl_state_t          state         = OFF_STATE;
    const nbgl_content_t *p_content     = NULL;
    nbgl_content_t        content       = {0};
    nbgl_contentSwitch_t *contentSwitch = NULL;
#ifdef WITH_HORIZONTAL_CHOICES_LIST
    nbgl_contentRadioChoice_t *contentChoices = NULL;
    char                     **names          = NULL;
#endif

    p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
    if (p_content == NULL) {
        return;
    }
    switch (p_content->type) {
        case CENTERED_INFO:
            *text    = PIC(p_content->content.centeredInfo.text1);
            *subText = PIC(p_content->content.centeredInfo.text2);
            break;
        case INFO_BUTTON:
            *icon    = PIC(p_content->content.infoButton.icon);
            *text    = PIC(p_content->content.infoButton.text);
            *subText = PIC(p_content->content.infoButton.buttonText);
            break;
        case TAG_VALUE_LIST:
            getPairData(&p_content->content.tagValueList, elemIdx, text, subText);
            break;
        case SWITCHES_LIST:
            contentSwitch = &(
                (nbgl_contentSwitch_t *) PIC(p_content->content.switchesList.switches))[elemIdx];
            *text = contentSwitch->text;
            state = contentSwitch->initState;
            if (toogle_state) {
                state = (state == ON_STATE) ? OFF_STATE : ON_STATE;
            }
            if (state == ON_STATE) {
                snprintf(fullText, sizeof(fullText), "%s\nEnabled", contentSwitch->subText);
            }
            else {
                snprintf(fullText, sizeof(fullText), "%s\nDisabled", contentSwitch->subText);
            }
            context.stepCallback = onSwitchAction;
            *subText             = fullText;
            break;
        case INFOS_LIST:
            *text = ((const char *const *) PIC(p_content->content.infosList.infoTypes))[elemIdx];
            *subText
                = ((const char *const *) PIC(p_content->content.infosList.infoContents))[elemIdx];
            break;
        case CHOICES_LIST:
#ifdef WITH_HORIZONTAL_CHOICES_LIST
            if ((context.type == CONTENT_USE_CASE) && (context.content.title != NULL)) {
                *text    = PIC(context.content.title);
                *subText = PIC(p_content->content.choicesList.names[elemIdx]);
            }
            else {
                contentChoices = (nbgl_contentRadioChoice_t *) PIC(&p_content->content.choicesList);
                names          = (char **) PIC(contentChoices->names);
                *text          = (const char *) PIC(names[elemIdx]);
            }
#endif
            break;
        case BARS_LIST:
#ifdef WITH_HORIZONTAL_BARS_LIST
            if ((context.type == CONTENT_USE_CASE) && (context.content.title != NULL)) {
                *text    = PIC(context.content.title);
                *subText = PIC(p_content->content.barsList.barTexts[elemIdx]);
            }
            else {
                *text = PIC(p_content->content.barsList.barTexts[elemIdx]);
            }
#endif
            break;
        default:
            break;
    }
}

// function used to display the current page in settings
static void displaySettingsPage(nbgl_stepPosition_t pos, bool toogle_state)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    if (context.currentPage < (context.nbPages - 1)) {
        getContentPage(toogle_state, &text, &subText, &icon);
    }
    else {  // last page is for quit
        icon = &C_icon_back_x;
        text = "Back";
        if (context.type == GENERIC_SETTINGS) {
            context.stepCallback = context.home.quitCallback;
        }
        else {
            context.stepCallback = startUseCaseHome;
        }
    }

    drawStep(pos, icon, text, subText, settingsCallback, false);
    nbgl_refresh();
}

static void startUseCaseHome(void)
{
    int8_t addPages = 0;
    if (context.home.homeAction) {
        // Action page index
        addPages++;
    }
    switch (context.type) {
        case SETTINGS_USE_CASE:
            // Settings page index
            context.currentPage = 1 + addPages;
            break;
        case INFO_USE_CASE:
            // Info page index
            context.currentPage = 2 + addPages;
            break;
        default:
            // Home page index
            context.currentPage = 0;
            break;
    }

    context.type    = HOME_USE_CASE;
    context.nbPages = 2;  // Home + Quit
    if (context.home.settingContents) {
        context.nbPages++;
    }
    if (context.home.infosList) {
        context.nbPages++;
    }
    if (context.home.homeAction) {
        context.nbPages += addPages;
    }
    displayHomePage(FORWARD_DIRECTION);
}

static void startUseCaseInfo(void)
{
    context.type        = INFO_USE_CASE;
    context.nbPages     = context.home.infosList->nbInfos + 1;  // For back screen
    context.currentPage = 0;

    displayInfoPage(FORWARD_DIRECTION);
}

static void startUseCaseSettingsAtPage(uint8_t initSettingPage)
{
    nbgl_content_t        content   = {0};
    const nbgl_content_t *p_content = NULL;

    if (context.type == 0) {
        // Not yet init, it is not a GENERIC_SETTINGS
        context.type = SETTINGS_USE_CASE;
    }
    context.nbPages = 1;  // For back screen
    for (int i = 0; i < context.home.settingContents->nbContents; i++) {
        p_content = getContentAtIdx(context.home.settingContents, i, &content);
        context.nbPages += getContentNbElement(p_content);
    }
    context.currentPage = initSettingPage;

    displaySettingsPage(FORWARD_DIRECTION, false);
}

static void startUseCaseSettings(void)
{
    startUseCaseSettingsAtPage(0);
}

static void startUseCaseContent(void)
{
    uint8_t               contentIdx = 0;
    const nbgl_content_t *p_content  = NULL;
    nbgl_content_t        content    = {0};

    context.nbPages = 1;  // Quit

    for (contentIdx = 0; contentIdx < context.content.genericContents.nbContents; contentIdx++) {
        p_content = getContentAtIdx(&context.content.genericContents, contentIdx, &content);
        context.nbPages += getContentNbElement(p_content);
    }

    displayContent(FORWARD_DIRECTION, false);
}

// function used to display the current page in home
static void displayHomePage(nbgl_stepPosition_t pos)
{
    const char                *text          = NULL;
    const char                *subText       = NULL;
    const nbgl_icon_details_t *icon          = NULL;
    uint8_t                    currentIndex  = 0;
    uint8_t                    homeIndex     = 255;
    uint8_t                    actionIndex   = 255;
    uint8_t                    settingsIndex = 255;
    uint8_t                    infoIndex     = 255;

    context.stepCallback = NULL;

    // Determine which pages are present
    homeIndex = currentIndex++;
    if (context.home.homeAction) {
        actionIndex = currentIndex++;
    }
    if (context.home.settingContents) {
        settingsIndex = currentIndex++;
    }
    if (context.home.infosList) {
        infoIndex = currentIndex++;
    }

    if (context.currentPage == homeIndex) {
        // Home page
        icon = context.home.appIcon;
        if (context.home.tagline != NULL) {
            text = context.home.tagline;
        }
        else {
            text    = context.home.appName;
            subText = "is ready";
        }
    }
    else if (context.currentPage == actionIndex) {
        // Action page
        icon                 = context.home.homeAction->icon;
        text                 = PIC(context.home.homeAction->text);
        context.stepCallback = context.home.homeAction->callback;
    }
    else if (context.currentPage == settingsIndex) {
        // Settings page
        icon                 = &C_icon_coggle;
        text                 = "Settings";
        context.stepCallback = startUseCaseSettings;
    }
    else if (context.currentPage == infoIndex) {
        // About page
        icon                 = &C_icon_certificate;
        text                 = "About";
        context.stepCallback = startUseCaseInfo;
    }
    else {
        icon                 = &C_icon_dashboard_x;
        text                 = "Quit";
        context.stepCallback = context.home.quitCallback;
    }

    drawStep(pos, icon, text, subText, homeCallback, false);
    nbgl_refresh();
}

// function used to display the current page in choice
static void displayChoicePage(nbgl_stepPosition_t pos)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    // Handle case where there is no icon or subMessage
    if (context.currentPage == 1
        && (context.choice.icon == NULL || context.choice.subMessage == NULL)) {
        if (pos & BACKWARD_DIRECTION) {
            context.currentPage -= 1;
        }
        else {
            context.currentPage += 1;
        }
    }

    if (context.currentPage == 0) {  // title page
        text = context.choice.message;
        if (context.choice.icon != NULL) {
            icon = context.choice.icon;
        }
        else {
            subText = context.choice.subMessage;
        }
    }
    else if (context.currentPage == 1) {  // sub-title page
        // displayed only if there is both icon and submessage
        text    = context.choice.message;
        subText = context.choice.subMessage;
    }
    else if (context.currentPage == 2) {  // confirm page
        icon                 = &C_icon_validate_14;
        text                 = context.choice.confirmText;
        context.stepCallback = onChoiceAccept;
    }
    else {  // cancel page
        icon                 = &C_icon_crossmark;
        text                 = context.choice.cancelText;
        context.stepCallback = onChoiceReject;
    }

    drawStep(pos, icon, text, subText, genericChoiceCallback, false);
    nbgl_refresh();
}

// function used to display the Confirm page
static void displayConfirm(nbgl_stepPosition_t pos)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;
    switch (context.currentPage) {
        case 0:
            // title page
            text    = context.confirm.message;
            subText = context.confirm.subMessage;
            break;
        case 1:
            // confirm page
            icon                 = &C_icon_validate_14;
            text                 = context.confirm.confirmText;
            context.stepCallback = onConfirmAccept;
            break;
        case 2:
            // cancel page
            icon                 = &C_icon_crossmark;
            text                 = context.confirm.cancelText;
            context.stepCallback = onConfirmReject;
            break;
    }

    drawStep(pos, icon, text, subText, genericConfirmCallback, true);
    nbgl_refresh();
}

// function used to display the current navigable content
static void displayContent(nbgl_stepPosition_t pos, bool toogle_state)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    if (context.currentPage < (context.nbPages - 1)) {
        getContentPage(toogle_state, &text, &subText, &icon);
    }
    else {  // last page is for quit
        if (context.content.rejectText) {
            text = context.content.rejectText;
        }
        else {
            text = "Back";
        }
        if (context.type == GENERIC_REVIEW_USE_CASE) {
            icon = &C_icon_crossmark;
        }
        else {
            icon = &C_icon_back_x;
        }
        context.stepCallback = context.content.quitCallback;
    }

    drawStep(pos, icon, text, subText, contentCallback, false);
    nbgl_refresh();
}

static void displaySpinner(const char *text)
{
    drawStep(SINGLE_STEP, &C_icon_processing, text, NULL, NULL, false);
    nbgl_refresh();
}

// function to factorize code for all simple reviews
static void useCaseReview(ContextType_t                     type,
                          const nbgl_contentTagValueList_t *tagValueList,
                          const nbgl_icon_details_t        *icon,
                          const char                       *reviewTitle,
                          const char                       *reviewSubTitle,
                          const char                       *finishTitle,
                          nbgl_choiceCallback_t             choiceCallback)
{
    UNUSED(finishTitle);  // TODO dedicated screen for it?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = type;
    context.review.tagValueList   = tagValueList;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    // 1 page for title and 2 pages at the end for accept/reject
    context.nbPages = tagValueList->nbPairs + 3;
    if (type == REVIEW_BLIND_SIGN_USE_CASE) {
        context.nbPages++;  // 1 page for warning
    }
    if (reviewSubTitle) {
        context.nbPages++;  // 1 page for subtitle page
    }

    displayReviewPage(FORWARD_DIRECTION);
}

/**********************
 *  GLOBAL FUNCTIONS
 **********************/

/**
 * @brief with Nano Screen, only a single tag/value pair is displayable in a page
 *
 * @param nbPairs unused
 * @param tagValueList unused
 * @param startIndex unused
 * @param requireSpecificDisplay (output) set to true if the tag/value needs a specific display:
 *        - centeredInfo flag is enabled
 *        - the tag/value doesn't fit in a page
 * @return the number of tag/value pairs fitting in a page
 */
uint8_t nbgl_useCaseGetNbTagValuesInPage(uint8_t                           nbPairs,
                                         const nbgl_contentTagValueList_t *tagValueList,
                                         uint8_t                           startIndex,
                                         bool                             *requireSpecificDisplay)
{
    UNUSED(nbPairs);
    UNUSED(tagValueList);
    UNUSED(startIndex);
    *requireSpecificDisplay = true;
    return 1;
}

/**
 * @brief with Nano Screen, only a single tag/value pair is displayable in a page
 *
 * @param nbPairs unused
 * @param tagValueList unused
 * @param startIndex unused
 * @param isSkippable unused
 * @param requireSpecificDisplay (output) set to true if the tag/value needs a specific display:
 *        - centeredInfo flag is enabled
 *        - the tag/value doesn't fit in a page
 * @return the number of tag/value pairs fitting in a page
 */
uint8_t nbgl_useCaseGetNbTagValuesInPageExt(uint8_t                           nbPairs,
                                            const nbgl_contentTagValueList_t *tagValueList,
                                            uint8_t                           startIndex,
                                            bool                              isSkippable,
                                            bool *requireSpecificDisplay)
{
    UNUSED(nbPairs);
    UNUSED(tagValueList);
    UNUSED(startIndex);
    UNUSED(isSkippable);
    *requireSpecificDisplay = true;
    return 1;
}

/**
 * @brief with Nano Screen, only a single info is displayable in a page
 *
 * @param nbInfos unused
 * @param infosList unused
 * @param startIndex unused
 * @return the number of infos fitting in a page
 */
uint8_t nbgl_useCaseGetNbInfosInPage(uint8_t                       nbInfos,
                                     const nbgl_contentInfoList_t *infosList,
                                     uint8_t                       startIndex,
                                     bool                          withNav)
{
    UNUSED(nbInfos);
    UNUSED(infosList);
    UNUSED(startIndex);
    UNUSED(withNav);
    return 1;
}

/**
 * @brief with Nano Screen, only a single switch is displayable in a page
 *
 * @param nbSwitches unused
 * @param switchesList unused
 * @param startIndex unused
 * @return the number of switches fitting in a page
 */
uint8_t nbgl_useCaseGetNbSwitchesInPage(uint8_t                           nbSwitches,
                                        const nbgl_contentSwitchesList_t *switchesList,
                                        uint8_t                           startIndex,
                                        bool                              withNav)
{
    UNUSED(nbSwitches);
    UNUSED(switchesList);
    UNUSED(startIndex);
    UNUSED(withNav);
    return 1;
}

/**
 * @brief with Nano Screen, only a single bar is displayable in a page
 *
 * @param nbBars unused
 * @param barsList unused
 * @param startIndex unused
 * @return the number of bars fitting in a page
 */
uint8_t nbgl_useCaseGetNbBarsInPage(uint8_t                       nbBars,
                                    const nbgl_contentBarsList_t *barsList,
                                    uint8_t                       startIndex,
                                    bool                          withNav)
{
    UNUSED(nbBars);
    UNUSED(barsList);
    UNUSED(startIndex);
    UNUSED(withNav);
    return 1;
}

/**
 * @brief with Nano Screen, only a single radio choice displayable in a page
 *
 * @param nbChoices unused
 * @param choicesList unused
 * @param startIndex unused
 * @return the number of radio choices fitting in a page
 */
uint8_t nbgl_useCaseGetNbChoicesInPage(uint8_t                          nbChoices,
                                       const nbgl_contentRadioChoice_t *choicesList,
                                       uint8_t                          startIndex,
                                       bool                             withNav)
{
    UNUSED(nbChoices);
    UNUSED(choicesList);
    UNUSED(startIndex);
    UNUSED(withNav);
    return 1;
}

/**
 * @brief  computes the number of pages necessary to display the given list of tag/value pairs
 *
 * @param tagValueList list of tag/value pairs
 * @return the number of pages necessary to display the given list of tag/value pairs
 */
uint8_t nbgl_useCaseGetNbPagesForTagValueList(const nbgl_contentTagValueList_t *tagValueList)
{
    uint8_t nbPages = 0;
    uint8_t nbPairs = tagValueList->nbPairs;
    uint8_t nbPairsInPage;
    uint8_t i = 0;
    bool    flag;

    while (i < tagValueList->nbPairs) {
        // upper margin
        nbPairsInPage = nbgl_useCaseGetNbTagValuesInPageExt(nbPairs, tagValueList, i, false, &flag);
        i += nbPairsInPage;
        nbPairs -= nbPairsInPage;
        nbPages++;
    }
    return nbPages;
}

/**
 * @brief Initiates the drawing a set of pages of generic content, with a touchable header (usually
 * to go back or to an upper level) For each page (including the first one), the given 'navCallback'
 * will be called to get the content. Only 'type' and union has to be set in this content.
 *
 * @param title string to set in touchable title (header)
 * @param initPage page on which to start [0->(nbPages-1)]
 * @param nbPages number of pages
 * @param quitCallback callback called title is pressed
 * @param navCallback callback called when navigation arrows are pressed
 * @param controlsCallback callback called when any controls in the settings (radios, switches) is
 * called (the tokens must be >= @ref FIRST_USER_TOKEN)
 */
void nbgl_useCaseNavigableContent(const char                *title,
                                  uint8_t                    initPage,
                                  uint8_t                    nbPages,
                                  nbgl_callback_t            quitCallback,
                                  nbgl_navCallback_t         navCallback,
                                  nbgl_layoutTouchCallback_t controlsCallback)
{
    nbgl_pageContent_t    pageContent  = {0};
    static nbgl_content_t contentsList = {0};

    if (initPage >= nbPages) {
        return;
    }
    // Use Callback to get the page content
    if (navCallback(initPage, &pageContent) == false) {
        return;
    }
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                                       = CONTENT_USE_CASE;
    context.content.quitCallback                       = quitCallback;
    context.content.controlsCallback                   = controlsCallback;
    context.content.genericContents.callbackCallNeeded = false;
    context.content.genericContents.nbContents         = nbPages;

    contentsList.type = pageContent.type;
    switch (pageContent.type) {
        case CENTERED_INFO:
            contentsList.content.centeredInfo = pageContent.centeredInfo;
            break;
        case INFO_BUTTON:
            contentsList.content.infoButton = pageContent.infoButton;
            break;
        case TAG_VALUE_LIST:
            contentsList.content.tagValueList = pageContent.tagValueList;
            break;
        case SWITCHES_LIST:
            contentsList.content.switchesList = pageContent.switchesList;
            break;
        case INFOS_LIST:
            contentsList.content.infosList = pageContent.infosList;
            break;
        case CHOICES_LIST:
            contentsList.content.choicesList = pageContent.choicesList;
            context.content.title            = title;
            context.currentPage              = pageContent.choicesList.initChoice;
            break;
        case BARS_LIST:
            contentsList.content.barsList = pageContent.barsList;
            context.content.title         = title;
            break;
        default:
            break;
    }
    context.content.genericContents.contentsList = (const nbgl_content_t *) &contentsList;

    startUseCaseContent();
}

/**
 * @brief Draws the extended version of home page of an app (page on which we land when launching it
 *        from dashboard) with automatic support of setting display.
 *
 * @param appName app name
 * @param appIcon app icon
 * @param tagline text under app name (if NULL, it will be "<appName>\n is ready")
 * @param initSettingPage if not INIT_HOME_PAGE, start directly the corresponding setting page
 * @param settingContents setting contents to be displayed
 * @param infosList infos to be displayed (version, license, developer, ...)
 * @param action if not NULL, info used for an action button (on top of "Quit
 * App" button/footer)
 * @param quitCallback callback called when quit button is touched
 */
void nbgl_useCaseHomeAndSettings(const char                   *appName,
                                 const nbgl_icon_details_t    *appIcon,
                                 const char                   *tagline,
                                 const uint8_t                 initSettingPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 const nbgl_homeAction_t      *action,
                                 nbgl_callback_t               quitCallback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.home.appName         = appName;
    context.home.appIcon         = appIcon;
    context.home.tagline         = tagline;
    context.home.settingContents = PIC(settingContents);
    context.home.infosList       = PIC(infosList);
    context.home.homeAction      = action;
    context.home.quitCallback    = quitCallback;

    if (initSettingPage != INIT_HOME_PAGE) {
        startUseCaseSettingsAtPage(initSettingPage);
    }
    else {
        startUseCaseHome();
    }
}

/**
 * @brief Draws the settings pages of an app with automatic pagination depending on content
 *        to be displayed that is passed through settingContents and infosList
 *
 * @param appName string to use as title
 * @param initPage page on which to start, can be != 0 if you want to display a specific page
 * after a setting confirmation change or something. Then the value should be taken from the
 * nbgl_contentActionCallback_t callback call.
 * @param settingContents contents to be displayed
 * @param infosList infos to be displayed (version, license, developer, ...)
 * @param quitCallback callback called when quit button (or title) is pressed
 */
void nbgl_useCaseGenericSettings(const char                   *appName,
                                 uint8_t                       initPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 nbgl_callback_t               quitCallback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                 = GENERIC_SETTINGS;
    context.home.appName         = appName;
    context.home.settingContents = PIC(settingContents);
    context.home.infosList       = PIC(infosList);
    context.home.quitCallback    = quitCallback;

    startUseCaseSettingsAtPage(initPage);
}

/**
 * @brief Draws a set of pages with automatic pagination depending on content
 *        to be displayed that is passed through contents.
 *
 * @param title string to use as title
 * @param initPage page on which to start, can be != 0 if you want to display a specific page
 * after a confirmation change or something. Then the value should be taken from the
 * nbgl_contentActionCallback_t callback call.
 * @param contents contents to be displayed
 * @param quitCallback callback called when quit button (or title) is pressed
 */
void nbgl_useCaseGenericConfiguration(const char                   *title,
                                      uint8_t                       initPage,
                                      const nbgl_genericContents_t *contents,
                                      nbgl_callback_t               quitCallback)
{
    nbgl_useCaseGenericSettings(title, initPage, contents, NULL, quitCallback);
}

/**
 * @brief Draws a flow of pages of a review.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed.
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param tagValueList list of tag/value pairs
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param finishTitle string used in the last review page
 * @param choiceCallback callback called when operation is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseReview(nbgl_operationType_t              operationType,
                        const nbgl_contentTagValueList_t *tagValueList,
                        const nbgl_icon_details_t        *icon,
                        const char                       *reviewTitle,
                        const char                       *reviewSubTitle,
                        const char                       *finishTitle,
                        nbgl_choiceCallback_t             choiceCallback)
{
    UNUSED(operationType);  // TODO adapt accept and reject text depending on this value?

    useCaseReview(REVIEW_USE_CASE,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  choiceCallback);
}

/**
 * @brief Draws a flow of pages of a review. Navigation operates with either swipe or navigation
 * keys at bottom right. The last page contains a long-press button with the given finishTitle and
 * the given icon.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param tagValueList list of tag/value pairs
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param finishTitle string used in the last review page
 * @param dummy inconsistent parameter on Nano devices (ignored)
 * @param choiceCallback callback called when operation is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseAdvancedReview(nbgl_operationType_t              operationType,
                                const nbgl_contentTagValueList_t *tagValueList,
                                const nbgl_icon_details_t        *icon,
                                const char                       *reviewTitle,
                                const char                       *reviewSubTitle,
                                const char                       *finishTitle,
                                const nbgl_tipBox_t              *dummy,
                                nbgl_choiceCallback_t             choiceCallback)
{
    UNUSED(operationType);  // TODO adapt accept and reject text depending on this value?
    UNUSED(dummy);

    useCaseReview(REVIEW_USE_CASE,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  choiceCallback);
}

/**
 * @brief Draws a flow of pages of a blind-signing review. The review is preceded by a warning page
 *
 * Navigation operates with either swipe or navigation
 * keys at bottom right. The last page contains a long-press button with the given finishTitle and
 * the given icon.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param tagValueList list of tag/value pairs
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param finishTitle string used in the last review page
 * @param dummy inconsistent parameter on Nano devices (ignored)
 * @param choiceCallback callback called when operation is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseReviewBlindSigning(nbgl_operationType_t              operationType,
                                    const nbgl_contentTagValueList_t *tagValueList,
                                    const nbgl_icon_details_t        *icon,
                                    const char                       *reviewTitle,
                                    const char                       *reviewSubTitle,
                                    const char                       *finishTitle,
                                    const nbgl_tipBox_t              *dummy,
                                    nbgl_choiceCallback_t             choiceCallback)
{
    UNUSED(operationType);  // TODO adapt accept and reject text depending on this value?
    UNUSED(dummy);

    useCaseReview(REVIEW_BLIND_SIGN_USE_CASE,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  choiceCallback);
}

/**
 * @brief Draws a flow of pages of a review.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed.
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param tagValueList list of tag/value pairs
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param finishTitle string used in the last review page
 * @param choiceCallback callback called when operation is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseReviewLight(nbgl_operationType_t              operationType,
                             const nbgl_contentTagValueList_t *tagValueList,
                             const nbgl_icon_details_t        *icon,
                             const char                       *reviewTitle,
                             const char                       *reviewSubTitle,
                             const char                       *finishTitle,
                             nbgl_choiceCallback_t             choiceCallback)
{
    return nbgl_useCaseReview(operationType,
                              tagValueList,
                              icon,
                              reviewTitle,
                              reviewSubTitle,
                              finishTitle,
                              choiceCallback);
}

/**
 * @brief Draws a flow of pages of an extended address verification page.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed.
 *
 * @param address address to confirm (NULL terminated string)
 * @param additionalTagValueList list of tag/value pairs (can be NULL) (must be persistent because
 * no copy)
 * @param callback callback called when button or footer is touched (if true, button, if false
 * footer)
 * @param icon icon used on the first review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param choiceCallback callback called when transaction is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseAddressReview(const char                       *address,
                               const nbgl_contentTagValueList_t *additionalTagValueList,
                               const nbgl_icon_details_t        *icon,
                               const char                       *reviewTitle,
                               const char                       *reviewSubTitle,
                               nbgl_choiceCallback_t             choiceCallback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = ADDRESS_REVIEW_USE_CASE;
    context.review.address        = address;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    // + 4 because 1 page for title, 1 for address and 2 pages at the end for approve/reject
    context.nbPages = 4;
    if (additionalTagValueList) {
        context.review.tagValueList = PIC(additionalTagValueList);
        context.nbPages += additionalTagValueList->nbPairs;
    }

    displayReviewPage(FORWARD_DIRECTION);
}

/**
 * @brief Draws a flow of pages of a review with automatic pagination depending on content
 *        to be displayed that is passed through contents.
 *
 * @param contents contents to be displayed
 * @param rejectText text to use in footer
 * @param rejectCallback callback called when reject is pressed
 */
void nbgl_useCaseGenericReview(const nbgl_genericContents_t *contents,
                               const char                   *rejectText,
                               nbgl_callback_t               rejectCallback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                                       = GENERIC_REVIEW_USE_CASE;
    context.content.rejectText                         = rejectText;
    context.content.quitCallback                       = rejectCallback;
    context.content.genericContents.nbContents         = contents->nbContents;
    context.content.genericContents.callbackCallNeeded = contents->callbackCallNeeded;
    if (contents->callbackCallNeeded) {
        context.content.genericContents.contentGetterCallback = contents->contentGetterCallback;
    }
    else {
        context.content.genericContents.contentsList = PIC(contents->contentsList);
    }

    startUseCaseContent();
}

/**
 * @brief Draws a transient (3s) status page, either of success or failure, with the given message
 *
 * @param message string to set in middle of page (Upper case for success)
 * @param isSuccess if true, message is drawn in a Ledger style (with corners)
 * @param quitCallback callback called when quit timer times out or status is manually dismissed
 */
void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback)
{
    const nbgl_icon_details_t *icon = NULL;

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type         = STATUS_USE_CASE;
    context.stepCallback = quitCallback;
    context.currentPage  = 0;
    context.nbPages      = 1;

    icon = isSuccess ? &C_icon_validate_14 : &C_icon_crossmark;
    drawStep(SINGLE_STEP, icon, message, NULL, statusButtonCallback, false);
}

/**
 * @brief Draws a transient (3s) status page for the reviewStatusType
 *
 * @param reviewStatusType type of status to display
 * @param quitCallback callback called when quit timer times out or status is manually dismissed
 */
void nbgl_useCaseReviewStatus(nbgl_reviewStatusType_t reviewStatusType,
                              nbgl_callback_t         quitCallback)
{
    const char *msg;
    bool        isSuccess;
    switch (reviewStatusType) {
        case STATUS_TYPE_OPERATION_SIGNED:
            msg       = "Operation signed";
            isSuccess = true;
            break;
        case STATUS_TYPE_OPERATION_REJECTED:
            msg       = "Operation rejected";
            isSuccess = false;
            break;
        case STATUS_TYPE_TRANSACTION_SIGNED:
            msg       = "Transaction signed";
            isSuccess = true;
            break;
        case STATUS_TYPE_TRANSACTION_REJECTED:
            msg       = "Transaction rejected";
            isSuccess = false;
            break;
        case STATUS_TYPE_MESSAGE_SIGNED:
            msg       = "Message signed";
            isSuccess = true;
            break;
        case STATUS_TYPE_MESSAGE_REJECTED:
            msg       = "Message rejected";
            isSuccess = false;
            break;
        case STATUS_TYPE_ADDRESS_VERIFIED:
            msg       = "Address verified";
            isSuccess = true;
            break;
        case STATUS_TYPE_ADDRESS_REJECTED:
            msg       = "Address verification cancelled";
            isSuccess = false;
            break;
        default:
            return;
    }
    nbgl_useCaseStatus(msg, isSuccess, quitCallback);
}

/**
 * @brief Start drawing the flow of pages of a review.
 * @note  This should be followed by calls to nbgl_useCaseReviewStreamingContinue and finally to
 *        nbgl_useCaseReviewStreamingFinish.
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param choiceCallback callback called when more operation data are needed (param is true) or
 * operation is rejected (param is false)
 */
void nbgl_useCaseReviewStreamingStart(nbgl_operationType_t       operationType,
                                      const nbgl_icon_details_t *icon,
                                      const char                *reviewTitle,
                                      const char                *reviewSubTitle,
                                      nbgl_choiceCallback_t      choiceCallback)
{
    UNUSED(operationType);  // TODO adapt accept and reject text depending on this value?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = STREAMING_START_REVIEW_USE_CASE;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    context.nbPages               = 2;  // Start page + trick for review continue

    displayStreamingReviewPage(FORWARD_DIRECTION);
}

/**
 * @brief Start drawing the flow of pages of a blind-signing review. The review is preceded by a
 * warning page
 * @note  This should be followed by calls to nbgl_useCaseReviewStreamingContinue and finally to
 *        nbgl_useCaseReviewStreamingFinish.
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param choiceCallback callback called when more operation data are needed (param is true) or
 * operation is rejected (param is false)
 */
void nbgl_useCaseReviewStreamingBlindSigningStart(nbgl_operationType_t       operationType,
                                                  const nbgl_icon_details_t *icon,
                                                  const char                *reviewTitle,
                                                  const char                *reviewSubTitle,
                                                  nbgl_choiceCallback_t      choiceCallback)
{
    UNUSED(operationType);  // TODO adapt accept and reject text depending on this value?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = STREAMING_BLIND_SIGN_START_REVIEW_USE_CASE;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    context.nbPages               = 3;  // Warning + Start page + trick for review continue

    displayStreamingReviewPage(FORWARD_DIRECTION);
}

/**
 * @brief Continue drawing the flow of pages of a review.
 * @note  This should be called after a call to nbgl_useCaseReviewStreamingStart and can be followed
 *        by others calls to nbgl_useCaseReviewStreamingContinue and finally to
 *        nbgl_useCaseReviewStreamingFinish.
 *
 * @param tagValueList list of tag/value pairs
 * @param choiceCallback callback called when more operation data are needed (param is true) or
 * operation is rejected (param is false)
 * @param skipCallback unused yet on Nano.
 */
void nbgl_useCaseReviewStreamingContinueExt(const nbgl_contentTagValueList_t *tagValueList,
                                            nbgl_choiceCallback_t             choiceCallback,
                                            nbgl_callback_t                   skipCallback)
{
    UNUSED(skipCallback);

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                = STREAMING_CONTINUE_REVIEW_USE_CASE;
    context.review.tagValueList = tagValueList;
    context.review.onChoice     = choiceCallback;
    context.currentPage         = 0;
    context.nbPages             = tagValueList->nbPairs + 1;  // data + trick for review continue

    displayStreamingReviewPage(FORWARD_DIRECTION);
}

/**
 * @brief Continue drawing the flow of pages of a review.
 * @note  This should be called after a call to nbgl_useCaseReviewStreamingStart and can be followed
 *        by others calls to nbgl_useCaseReviewStreamingContinue and finally to
 *        nbgl_useCaseReviewStreamingFinish.
 *
 * @param tagValueList list of tag/value pairs
 * @param choiceCallback callback called when more operation data are needed (param is true) or
 * operation is rejected (param is false)
 */
void nbgl_useCaseReviewStreamingContinue(const nbgl_contentTagValueList_t *tagValueList,
                                         nbgl_choiceCallback_t             choiceCallback)
{
    nbgl_useCaseReviewStreamingContinueExt(tagValueList, choiceCallback, NULL);
}

void nbgl_useCaseReviewStreamingFinish(const char           *finishTitle,
                                       nbgl_choiceCallback_t choiceCallback)
{
    UNUSED(finishTitle);  // TODO dedicated screen for it?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type            = STREAMING_FINISH_REVIEW_USE_CASE;
    context.review.onChoice = choiceCallback;
    context.currentPage     = 0;
    context.nbPages         = 2;  // 2 pages at the end for accept/reject

    displayStreamingReviewPage(FORWARD_DIRECTION);
}

/**
 * @brief draw a spinner page with the given parameters.
 *
 * @param text text to use with the spinner
 */
void nbgl_useCaseSpinner(const char *text)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type        = SPINNER_USE_CASE;
    context.currentPage = 0;
    context.nbPages     = 1;

    displaySpinner(text);
}

void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *cancelText,
                        nbgl_choiceCallback_t      callback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type               = CHOICE_USE_CASE;
    context.choice.icon        = icon;
    context.choice.message     = message;
    context.choice.subMessage  = subMessage;
    context.choice.confirmText = confirmText;
    context.choice.cancelText  = cancelText;
    context.choice.onChoice    = callback;
    context.currentPage        = 0;
    context.nbPages            = 1 + 1 + 2;  // 2 pages at the end for confirm/cancel

    displayChoicePage(FORWARD_DIRECTION);
};

/**
 * @brief Draws a page to confirm or not an action, described in a centered info (with info icon),
 * thanks to a button and a footer at the bottom of the page. The given callback is called if the
 * button is touched. If the footer is touched, the page is only "dismissed"
 * @note This page is displayed as a modal (so the content of the previous page will be visible when
 * dismissed).
 *
 * @param message string to set in center of page (32px)
 * @param subMessage string to set under message (24px) (can be NULL)
 * @param confirmText string to set in button, to confirm
 * @param cancelText string to set in footer, to reject
 * @param callback callback called when confirmation button is touched
 */
void nbgl_useCaseConfirm(const char     *message,
                         const char     *subMessage,
                         const char     *confirmText,
                         const char     *cancelText,
                         nbgl_callback_t callback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                = CONFIRM_USE_CASE;
    context.confirm.message     = message;
    context.confirm.subMessage  = subMessage;
    context.confirm.confirmText = confirmText;
    context.confirm.cancelText  = cancelText;
    context.confirm.onConfirm   = callback;
    context.currentPage         = 0;
    context.nbPages             = 1 + 2;  // 2 pages at the end for confirm/cancel

    displayConfirm(FORWARD_DIRECTION);
}

#endif  // HAVE_SE_TOUCH
#endif  // NBGL_USE_CASE
