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

/**
 * @brief This is to use in @ref nbgl_operationType_t when the operation is concerned by an internal
 * warning This is used to indicate a warning with a top-right button in review first & last page
 *
 */
#define RISKY_OPERATION (1 << 6)

/**
 * @brief This is to use in @ref nbgl_operationType_t when the operation is concerned by an internal
 * information. This is used to indicate an info with a top-right button in review first & last page
 *
 */
#define NO_THREAT_OPERATION (1 << 7)

/**********************
 *      TYPEDEFS
 **********************/

typedef struct ReviewContext_s {
    nbgl_choiceCallback_t             onChoice;
    const nbgl_contentTagValueList_t *tagValueList;
    const nbgl_icon_details_t        *icon;
    const char                       *reviewTitle;
    const char                       *reviewSubTitle;
    const char                       *finishTitle;
    const char                       *address;       // for address confirmation review
    nbgl_callback_t                   skipCallback;  // callback provided by used
    uint8_t nbDataSets;     // number of sets of data received by StreamingContinue
    bool    skipDisplay;    // if set to true, means that we are displaying the skip page
    uint8_t dataDirection;  // used to know whether the skip page is reached from back or forward
    uint8_t currentTagValueIndex;
    uint8_t currentExtensionPage;
    uint8_t nbExtensionPages;
    const nbgl_contentValueExt_t *extension;
    nbgl_step_t                   extensionStepCtx;

} ReviewContext_t;

typedef struct ChoiceContext_s {
    const nbgl_icon_details_t   *icon;
    const char                  *message;
    const char                  *subMessage;
    const char                  *confirmText;
    const char                  *cancelText;
    nbgl_choiceCallback_t        onChoice;
    const nbgl_genericDetails_t *details;
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
    nbgl_navCallback_t         navCallback;
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

typedef struct ActionContext_s {
    nbgl_callback_t actionCallback;
} ActionContext_t;

#ifdef NBGL_KEYPAD
typedef struct KeypadContext_s {
    uint8_t                 pinEntry[8];
    uint8_t                 pinLen;
    uint8_t                 pinMinDigits;
    uint8_t                 pinMaxDigits;
    nbgl_layout_t          *layoutCtx;
    bool                    hidden;
    uint8_t                 keypadIndex;
    nbgl_pinValidCallback_t validatePin;
    nbgl_callback_t         backCallback;
} KeypadContext_t;
#endif

typedef enum {
    NONE_USE_CASE,
    SPINNER_USE_CASE,
    REVIEW_USE_CASE,
    GENERIC_REVIEW_USE_CASE,
    ADDRESS_REVIEW_USE_CASE,
    STREAMING_START_REVIEW_USE_CASE,
    STREAMING_CONTINUE_REVIEW_USE_CASE,
    STREAMING_FINISH_REVIEW_USE_CASE,
    CHOICE_USE_CASE,
    STATUS_USE_CASE,
    CONFIRM_USE_CASE,
    KEYPAD_USE_CASE,
    HOME_USE_CASE,
    INFO_USE_CASE,
    SETTINGS_USE_CASE,
    GENERIC_SETTINGS,
    CONTENT_USE_CASE,
    ACTION_USE_CASE
} ContextType_t;

typedef struct UseCaseContext_s {
    ContextType_t        type;
    nbgl_operationType_t operationType;
    uint8_t              nbPages;
    int8_t               currentPage;
    int8_t               firstPairPage;
    nbgl_stepCallback_t
        stepCallback;  ///< if not NULL, function to be called on "double-key" action
    union {
        ReviewContext_t  review;
        ChoiceContext_t  choice;
        ConfirmContext_t confirm;
        HomeContext_t    home;
        ContentContext_t content;
#ifdef NBGL_KEYPAD
        KeypadContext_t keypad;
#endif
        ActionContext_t action;
    };
} UseCaseContext_t;

typedef struct PageContent_s {
    bool                          isSwitch;
    const char                   *text;
    const char                   *subText;
    const nbgl_icon_details_t    *icon;
    const nbgl_contentValueExt_t *extension;
    nbgl_state_t                  state;
    bool                          isCenteredInfo;
} PageContent_t;

typedef struct ReviewWithWarningContext_s {
    ContextType_t                     type;
    nbgl_operationType_t              operationType;
    const nbgl_contentTagValueList_t *tagValueList;
    const nbgl_icon_details_t        *icon;
    const char                       *reviewTitle;
    const char                       *reviewSubTitle;
    const char                       *finishTitle;
    const nbgl_warning_t             *warning;
    nbgl_choiceCallback_t             choiceCallback;
    uint8_t                           securityReportLevel;  // level 1 is the first level of menus
    bool                              isIntro;  // set to true during intro (before actual review)
    uint8_t                           warningPage;
    uint8_t                           nbWarningPages;
    uint8_t                           firstWarningPage;
} ReviewWithWarningContext_t;

typedef enum {
    NO_FORCED_TYPE = 0,
    FORCE_BUTTON,
    FORCE_CENTERED_INFO
} ForcedType_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static UseCaseContext_t context;

static ReviewWithWarningContext_t reviewWithWarnCtx;
// configuration of warning when using @ref nbgl_useCaseReviewBlindSigning()
static const nbgl_warning_t blindSigningWarning = {.predefinedSet = (1 << BLIND_SIGNING_WARN)};

// Operation type for streaming (because the one in 'context' is reset at each streaming API call)
nbgl_operationType_t streamingOpType;

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

static void displayExtensionStep(nbgl_stepPosition_t pos);
static void displayWarningStep(void);

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
    nbgl_pageContent_t pageContent = {0};
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
        if (context.content.navCallback) {
            if (context.content.navCallback(contentIdx, &pageContent) == true) {
                // Copy the Page Content to the Content variable
                content->type = pageContent.type;
                switch (content->type) {
                    case CENTERED_INFO:
                        content->content.centeredInfo = pageContent.centeredInfo;
                        break;
                    case INFO_BUTTON:
                        content->content.infoButton = pageContent.infoButton;
                        break;
                    case TAG_VALUE_LIST:
                        content->content.tagValueList = pageContent.tagValueList;
                        break;
                    case SWITCHES_LIST:
                        content->content.switchesList = pageContent.switchesList;
                        break;
                    case INFOS_LIST:
                        content->content.infosList = pageContent.infosList;
                        break;
                    case CHOICES_LIST:
                        content->content.choicesList = pageContent.choicesList;
                        break;
                    case BARS_LIST:
                        content->content.barsList = pageContent.barsList;
                        break;
                    default:
                        LOG_DEBUG(USE_CASE_LOGGER, "Invalid content type\n");
                        return NULL;
                }
            }
            else {
                LOG_DEBUG(USE_CASE_LOGGER, "Error getting page content\n");
                return NULL;
            }
        }
        else {
            genericContents->contentGetterCallback(contentIdx, content);
        }
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
                        const char                      **value,
                        const nbgl_contentValueExt_t    **extension,
                        const nbgl_icon_details_t       **icon,
                        bool                             *isCenteredInfo)
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
    if (pair->aliasValue) {
        *extension = pair->extension;
    }
    else if (pair->centeredInfo) {
        *isCenteredInfo = true;
        *icon           = pair->valueIcon;
    }
    else {
        *extension = NULL;
    }
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
        actionCallback(contentSwitch->token,
                       (contentSwitch->initState == ON_STATE) ? OFF_STATE : ON_STATE,
                       context.currentPage);
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
                     bool                       modal,
                     ForcedType_t               forcedType)
{
    uint8_t                           elemIdx;
    nbgl_step_t                       newStep        = NULL;
    const nbgl_content_t             *p_content      = NULL;
    nbgl_content_t                    content        = {0};
    nbgl_contentRadioChoice_t        *contentChoices = NULL;
    nbgl_contentBarsList_t           *contentBars    = NULL;
    nbgl_screenTickerConfiguration_t *p_ticker       = NULL;
    nbgl_layoutMenuList_t             list           = {0};
    nbgl_screenTickerConfiguration_t  ticker         = {.tickerCallback  = PIC(statusTickerCallback),
                                                        .tickerIntervale = 0,  // not periodic
                                                        .tickerValue     = STATUS_SCREEN_DURATION};

    pos |= GET_POS_OF_STEP(context.currentPage, context.nbPages);
    // if we are in streaming+skip case, enable going backward even for first tag/value of the set
    // (except the first set) because the set starts with a "skip" page
    if ((context.type == STREAMING_CONTINUE_REVIEW_USE_CASE)
        && (context.review.skipCallback != NULL) && (context.review.nbDataSets > 1)) {
        pos |= LAST_STEP;
    }
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
    else if ((icon == NULL) && (forcedType != FORCE_CENTERED_INFO)) {
        nbgl_contentCenteredInfoStyle_t style;
        if (subTxt != NULL) {
            style = (forcedType == FORCE_BUTTON) ? BUTTON_INFO : BOLD_TEXT1_INFO;
        }
        else {
            style = REGULAR_INFO;
        }
        newStep = nbgl_stepDrawText(pos, onActionCallback, p_ticker, txt, subTxt, style, modal);
    }
    else {
        nbgl_layoutCenteredInfo_t info;
        info.icon  = icon;
        info.text1 = txt;
        info.text2 = subTxt;
        info.onTop = false;
        if ((subTxt != NULL) || (context.stepCallback != NULL)) {
            info.style = BOLD_TEXT1_INFO;
        }
        else {
            info.style = REGULAR_INFO;
        }
        newStep = nbgl_stepDrawCenteredInfo(pos, onActionCallback, p_ticker, &info, modal);
    }
    if (context.type == CONFIRM_USE_CASE) {
        context.confirm.currentStep = newStep;
    }
}

static void drawSwitchStep(nbgl_stepPosition_t       pos,
                           const char               *title,
                           const char               *description,
                           bool                      state,
                           nbgl_stepButtonCallback_t onActionCallback,
                           bool                      modal)
{
    nbgl_layoutSwitch_t switchInfo;

    pos |= GET_POS_OF_STEP(context.currentPage, context.nbPages);
    switchInfo.initState = state;
    switchInfo.text      = title;
    switchInfo.subText   = description;
    nbgl_stepDrawSwitch(pos, onActionCallback, NULL, &switchInfo, modal);
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
        // in streaming+skip case, it is allowed to go backward at the first tag/value, except for
        // the first set
        else if ((context.type != STREAMING_CONTINUE_REVIEW_USE_CASE)
                 || (context.review.skipCallback == NULL) || (context.review.nbDataSets == 1)) {
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
            else if ((context.type == CONTENT_USE_CASE) || (context.type == SETTINGS_USE_CASE)
                     || (context.type == GENERIC_SETTINGS)
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
    else {
        // memorize last direction
        context.review.dataDirection = pos;
    }
    displayReviewPage(pos);
}

// this is the callback used when button action on the "skip" page
static void buttonSkipCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    nbgl_stepPosition_t pos;

    if (event == BUTTON_LEFT_PRESSED) {
        // only decrement page if we are going backward but coming from forward (back & forth)
        if ((context.review.dataDirection == FORWARD_DIRECTION)
            && (context.currentPage > context.firstPairPage)) {
            context.currentPage--;
        }
        pos = BACKWARD_DIRECTION;
    }
    else if (event == BUTTON_RIGHT_PRESSED) {
        // only increment page if we are going forward but coming from backward (back & forth)
        if ((context.review.dataDirection == BACKWARD_DIRECTION)
            && (context.currentPage < (int) (context.nbPages - 1))
            && (context.currentPage > context.firstPairPage)) {
            context.currentPage++;
        }
        pos = FORWARD_DIRECTION;
    }
    else if (event == BUTTON_BOTH_PRESSED) {
        // the first tag/value page is at page 0 only in streaming case
        if (context.firstPairPage == 0) {
            // in streaming, we have to call the provided callback
            context.review.skipCallback();
        }
        else {
            // if not in streaming, go directly to the "approve" page
            context.currentPage = context.nbPages - 2;
            displayReviewPage(FORWARD_DIRECTION);
        }
        return;
    }
    else {
        return;
    }
    // the first tag/value page is at page 0 only in streaming case
    if (context.firstPairPage == 0) {
        displayStreamingReviewPage(pos);
    }
    else {
        displayReviewPage(pos);
    }
}

// this is the callback used when buttons in "Action" use case are pressed
static void useCaseActionCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);

    if (event == BUTTON_BOTH_PRESSED) {
        context.action.actionCallback();
    }
}

static void streamingReviewCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    nbgl_stepPosition_t pos;
    UNUSED(stepCtx);

    if (!buttonGenericCallback(event, &pos)) {
        return;
    }
    else {
        // memorize last direction
        context.review.dataDirection = pos;
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
    // any button press should dismiss the status screen
    if ((event == BUTTON_BOTH_PRESSED) || (event == BUTTON_LEFT_PRESSED)
        || (event == BUTTON_RIGHT_PRESSED)) {
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

// this is the callback used when navigating in extension pages
static void extensionNavigate(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    nbgl_stepPosition_t pos;
    UNUSED(stepCtx);

    if (event == BUTTON_LEFT_PRESSED) {
        // only decrement page if we are not at the first page
        if (context.review.currentExtensionPage > 0) {
            context.review.currentExtensionPage--;
        }
        pos = BACKWARD_DIRECTION;
    }
    else if (event == BUTTON_RIGHT_PRESSED) {
        // only increment page if not at last page
        if (context.review.currentExtensionPage < (context.review.nbExtensionPages - 1)) {
            context.review.currentExtensionPage++;
        }
        pos = FORWARD_DIRECTION;
    }
    else if (event == BUTTON_BOTH_PRESSED) {
        // if at last page, leave modal context
        if (context.review.currentExtensionPage == (context.review.nbExtensionPages - 1)) {
            nbgl_stepRelease(context.review.extensionStepCtx);
            nbgl_screenRedraw();
            nbgl_refresh();
        }
        return;
    }
    else {
        return;
    }
    displayExtensionStep(pos);
}

// function used to display the extension pages
static void displayExtensionStep(nbgl_stepPosition_t pos)
{
    nbgl_layoutCenteredInfo_t         info         = {0};
    const nbgl_contentTagValueList_t *tagValueList = NULL;
    const nbgl_contentInfoList_t     *infoList     = NULL;
    const char                       *text         = NULL;
    const char                       *subText      = NULL;

    if (context.review.extensionStepCtx != NULL) {
        nbgl_stepRelease(context.review.extensionStepCtx);
    }
    if (context.review.currentExtensionPage < (context.review.nbExtensionPages - 1)) {
        if (context.review.currentExtensionPage == 0) {
            pos |= FIRST_STEP;
        }
        else {
            pos |= NEITHER_FIRST_NOR_LAST_STEP;
        }

        switch (context.review.extension->aliasType) {
            case ENS_ALIAS:
                text    = context.review.extension->title;
                subText = context.review.extension->fullValue;
                break;
            case INFO_LIST_ALIAS:
                infoList = context.review.extension->infolist;
                text     = PIC(infoList->infoTypes[context.review.currentExtensionPage]);
                subText  = PIC(infoList->infoContents[context.review.currentExtensionPage]);
                break;
            case TAG_VALUE_LIST_ALIAS:
                tagValueList = context.review.extension->tagValuelist;
                text         = PIC(tagValueList->pairs[context.review.currentExtensionPage].item);
                subText      = PIC(tagValueList->pairs[context.review.currentExtensionPage].value);
                break;
            default:
                break;
        }
        if (text != NULL) {
            context.review.extensionStepCtx = nbgl_stepDrawText(
                pos, extensionNavigate, NULL, text, subText, BOLD_TEXT1_INFO, true);
        }
    }
    else if (context.review.currentExtensionPage == (context.review.nbExtensionPages - 1)) {
        // draw the back page
        info.icon  = &C_icon_back_x;
        info.text1 = "Back";
        info.style = BOLD_TEXT1_INFO;
        pos |= LAST_STEP;
        context.review.extensionStepCtx
            = nbgl_stepDrawCenteredInfo(pos, extensionNavigate, NULL, &info, true);
    }
    nbgl_refresh();
}

static void displayAliasFullValue(void)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon;
    bool                       isCenteredInfo;

    getPairData(context.review.tagValueList,
                context.review.currentTagValueIndex,
                &text,
                &subText,
                &context.review.extension,
                &icon,
                &isCenteredInfo);
    if (context.review.extension == NULL) {
        // probably an error
        LOG_WARN(USE_CASE_LOGGER,
                 "displayAliasFullValue: extension nor found for pair %d\n",
                 context.review.currentTagValueIndex);
        return;
    }
    context.review.currentExtensionPage = 0;
    context.review.extensionStepCtx     = NULL;
    // create a modal flow to display this extension
    switch (context.review.extension->aliasType) {
        case ENS_ALIAS:
            context.review.nbExtensionPages = 2;
            break;
        case INFO_LIST_ALIAS:
            context.review.nbExtensionPages = context.review.extension->infolist->nbInfos + 1;
            break;
        case TAG_VALUE_LIST_ALIAS:
            context.review.nbExtensionPages = context.review.extension->tagValuelist->nbPairs + 1;
            break;
        default:
            LOG_WARN(USE_CASE_LOGGER,
                     "displayAliasFullValue: unsupported alias type %d\n",
                     context.review.extension->aliasType);
            return;
    }
    displayExtensionStep(FORWARD_DIRECTION);
}

static void getLastPageInfo(bool approve, const nbgl_icon_details_t **icon, const char **text)
{
    if (approve) {
        // Approve page
        *icon = &C_icon_validate_14;
        if (context.type == ADDRESS_REVIEW_USE_CASE) {
            *text = "Confirm";
        }
        else {
            // if finish title is provided, use it
            if (context.review.finishTitle != NULL) {
                *text = context.review.finishTitle;
            }
            else {
                switch (context.operationType & REAL_TYPE_MASK) {
                    case TYPE_TRANSACTION:
                        if (context.operationType & RISKY_OPERATION) {
                            *text = "Accept risk and sign transaction";
                        }
                        else {
                            *text = "Sign transaction";
                        }
                        break;
                    case TYPE_MESSAGE:
                        if (context.operationType & RISKY_OPERATION) {
                            *text = "Accept risk and sign message";
                        }
                        else {
                            *text = "Sign message";
                        }
                        break;
                    default:
                        if (context.operationType & RISKY_OPERATION) {
                            *text = "Accept risk and sign operation";
                        }
                        else {
                            *text = "Sign operation";
                        }
                        break;
                }
            }
        }
        context.stepCallback = onReviewAccept;
    }
    else {
        // Reject page
        *icon = &C_icon_crossmark;
        if (context.type == ADDRESS_REVIEW_USE_CASE) {
            *text = "Cancel";
        }
        else if ((context.operationType & REAL_TYPE_MASK) == TYPE_TRANSACTION) {
            *text = "Reject transaction";
        }
        else if ((context.operationType & REAL_TYPE_MASK) == TYPE_MESSAGE) {
            *text = "Reject message";
        }
        else {
            *text = "Reject operation";
        }
        context.stepCallback = onReviewReject;
    }
}

// function used to display the current page in review
static void displayReviewPage(nbgl_stepPosition_t pos)
{
    uint8_t                       reviewPages  = 0;
    uint8_t                       finalPages   = 0;
    uint8_t                       pairIndex    = 0;
    const char                   *text         = NULL;
    const char                   *subText      = NULL;
    const nbgl_icon_details_t    *icon         = NULL;
    uint8_t                       currentIndex = 0;
    uint8_t                       titleIndex   = 255;
    uint8_t                       subIndex     = 255;
    uint8_t                       approveIndex = 255;
    uint8_t                       rejectIndex  = 255;
    const nbgl_contentValueExt_t *extension    = NULL;
    ForcedType_t                  forcedType   = NO_FORCED_TYPE;

    context.stepCallback = NULL;

    // Determine the 1st page to display tag/values
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
            getLastPageInfo(true, &icon, &text);
        }
        else if (context.currentPage == rejectIndex) {
            // Reject page
            getLastPageInfo(false, &icon, &text);
        }
    }
    else if (context.currentPage < reviewPages) {
        if (context.currentPage == titleIndex) {
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
        // if there is a skip, and we are not already displaying the "skip" page
        // and we are not at the first tag/value of the first set of data (except if going
        // backward) then display the "skip" page
        if ((context.operationType & SKIPPABLE_OPERATION) && (context.review.skipDisplay == false)
            && ((context.currentPage > reviewPages)
                || (context.review.dataDirection == BACKWARD_DIRECTION))) {
            nbgl_stepPosition_t       directions = (pos & BACKWARD_DIRECTION) | FIRST_STEP;
            nbgl_layoutCenteredInfo_t info       = {0};
            if ((context.review.nbDataSets == 1) || (context.currentPage > 0)) {
                directions |= LAST_STEP;
            }
            info.icon  = &C_Information_circle_14px;
            info.text1 = "Press right button to continue message or \bpress both to skip\b";
            nbgl_stepDrawCenteredInfo(directions, buttonSkipCallback, NULL, &info, false);
            nbgl_refresh();
            context.review.skipDisplay = true;
            context.firstPairPage      = reviewPages;
            return;
        }
        context.review.skipDisplay = false;
        bool isCenteredInfo        = false;
        pairIndex                  = context.currentPage - reviewPages;
        if (context.review.address != NULL) {
            pairIndex--;
        }
        getPairData(context.review.tagValueList,
                    pairIndex,
                    &text,
                    &subText,
                    &extension,
                    &icon,
                    &isCenteredInfo);
        if (extension != NULL) {
            context.stepCallback                = displayAliasFullValue;
            context.review.currentTagValueIndex = pairIndex;
            forcedType                          = FORCE_BUTTON;
        }
        else {
            if (isCenteredInfo) {
                forcedType = FORCE_CENTERED_INFO;
            }
        }
    }

    drawStep(pos, icon, text, subText, reviewCallback, false, forcedType);
    nbgl_refresh();
}

// function used to display the current page in review
static void displayStreamingReviewPage(nbgl_stepPosition_t pos)
{
    const char                   *text        = NULL;
    const char                   *subText     = NULL;
    const nbgl_icon_details_t    *icon        = NULL;
    uint8_t                       reviewPages = 0;
    uint8_t                       titleIndex  = 255;
    uint8_t                       subIndex    = 255;
    const nbgl_contentValueExt_t *extension   = NULL;
    ForcedType_t                  forcedType  = NO_FORCED_TYPE;

    context.stepCallback = NULL;
    switch (context.type) {
        case STREAMING_START_REVIEW_USE_CASE:
            // Title page to display
            titleIndex = reviewPages++;
            if (context.review.reviewSubTitle) {
                // subtitle page to display
                subIndex = reviewPages++;
            }
            // Determine which page to display
            if (context.currentPage >= reviewPages) {
                onReviewAccept();
                return;
            }
            // header page(s)
            if (context.currentPage == titleIndex) {
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
                onReviewAccept();
                return;
            }
            // if there is a skip, and we are not already displaying the "skip" page
            // and we are not at the first tag/value of the first set of data (except if going
            // backward) then display the "skip" page
            if ((context.review.skipCallback != NULL) && (context.review.skipDisplay == false)
                && ((context.review.nbDataSets > 1) || (context.currentPage > 0)
                    || (context.review.dataDirection == BACKWARD_DIRECTION))) {
                nbgl_stepPosition_t       directions = (pos & BACKWARD_DIRECTION) | FIRST_STEP;
                nbgl_layoutCenteredInfo_t info       = {0};
                if ((context.review.nbDataSets == 1) || (context.currentPage > 0)) {
                    directions |= LAST_STEP;
                }
                info.icon  = &C_Information_circle_14px;
                info.text1 = "Press right button to continue message or \bpress both to skip\b";
                nbgl_stepDrawCenteredInfo(directions, buttonSkipCallback, NULL, &info, false);
                nbgl_refresh();
                context.review.skipDisplay = true;
                return;
            }
            context.review.skipDisplay = false;
            bool isCenteredInfo        = false;
            getPairData(context.review.tagValueList,
                        context.currentPage,
                        &text,
                        &subText,
                        &extension,
                        &icon,
                        &isCenteredInfo);
            if (extension != NULL) {
                forcedType = FORCE_BUTTON;
            }
            else {
                if (isCenteredInfo) {
                    forcedType = FORCE_CENTERED_INFO;
                }
            }
            break;

        case STREAMING_FINISH_REVIEW_USE_CASE:
        default:
            if (context.currentPage == 0) {
                // accept page
                getLastPageInfo(true, &icon, &text);
            }
            else {
                // reject page
                getLastPageInfo(false, &icon, &text);
            }
            break;
    }

    drawStep(pos, icon, text, subText, streamingReviewCallback, false, forcedType);
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

    drawStep(pos, icon, text, subText, infoCallback, false, FORCE_CENTERED_INFO);
    nbgl_refresh();
}

// function used to get the current page content
static void getContentPage(bool toogle_state, PageContent_t *contentPage)
{
    uint8_t               elemIdx       = 0;
    const nbgl_content_t *p_content     = NULL;
    nbgl_content_t        content       = {0};
    nbgl_contentSwitch_t *contentSwitch = NULL;
#ifdef WITH_HORIZONTAL_CHOICES_LIST
    nbgl_contentRadioChoice_t *contentChoices = NULL;
    char                     **names          = NULL;
#endif
#ifdef WITH_HORIZONTAL_BARS_LIST
    nbgl_contentBarsList_t *contentBars = NULL;
    char                  **texts       = NULL;
#endif
    p_content = getContentElemAtIdx(context.currentPage, &elemIdx, &content);
    if (p_content == NULL) {
        return;
    }
    switch (p_content->type) {
        case CENTERED_INFO:
            contentPage->text    = PIC(p_content->content.centeredInfo.text1);
            contentPage->subText = PIC(p_content->content.centeredInfo.text2);
            break;
        case INFO_BUTTON:
            contentPage->icon    = PIC(p_content->content.infoButton.icon);
            contentPage->text    = PIC(p_content->content.infoButton.text);
            contentPage->subText = PIC(p_content->content.infoButton.buttonText);
            break;
        case TAG_VALUE_LIST:
            getPairData(&p_content->content.tagValueList,
                        elemIdx,
                        &contentPage->text,
                        &contentPage->subText,
                        &contentPage->extension,
                        &contentPage->icon,
                        &contentPage->isCenteredInfo);
            break;
        case SWITCHES_LIST:
            contentPage->isSwitch = true;
            contentSwitch         = &(
                (nbgl_contentSwitch_t *) PIC(p_content->content.switchesList.switches))[elemIdx];
            contentPage->text  = contentSwitch->text;
            contentPage->state = contentSwitch->initState;
            if (toogle_state) {
                contentPage->state = (contentPage->state == ON_STATE) ? OFF_STATE : ON_STATE;
            }
            context.stepCallback = onSwitchAction;
            contentPage->subText = contentSwitch->subText;
            break;
        case INFOS_LIST:
            contentPage->text
                = ((const char *const *) PIC(p_content->content.infosList.infoTypes))[elemIdx];
            contentPage->subText
                = ((const char *const *) PIC(p_content->content.infosList.infoContents))[elemIdx];
            break;
        case CHOICES_LIST:
#ifdef WITH_HORIZONTAL_CHOICES_LIST
            contentChoices = (nbgl_contentRadioChoice_t *) PIC(&p_content->content.choicesList);
            names          = (char **) PIC(contentChoices->names);
            if ((context.type == CONTENT_USE_CASE) && (context.content.title != NULL)) {
                contentPage->text    = PIC(context.content.title);
                contentPage->subText = (const char *) PIC(names[elemIdx]);
            }
            else if ((context.type == GENERIC_SETTINGS) && (context.home.appName != NULL)) {
                contentPage->text    = PIC(context.home.appName);
                contentPage->subText = (const char *) PIC(names[elemIdx]);
            }
            else {
                contentPage->text = (const char *) PIC(names[elemIdx]);
            }
#endif
            break;
        case BARS_LIST:
#ifdef WITH_HORIZONTAL_BARS_LIST
            contentBars = (nbgl_contentBarsList_t *) PIC(&p_content->content.barsList);
            texts       = (char **) PIC(contentBars->barTexts);
            if ((context.type == CONTENT_USE_CASE) && (context.content.title != NULL)) {
                contentPage->text    = PIC(context.content.title);
                contentPage->subText = PIC(texts[elemIdx]);
            }
            else if ((context.type == GENERIC_SETTINGS) && (context.home.appName != NULL)) {
                contentPage->text    = PIC(context.home.appName);
                contentPage->subText = PIC(texts[elemIdx]);
            }
            else {
                contentPage->text = PIC(texts[elemIdx]);
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
    PageContent_t contentPage = {0};

    context.stepCallback = NULL;

    if (context.currentPage < (context.nbPages - 1)) {
        getContentPage(toogle_state, &contentPage);
    }
    else {  // last page is for quit
        contentPage.icon = &C_icon_back_x;
        contentPage.text = "Back";
        if (context.type == GENERIC_SETTINGS) {
            context.stepCallback = context.home.quitCallback;
        }
        else {
            context.stepCallback = startUseCaseHome;
        }
    }

    if (contentPage.isSwitch) {
        drawSwitchStep(
            pos, contentPage.text, contentPage.subText, contentPage.state, settingsCallback, false);
    }
    else {
        drawStep(pos,
                 contentPage.icon,
                 contentPage.text,
                 contentPage.subText,
                 settingsCallback,
                 false,
                 NO_FORCED_TYPE);
    }

    nbgl_refresh();
}

static void startUseCaseHome(void)
{
    switch (context.type) {
        case SETTINGS_USE_CASE:
            // Settings page index
            context.currentPage = 1;
            if (context.home.homeAction) {
                // Action page is before Settings page
                context.currentPage++;
            }
            break;
        case INFO_USE_CASE:
            // Info page index
            context.currentPage = 1;
            if (context.home.homeAction) {
                // Action page is before Settings and Info pages
                context.currentPage++;
            }
            if (context.home.settingContents) {
                // Settings page is before Info pages
                context.currentPage++;
            }
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
        context.nbPages++;
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

    // if not coming from GENERIC_SETTINGS, force to SETTINGS_USE_CASE
    if (context.type != GENERIC_SETTINGS) {
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

    // Ensure currentPage is valid
    if (context.currentPage >= context.nbPages) {
        return;
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
            subText = "app is ready";
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
        text                 = "App settings";
        context.stepCallback = startUseCaseSettings;
    }
    else if (context.currentPage == infoIndex) {
        // About page
        icon                 = &C_Information_circle_14px;
        text                 = "App info";
        context.stepCallback = startUseCaseInfo;
    }
    else {
        icon                 = &C_Quit_14px;
        text                 = "Quit app";
        context.stepCallback = context.home.quitCallback;
    }

    drawStep(pos, icon, text, subText, homeCallback, false, NO_FORCED_TYPE);
    nbgl_refresh();
}

// function used to display the current page in choice
static void displayChoicePage(nbgl_stepPosition_t pos)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;
    // set to 1 if there is only one page for intro (if either icon or subMessage is NULL)
    uint8_t acceptPage = 0;

    if (context.choice.message != NULL) {
        if ((context.choice.icon == NULL) || (context.choice.subMessage == NULL)) {
            acceptPage = 1;
        }
        else {
            acceptPage = 2;
        }
    }
    context.stepCallback = NULL;

    if (context.currentPage < acceptPage) {
        if (context.currentPage == 0) {  // title page
            text = context.choice.message;
            if (context.choice.icon != NULL) {
                icon = context.choice.icon;
            }
            else {
                subText = context.choice.subMessage;
            }
        }
        else if ((acceptPage == 2) && (context.currentPage == 1)) {  // sub-title page
            // displayed only if there is both icon and submessage
            text    = context.choice.message;
            subText = context.choice.subMessage;
        }
    }
    else if (context.currentPage == acceptPage) {  // confirm page
        icon                 = &C_icon_validate_14;
        text                 = context.choice.confirmText;
        context.stepCallback = onChoiceAccept;
    }
    else if (context.currentPage == (acceptPage + 1)) {  // cancel page
        icon                 = &C_icon_crossmark;
        text                 = context.choice.cancelText;
        context.stepCallback = onChoiceReject;
    }
    else if (context.choice.details != NULL) {
        // only the first level of details and BAR_LIST type are supported
        if (context.choice.details->type == BAR_LIST_WARNING) {
            text = context.choice.details->barList.texts[context.currentPage - (acceptPage + 2)];
            subText
                = context.choice.details->barList.subTexts[context.currentPage - (acceptPage + 2)];
        }
    }

    drawStep(pos, icon, text, subText, genericChoiceCallback, false, NO_FORCED_TYPE);
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

    drawStep(pos, icon, text, subText, genericConfirmCallback, true, NO_FORCED_TYPE);
    nbgl_refresh();
}

// function used to display the current navigable content
static void displayContent(nbgl_stepPosition_t pos, bool toogle_state)
{
    PageContent_t contentPage = {0};
    ForcedType_t  forcedType  = NO_FORCED_TYPE;

    context.stepCallback = NULL;

    if (context.currentPage < (context.nbPages - 1)) {
        getContentPage(toogle_state, &contentPage);
        if (contentPage.isCenteredInfo) {
            forcedType = FORCE_CENTERED_INFO;
        }
    }
    else {  // last page is for quit
        if (context.content.rejectText) {
            contentPage.text = context.content.rejectText;
        }
        else {
            contentPage.text = "Back";
        }
        if (context.type == GENERIC_REVIEW_USE_CASE) {
            contentPage.icon = &C_icon_crossmark;
        }
        else {
            contentPage.icon = &C_icon_back_x;
        }
        context.stepCallback = context.content.quitCallback;
    }

    if (contentPage.isSwitch) {
        drawSwitchStep(
            pos, contentPage.text, contentPage.subText, contentPage.state, contentCallback, false);
    }
    else {
        drawStep(pos,
                 contentPage.icon,
                 contentPage.text,
                 contentPage.subText,
                 contentCallback,
                 false,
                 forcedType);
    }

    nbgl_refresh();
}

static void displaySpinner(const char *text)
{
    drawStep(SINGLE_STEP, &C_icon_processing, text, NULL, NULL, false, false);
    nbgl_refresh();
}

// function to factorize code for all simple reviews
static void useCaseReview(ContextType_t                     type,
                          nbgl_operationType_t              operationType,
                          const nbgl_contentTagValueList_t *tagValueList,
                          const nbgl_icon_details_t        *icon,
                          const char                       *reviewTitle,
                          const char                       *reviewSubTitle,
                          const char                       *finishTitle,
                          nbgl_choiceCallback_t             choiceCallback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = type;
    context.operationType         = operationType;
    context.review.tagValueList   = tagValueList;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.finishTitle    = finishTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    // 1 page for title and 2 pages at the end for accept/reject
    context.nbPages = tagValueList->nbPairs + 3;
    if (reviewSubTitle) {
        context.nbPages++;  // 1 page for subtitle page
    }

    displayReviewPage(FORWARD_DIRECTION);
}

#ifdef NBGL_KEYPAD
static void setPinCodeText(void)
{
    bool enableValidate  = false;
    bool enableBackspace = true;

    // pin can be validated when min digits is entered
    enableValidate = (context.keypad.pinLen >= context.keypad.pinMinDigits);
    // backspace is disabled when no digit is entered and back vallback is not provided
    enableBackspace = (context.keypad.pinLen > 0) || (context.keypad.backCallback != NULL);
    nbgl_layoutUpdateKeypadContent(context.keypad.layoutCtx,
                                   context.keypad.hidden,
                                   context.keypad.pinLen,
                                   (const char *) context.keypad.pinEntry);
    nbgl_layoutUpdateKeypad(
        context.keypad.layoutCtx, context.keypad.keypadIndex, enableValidate, enableBackspace);
    nbgl_layoutDraw(context.keypad.layoutCtx);
    nbgl_refresh();
}

// called when a key is touched on the keypad
static void keypadCallback(char touchedKey)
{
    switch (touchedKey) {
        case BACKSPACE_KEY:
            if (context.keypad.pinLen > 0) {
                context.keypad.pinLen--;
                context.keypad.pinEntry[context.keypad.pinLen] = 0;
            }
            else if (context.keypad.backCallback != NULL) {
                context.keypad.backCallback();
                break;
            }
            setPinCodeText();
            break;

        case VALIDATE_KEY:
            context.keypad.validatePin(context.keypad.pinEntry, context.keypad.pinLen);
            break;

        default:
            if ((touchedKey >= 0x30) && (touchedKey < 0x40)) {
                if (context.keypad.pinLen < context.keypad.pinMaxDigits) {
                    context.keypad.pinEntry[context.keypad.pinLen] = touchedKey;
                    context.keypad.pinLen++;
                }
                setPinCodeText();
            }
            break;
    }
}
#endif  // NBGL_KEYPAD

// this is the function called to start the actual review, from the initial warning pages
static void launchReviewAfterWarning(void)
{
    if (reviewWithWarnCtx.type == REVIEW_USE_CASE) {
        useCaseReview(reviewWithWarnCtx.type,
                      reviewWithWarnCtx.operationType,
                      reviewWithWarnCtx.tagValueList,
                      reviewWithWarnCtx.icon,
                      reviewWithWarnCtx.reviewTitle,
                      reviewWithWarnCtx.reviewSubTitle,
                      reviewWithWarnCtx.finishTitle,
                      reviewWithWarnCtx.choiceCallback);
    }
    else if (reviewWithWarnCtx.type == STREAMING_START_REVIEW_USE_CASE) {
        displayStreamingReviewPage(FORWARD_DIRECTION);
    }
}

// this is the callback used when navigating in warning pages
static void warningNavigate(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);

    if (event == BUTTON_LEFT_PRESSED) {
        // only decrement page if we are not at the first page
        if (reviewWithWarnCtx.warningPage > 0) {
            reviewWithWarnCtx.warningPage--;
        }
    }
    else if (event == BUTTON_RIGHT_PRESSED) {
        // only increment page if not at last page
        if (reviewWithWarnCtx.warningPage < (reviewWithWarnCtx.nbWarningPages - 1)) {
            reviewWithWarnCtx.warningPage++;
        }
        else if ((reviewWithWarnCtx.warning->predefinedSet == 0)
                 && (reviewWithWarnCtx.warning->info != NULL)) {
            launchReviewAfterWarning();
            return;
        }
    }
    else if ((event == BUTTON_BOTH_PRESSED)
             && (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN))) {
        // if at first page, double press leads to start of review
        if (reviewWithWarnCtx.warningPage == reviewWithWarnCtx.firstWarningPage) {
            launchReviewAfterWarning();
        }
        // if at last page, reject operation
        else if (reviewWithWarnCtx.warningPage == (reviewWithWarnCtx.nbWarningPages - 1)) {
            reviewWithWarnCtx.choiceCallback(false);
        }
        return;
    }
    else {
        return;
    }
    displayWarningStep();
}

// function used to display the initial warning pages when starting a "review with warning"
static void displayWarningStep(void)
{
    nbgl_layoutCenteredInfo_t info = {0};
    nbgl_stepPosition_t       pos  = 0;
    if ((reviewWithWarnCtx.warning->prelude) && (reviewWithWarnCtx.warningPage == 0)) {
        // for prelude, only draw text as a single step
        nbgl_stepDrawText(FIRST_STEP | FORWARD_DIRECTION,
                          warningNavigate,
                          NULL,
                          reviewWithWarnCtx.warning->prelude->title,
                          reviewWithWarnCtx.warning->prelude->description,
                          REGULAR_INFO,
                          false);
        nbgl_refresh();
        return;
    }
    else if (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN)) {
        if (reviewWithWarnCtx.warningPage == reviewWithWarnCtx.firstWarningPage) {
            // draw the main warning page
            info.icon  = &C_icon_warning;
            info.text1 = "Blind signing ahead";
            info.text2 = "To accept risk, press both buttons";
            pos        = (reviewWithWarnCtx.firstWarningPage == 0) ? FIRST_STEP
                                                                   : NEITHER_FIRST_NOR_LAST_STEP;
            pos |= FORWARD_DIRECTION;
        }
        else if (reviewWithWarnCtx.warningPage == (reviewWithWarnCtx.nbWarningPages - 1)) {
            getLastPageInfo(false, &info.icon, &info.text1);
            pos = LAST_STEP | BACKWARD_DIRECTION;
        }
    }
    else if ((reviewWithWarnCtx.warning->predefinedSet == 0)
             && (reviewWithWarnCtx.warning->info != NULL)) {
        if (reviewWithWarnCtx.warningPage == reviewWithWarnCtx.firstWarningPage) {
            info.icon  = reviewWithWarnCtx.warning->info->icon;
            info.text1 = reviewWithWarnCtx.warning->info->title;
            info.text2 = reviewWithWarnCtx.warning->info->description;
            pos        = (reviewWithWarnCtx.firstWarningPage == 0) ? FIRST_STEP
                                                                   : NEITHER_FIRST_NOR_LAST_STEP;
            pos |= FORWARD_DIRECTION;
        }
        else if (reviewWithWarnCtx.warningPage == (reviewWithWarnCtx.nbWarningPages - 1)) {
            if (reviewWithWarnCtx.warning->introDetails->type == CENTERED_INFO_WARNING) {
                info.icon  = reviewWithWarnCtx.warning->introDetails->centeredInfo.icon;
                info.text1 = reviewWithWarnCtx.warning->introDetails->centeredInfo.title;
                info.text2 = reviewWithWarnCtx.warning->introDetails->centeredInfo.description;
                pos        = NEITHER_FIRST_NOR_LAST_STEP;
            }
            else {
                // not supported
                return;
            }
        }
    }
    else {
        // not supported
        return;
    }
    info.style = BOLD_TEXT1_INFO;
    nbgl_stepDrawCenteredInfo(pos, warningNavigate, NULL, &info, false);
    nbgl_refresh();
}

// function used to display the initial warning page when starting a "review with warning"
static void displayInitialWarning(void)
{
    // draw the main warning page
    reviewWithWarnCtx.warningPage = 0;
    if ((reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN))
        || ((reviewWithWarnCtx.warning->introDetails)
            && (reviewWithWarnCtx.warning->introDetails->type == CENTERED_INFO_WARNING))) {
        reviewWithWarnCtx.nbWarningPages = 2;
    }
    else {
        // if no intro details and not Blind Signing warning, only one page
        reviewWithWarnCtx.nbWarningPages = 1;
    }

    reviewWithWarnCtx.firstWarningPage = 0;
    displayWarningStep();
}

// function used to display the prelude page when starting a "review with warning"
static void displayPrelude(void)
{
    // draw the main warning page
    reviewWithWarnCtx.warningPage = 0;
    if ((reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN))
        || ((reviewWithWarnCtx.warning->introDetails)
            && (reviewWithWarnCtx.warning->introDetails->type == CENTERED_INFO_WARNING))) {
        reviewWithWarnCtx.nbWarningPages = 3;
    }
    else {
        // if no intro details and not Blind Signing warning, only 2 pages
        reviewWithWarnCtx.nbWarningPages = 2;
    }
    reviewWithWarnCtx.firstWarningPage = 1;
    displayWarningStep();
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
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                                       = CONTENT_USE_CASE;
    context.currentPage                                = initPage;
    context.content.title                              = title;
    context.content.quitCallback                       = quitCallback;
    context.content.navCallback                        = navCallback;
    context.content.controlsCallback                   = controlsCallback;
    context.content.genericContents.callbackCallNeeded = true;
    context.content.genericContents.nbContents         = nbPages;

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

    if ((initSettingPage != INIT_HOME_PAGE) && (settingContents != NULL)) {
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
    useCaseReview(REVIEW_USE_CASE,
                  operationType,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  choiceCallback);
}

/**
 * @brief Draws a flow of pages of a review requiring if necessary a warning page before the review.
 * Moreover, the first and last pages of review display a top-right button, that displays more
 * information about the warnings
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
 * @param tipBox parameter to build a tip-box and necessary modal (can be NULL)
 * @param warning structure to build the initial warning page (can be NULL)
 * @param choiceCallback callback called when operation is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseAdvancedReview(nbgl_operationType_t              operationType,
                                const nbgl_contentTagValueList_t *tagValueList,
                                const nbgl_icon_details_t        *icon,
                                const char                       *reviewTitle,
                                const char                       *reviewSubTitle,
                                const char                       *finishTitle,
                                const nbgl_tipBox_t              *tipBox,
                                const nbgl_warning_t             *warning,
                                nbgl_choiceCallback_t             choiceCallback)
{
    UNUSED(tipBox);
    ContextType_t type = REVIEW_USE_CASE;

    // if no warning at all, it's a simple review
    if ((warning == NULL)
        || ((warning->predefinedSet == 0) && (warning->info == NULL)
            && (warning->reviewDetails == NULL) && (warning->prelude == NULL))) {
        useCaseReview(type,
                      operationType,
                      tagValueList,
                      icon,
                      reviewTitle,
                      reviewSubTitle,
                      finishTitle,
                      choiceCallback);
        return;
    }
    if (warning->predefinedSet == (1 << W3C_NO_THREAT_WARN)) {
        operationType |= NO_THREAT_OPERATION;
    }
    else if (warning->predefinedSet != 0) {
        operationType |= RISKY_OPERATION;
    }

    memset(&reviewWithWarnCtx, 0, sizeof(reviewWithWarnCtx));
    reviewWithWarnCtx.type           = type;
    reviewWithWarnCtx.operationType  = operationType;
    reviewWithWarnCtx.tagValueList   = tagValueList;
    reviewWithWarnCtx.icon           = icon;
    reviewWithWarnCtx.reviewTitle    = reviewTitle;
    reviewWithWarnCtx.reviewSubTitle = reviewSubTitle;
    reviewWithWarnCtx.finishTitle    = finishTitle;
    reviewWithWarnCtx.warning        = warning;
    reviewWithWarnCtx.choiceCallback = choiceCallback;

    // if the warning contains a prelude, display it first
    if (reviewWithWarnCtx.warning->prelude) {
        displayPrelude();
    }
    // display the initial warning only of a risk/threat or blind signing or prelude
    else if (HAS_INITIAL_WARNING(warning)) {
        displayInitialWarning();
    }
    else {
        useCaseReview(type,
                      operationType,
                      tagValueList,
                      icon,
                      reviewTitle,
                      reviewSubTitle,
                      finishTitle,
                      choiceCallback);
    }
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
    nbgl_useCaseAdvancedReview(operationType,
                               tagValueList,
                               icon,
                               reviewTitle,
                               reviewSubTitle,
                               finishTitle,
                               dummy,
                               &blindSigningWarning,
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
    nbgl_useCaseReview(operationType,
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
    // + 1 if sub Title
    context.nbPages = reviewSubTitle ? 5 : 4;
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
 * (any button press)
 */
void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback)
{
    UNUSED(isSuccess);
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type         = STATUS_USE_CASE;
    context.stepCallback = quitCallback;
    context.currentPage  = 0;
    context.nbPages      = 1;

    drawStep(SINGLE_STEP | ACTION_ON_ANY_BUTTON,
             NULL,
             message,
             NULL,
             statusButtonCallback,
             false,
             NO_FORCED_TYPE);
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
    // memorize streaming operation type for future API calls
    streamingOpType = operationType;

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = STREAMING_START_REVIEW_USE_CASE;
    context.operationType         = operationType;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    context.nbPages = reviewSubTitle ? 3 : 2;  // Start page(s) + trick for review continue

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
    nbgl_useCaseAdvancedReviewStreamingStart(
        operationType, icon, reviewTitle, reviewSubTitle, &blindSigningWarning, choiceCallback);
}

/**
 * @brief Start drawing the flow of pages of a blind-signing review. The review is preceded by a
 * warning page if needed
 * @note  This should be followed by calls to @ref nbgl_useCaseReviewStreamingContinue and finally
 * to
 *        @ref nbgl_useCaseReviewStreamingFinish.
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param warning structure to build the initial warning page (cannot be NULL)
 * @param choiceCallback callback called when more operation data are needed (param is true) or
 * operation is rejected (param is false)
 */
void nbgl_useCaseAdvancedReviewStreamingStart(nbgl_operationType_t       operationType,
                                              const nbgl_icon_details_t *icon,
                                              const char                *reviewTitle,
                                              const char                *reviewSubTitle,
                                              const nbgl_warning_t      *warning,
                                              nbgl_choiceCallback_t      choiceCallback)
{
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = STREAMING_START_REVIEW_USE_CASE;
    context.operationType         = operationType;
    context.review.reviewTitle    = reviewTitle;
    context.review.reviewSubTitle = reviewSubTitle;
    context.review.icon           = icon;
    context.review.onChoice       = choiceCallback;
    context.currentPage           = 0;
    context.nbPages = reviewSubTitle ? 3 : 2;  // Start page(s) + trick for review continue

    // memorize streaming operation type for future API calls
    streamingOpType = operationType;

    // if no warning at all, it's a simple review
    if ((warning == NULL)
        || ((warning->predefinedSet == 0) && (warning->info == NULL)
            && (warning->reviewDetails == NULL) && (warning->prelude == NULL))) {
        displayStreamingReviewPage(FORWARD_DIRECTION);
        return;
    }
    if (warning->predefinedSet == (1 << W3C_NO_THREAT_WARN)) {
        operationType |= NO_THREAT_OPERATION;
    }
    else if (warning->predefinedSet != 0) {
        operationType |= RISKY_OPERATION;
    }
    memset(&reviewWithWarnCtx, 0, sizeof(reviewWithWarnCtx));

    reviewWithWarnCtx.type           = context.type;
    reviewWithWarnCtx.operationType  = operationType;
    reviewWithWarnCtx.icon           = icon;
    reviewWithWarnCtx.reviewTitle    = reviewTitle;
    reviewWithWarnCtx.reviewSubTitle = reviewSubTitle;
    reviewWithWarnCtx.choiceCallback = choiceCallback;
    reviewWithWarnCtx.warning        = warning;

    // if the warning contains a prelude, display it first
    if (reviewWithWarnCtx.warning->prelude) {
        displayPrelude();
    }
    // display the initial warning only of a risk/threat or blind signing or prelude
    else if (HAS_INITIAL_WARNING(warning)) {
        displayInitialWarning();
    }
    else {
        displayStreamingReviewPage(FORWARD_DIRECTION);
    }
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
 * @param skipCallback callback called when skip button is pressed (if operationType has the @ref
 * SKIPPABLE_OPERATION in @ref nbgl_useCaseReviewStreamingStart)
 * @ref nbgl_useCaseReviewStreamingFinish shall then be called.
 */
void nbgl_useCaseReviewStreamingContinueExt(const nbgl_contentTagValueList_t *tagValueList,
                                            nbgl_choiceCallback_t             choiceCallback,
                                            nbgl_callback_t                   skipCallback)
{
    uint8_t curNbDataSets = context.review.nbDataSets;

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                = STREAMING_CONTINUE_REVIEW_USE_CASE;
    context.operationType       = streamingOpType;
    context.review.tagValueList = tagValueList;
    context.review.onChoice     = choiceCallback;
    context.currentPage         = 0;
    context.nbPages             = tagValueList->nbPairs + 1;  // data + trick for review continue
    context.review.skipCallback = skipCallback;
    context.review.nbDataSets   = curNbDataSets + 1;

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
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type               = STREAMING_FINISH_REVIEW_USE_CASE;
    context.operationType      = streamingOpType;
    context.review.onChoice    = choiceCallback;
    context.review.finishTitle = finishTitle;
    context.currentPage        = 0;
    context.nbPages            = 2;  // 2 pages at the end for accept/reject

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

/**
 * @brief Draws a generic choice flow, starting with a centered info, and then two pages, one to
 * confirm, the other to cancel. The given callback is called with true as argument if confirmed,
 * false if footer if cancelled
 *
 * @param icon icon to set in center of page
 * @param message main string to set in center of page
 * @param subMessage string to set under message (can be NULL)
 * @param confirmText string to set in 2nd page, to confirm (cannot be NULL)
 * @param cancelText string to set in 3rd page, to reject (cannot be NULL)
 * @param callback callback called when 2nd or 3rd page is double-pressed
 */
void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *cancelText,
                        nbgl_choiceCallback_t      callback)
{
    nbgl_useCaseChoiceWithDetails(
        icon, message, subMessage, confirmText, cancelText, NULL, callback);
};

/**
 * @brief Draws a generic choice flow, starting with a centered info, and then two pages, one to
 * accept, the other to refuse. Then, additional pages are added to display the given details The
 * given callback is called with true as argument if confirmed, false if footer if cancelled
 * @note if icon, message and subMessage are NULL, the flow starts with accept page
 *
 * @param icon icon to set in center of page (can be NULL)
 * @param message main string to set in center of page (can be NULL)
 * @param subMessage string to set under message (can be NULL)
 * @param confirmText string to set in 2nd page, to confirm (cannot be NULL)
 * @param cancelText string to set in 3rd page, to reject (cannot be NULL)
 * @param details  details to be displayed after the 2 pages of action (can be NULL)
 * @param callback callback called when 2nd or 3rd page is double-pressed
 */
void nbgl_useCaseChoiceWithDetails(const nbgl_icon_details_t *icon,
                                   const char                *message,
                                   const char                *subMessage,
                                   const char                *confirmText,
                                   const char                *cancelText,
                                   nbgl_genericDetails_t     *details,
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
    context.choice.details     = details;
    context.currentPage        = 0;
    context.nbPages            = 2;  // 2 pages for confirm/cancel
    if (message != NULL) {
        context.nbPages++;
        // if both icon and subMessage are non NULL, add a page
        if ((icon != NULL) && (subMessage != NULL)) {
            context.nbPages++;
        }
    }
    if (details != NULL) {
        // only the first level of details and BAR_LIST type are supported
        if (details->type == BAR_LIST_WARNING) {
            context.nbPages += details->barList.nbBars;
        }
    }

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

/**
 * @brief Draws a page to represent an action, described in a centered info (with given icon),
 *  The given callback is called if the both buttons are pressed.
 *
 * @param icon icon to set in centered info
 * @param message string to set in centered info (in bold)
 * @param actionText Not used on Nano
 * @param callback callback called when action button is touched
 */
void nbgl_useCaseAction(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *actionText,
                        nbgl_callback_t            callback)
{
    nbgl_layoutCenteredInfo_t centeredInfo = {0};

    UNUSED(actionText);

    // memorize callback in context
    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                  = ACTION_USE_CASE;
    context.action.actionCallback = callback;

    centeredInfo.icon  = icon;
    centeredInfo.text1 = message;
    centeredInfo.style = BOLD_TEXT1_INFO;
    nbgl_stepDrawCenteredInfo(0, useCaseActionCallback, NULL, &centeredInfo, false);
}

#ifdef NBGL_KEYPAD
/**
 * @brief draws a standard keypad modal page with visible digits. It contains
 *        - a navigation bar at the top
 *        - a title for the pin code
 *        - a visible digit entry
 *        - the keypad at the bottom
 *
 * @note callbacks allow to control the behavior.
 *       backspace and validation button are shown/hidden automatically
 *
 * @param title string to set in pin code title
 * @param minDigits pin minimum number of digits
 * @param maxDigits maximum number of digits to be displayed
 * @param shuffled if set to true, digits are shuffled in keypad
 * @param hidden if set to true, digits are hidden in keypad
 * @param validatePinCallback function calledto validate the pin code
 * @param backCallback callback called on backspace is "pressed" in first digit
 */
void nbgl_useCaseKeypad(const char             *title,
                        uint8_t                 minDigits,
                        uint8_t                 maxDigits,
                        bool                    shuffled,
                        bool                    hidden,
                        nbgl_pinValidCallback_t validatePinCallback,
                        nbgl_callback_t         backCallback)
{
    nbgl_layoutDescription_t layoutDescription = {0};
    int                      status            = -1;

    // reset the keypad context
    memset(&context, 0, sizeof(KeypadContext_t));
    context.type                = KEYPAD_USE_CASE;
    context.currentPage         = 0;
    context.nbPages             = 1;
    context.keypad.validatePin  = validatePinCallback;
    context.keypad.backCallback = backCallback;
    context.keypad.pinMinDigits = minDigits;
    context.keypad.pinMaxDigits = maxDigits;
    context.keypad.hidden       = hidden;
    context.keypad.layoutCtx    = nbgl_layoutGet(&layoutDescription);

    // add keypad
    status = nbgl_layoutAddKeypad(context.keypad.layoutCtx, keypadCallback, title, shuffled);
    if (status < 0) {
        return;
    }
    context.keypad.keypadIndex = status;
    // add digits
    status = nbgl_layoutAddKeypadContent(context.keypad.layoutCtx, hidden, maxDigits, "");
    if (status < 0) {
        return;
    }

    nbgl_layoutDraw(context.keypad.layoutCtx);
    if (context.keypad.backCallback != NULL) {
        // force backspace to be visible at first digit, to be used as quit
        nbgl_layoutUpdateKeypad(context.keypad.layoutCtx, context.keypad.keypadIndex, false, true);
    }
    nbgl_refresh();
}
#endif  // NBGL_KEYPAD

#endif  // HAVE_SE_TOUCH
#endif  // NBGL_USE_CASE
