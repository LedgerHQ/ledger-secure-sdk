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

/**********************
 *      TYPEDEFS
 **********************/

typedef struct ReviewContext_s {
    nbgl_choiceCallback_t             onChoice;
    const nbgl_contentTagValueList_t *tagValueList;
    const nbgl_icon_details_t        *icon;
    const char                       *reviewTitle;
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

typedef struct HomeContext_s {
    const char                   *appName;
    const nbgl_icon_details_t    *appIcon;
    const char                   *tagline;
    const nbgl_genericContents_t *settingContents;
    const nbgl_contentInfoList_t *infosList;
    nbgl_callback_t               quitCallback;
} HomeContext_t;

typedef enum {
    NONE_USE_CASE,
    REVIEW_USE_CASE,
    ADDRESS_REVIEW_USE_CASE,
    STREAMING_START_REVIEW_USE_CASE,
    STREAMING_CONTINUE_REVIEW_USE_CASE,
    STREAMING_FINISH_REVIEW_USE_CASE,
    CHOICE_USE_CASE,
    HOME_USE_CASE,
    INFO_USE_CASE,
    SETTINGS_USE_CASE,
} ContextType_t;

typedef struct UseCaseContext_s {
    ContextType_t type;
    uint8_t       nbPages;
    int8_t        currentPage;
    nbgl_stepCallback_t
        stepCallback;  ///< if not NULL, function to be called on "double-key" action
    union {
        ReviewContext_t review;
        ChoiceContext_t choice;
        HomeContext_t   home;
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

static void startUseCaseHome(void);
static void startUseCaseInfo(void);
static void startUseCaseSettings(void);
static void startUseCaseSettingsAtPage(uint8_t initSettingPage);

// Simple helper to get the number of elements inside a nbgl_content_t
static uint8_t getContentNbElement(const nbgl_content_t *content)
{
    switch (content->type) {
        case TAG_VALUE_LIST:
            return content->content.tagValueList.nbPairs;
        case SWITCHES_LIST:
            return content->content.switchesList.nbSwitches;
        case INFOS_LIST:
            return content->content.infosList.nbInfos;
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
static const nbgl_content_t *getContentElemAtIdx(const nbgl_genericContents_t *genericContents,
                                                 uint8_t                       elemIdx,
                                                 uint8_t                      *elemContentIdx,
                                                 nbgl_content_t               *content)
{
    const nbgl_content_t *p_content;
    uint8_t               nbPages     = 0;
    uint8_t               elemNbPages = 0;

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

static void onSettingsAction(void)
{
    nbgl_content_t content;
    uint8_t        elemIdx;

    const nbgl_content_t *p_content = getContentElemAtIdx(
        context.home.settingContents, context.currentPage, &elemIdx, &content);

    switch (p_content->type) {
        case SWITCHES_LIST: {
            const nbgl_contentSwitch_t *contentSwitch = &((const nbgl_contentSwitch_t *) PIC(
                p_content->content.switchesList.switches))[elemIdx];
            nbgl_state_t state = (contentSwitch->initState == ON_STATE) ? OFF_STATE : ON_STATE;
            displaySettingsPage(FORWARD_DIRECTION, true);
            if (p_content->contentActionCallback != NULL) {
                nbgl_contentActionCallback_t onContentAction
                    = PIC(p_content->contentActionCallback);
                onContentAction(contentSwitch->token, state, context.currentPage);
            }
            break;
        }
        default:
            break;
    }
}

static void drawStep(nbgl_stepPosition_t        pos,
                     const nbgl_icon_details_t *icon,
                     const char                *txt,
                     const char                *subTxt,
                     nbgl_stepButtonCallback_t  onActionCallback)
{
    pos |= GET_POS_OF_STEP(context.currentPage, context.nbPages);

    if (icon == NULL) {
        nbgl_stepDrawText(pos, onActionCallback, NULL, txt, subTxt, BOLD_TEXT1_INFO, false);
    }
    else {
        nbgl_layoutCenteredInfo_t info;
        info.icon  = icon;
        info.text1 = txt;
        info.text2 = subTxt;
        info.onTop = false;
        info.style = BOLD_TEXT1_INFO;
        nbgl_stepDrawCenteredInfo(pos, onActionCallback, NULL, &info, false);
    }
}

static bool buttonGenericCallback(nbgl_buttonEvent_t event, nbgl_stepPosition_t *pos)
{
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
        if ((event == BUTTON_BOTH_PRESSED) && (context.stepCallback != NULL)) {
            context.stepCallback();
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

static void statusButtonCallback(nbgl_step_t stepCtx, nbgl_buttonEvent_t event)
{
    UNUSED(stepCtx);
    if (event == BUTTON_BOTH_PRESSED) {
        if (context.stepCallback != NULL) {
            context.stepCallback();
        }
    }
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
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    if (context.currentPage == 0) {  // title page
        icon = context.review.icon;
        text = context.review.reviewTitle;
    }
    else if (context.currentPage == (context.nbPages - 2)) {  // accept page
        icon                 = &C_icon_validate_14;
        text                 = "Approve";
        context.stepCallback = onReviewAccept;
    }
    else if (context.currentPage == (context.nbPages - 1)) {  // reject page
        icon                 = &C_icon_crossmark;
        text                 = "Reject";
        context.stepCallback = onReviewReject;
    }
    else if ((context.review.address != NULL)
             && (context.currentPage == 1)) {  // address confirmation and 2nd page
        text    = "Address";
        subText = context.review.address;
    }
    else {
        uint8_t pairIndex = (context.review.address != NULL) ? (context.currentPage - 2)
                                                             : (context.currentPage - 1);
        getPairData(context.review.tagValueList, pairIndex, &text, &subText);
    }

    drawStep(pos, icon, text, subText, reviewCallback);
    nbgl_refresh();
}

// function used to display the current page in review
static void displayStreamingReviewPage(nbgl_stepPosition_t pos)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    if (context.type == STREAMING_START_REVIEW_USE_CASE) {
        if (context.currentPage == 0) {  // title page
            icon = context.review.icon;
            text = context.review.reviewTitle;
        }
        else {
            nbgl_useCaseSpinner("Processing");
            onReviewAccept();
            return;
        }
    }
    else if (context.type == STREAMING_CONTINUE_REVIEW_USE_CASE) {
        if (context.currentPage < context.review.tagValueList->nbPairs) {
            getPairData(context.review.tagValueList, context.currentPage, &text, &subText);
        }
        else {
            nbgl_useCaseSpinner("Processing");
            onReviewAccept();
            return;
        }
    }
    else {
        if (context.currentPage == 0) {  // accept page
            icon                 = &C_icon_validate_14;
            text                 = "Approve";
            context.stepCallback = onReviewAccept;
        }
        else {  // reject page
            icon                 = &C_icon_crossmark;
            text                 = "Reject";
            context.stepCallback = onReviewReject;
        }
    }

    drawStep(pos, icon, text, subText, streamingReviewCallback);
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

    drawStep(pos, icon, text, subText, infoCallback);
    nbgl_refresh();
}

// function used to display the current page in settings
static void displaySettingsPage(nbgl_stepPosition_t pos, bool toogle_state)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    if (context.currentPage < (context.nbPages - 1)) {
        nbgl_content_t nbgl_content;
        uint8_t        elemIdx;

        const nbgl_content_t *p_nbgl_content = getContentElemAtIdx(
            context.home.settingContents, context.currentPage, &elemIdx, &nbgl_content);

        switch (p_nbgl_content->type) {
            case TAG_VALUE_LIST:
                getPairData(&p_nbgl_content->content.tagValueList, elemIdx, &text, &subText);
                break;
            case SWITCHES_LIST: {
                const nbgl_contentSwitch_t *contentSwitch = &((const nbgl_contentSwitch_t *) PIC(
                    p_nbgl_content->content.switchesList.switches))[elemIdx];
                text                                      = contentSwitch->text;
                // switch subtext is ignored
                nbgl_state_t state = contentSwitch->initState;
                if (toogle_state) {
                    state = (state == ON_STATE) ? OFF_STATE : ON_STATE;
                }
                if (state == ON_STATE) {
                    subText = "Enabled";
                }
                else {
                    subText = "Disabled";
                }
                context.stepCallback = onSettingsAction;
                break;
            }
            case INFOS_LIST:
                text    = ((const char *const *) PIC(
                    p_nbgl_content->content.infosList.infoTypes))[elemIdx];
                subText = ((const char *const *) PIC(
                    p_nbgl_content->content.infosList.infoContents))[elemIdx];
                break;
            default:
                break;
        }
    }
    else {  // last page is for quit
        icon                 = &C_icon_back_x;
        text                 = "Back";
        context.stepCallback = startUseCaseHome;
    }

    drawStep(pos, icon, text, subText, settingsCallback);
    nbgl_refresh();
}

static void startUseCaseHome(void)
{
    if (context.type == SETTINGS_USE_CASE) {
        context.currentPage = 1;
    }
    else if (context.type == INFO_USE_CASE) {
        context.currentPage = 2;
    }
    else {
        context.currentPage = 0;
    }
    context.type    = HOME_USE_CASE;
    context.nbPages = 4;

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
    nbgl_content_t        content;
    const nbgl_content_t *p_content;

    context.type    = SETTINGS_USE_CASE;
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

// function used to display the current page in home
static void displayHomePage(nbgl_stepPosition_t pos)
{
    const char                *text    = NULL;
    const char                *subText = NULL;
    const nbgl_icon_details_t *icon    = NULL;

    context.stepCallback = NULL;

    // Handle case where there is no settings
    if (context.home.settingContents == NULL && context.currentPage == 1) {
        if (pos & BACKWARD_DIRECTION) {
            context.currentPage -= 1;
        }
        else {
            context.currentPage += 1;
            if (context.home.infosList == NULL) {
                context.currentPage += 1;
            }
        }
    }

    // Handle case where there is no info
    if (context.home.infosList == NULL && context.currentPage == 2) {
        if (pos & BACKWARD_DIRECTION) {
            context.currentPage -= 1;
            if (context.home.settingContents == NULL) {
                context.currentPage -= 1;
            }
        }
        else {
            context.currentPage += 1;
        }
    }

    switch (context.currentPage) {
        case 0:
            icon = context.home.appIcon;
            if (context.home.tagline != NULL) {
                text = context.home.tagline;
            }
            else {
                text    = context.home.appName;
                subText = "is ready";
            }
            break;
        case 1:
            icon                 = &C_icon_coggle;
            text                 = "Settings";
            context.stepCallback = startUseCaseSettings;
            break;
        case 2:
            icon                 = &C_icon_certificate;
            text                 = "About";
            context.stepCallback = startUseCaseInfo;
            break;
        default:
            icon                 = &C_icon_dashboard_x;
            text                 = "Quit";
            context.stepCallback = context.home.quitCallback;
            break;
    }

    drawStep(pos, icon, text, subText, homeCallback);
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

    drawStep(pos, icon, text, subText, genericChoiceCallback);
    nbgl_refresh();
}

/**********************
 *  GLOBAL FUNCTIONS
 **********************/

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
    UNUSED(action);  // TODO support it at some point?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.home.appName         = appName;
    context.home.appIcon         = appIcon;
    context.home.tagline         = tagline;
    context.home.settingContents = PIC(settingContents);
    context.home.infosList       = PIC(infosList);
    context.home.quitCallback    = quitCallback;

    if (initSettingPage != INIT_HOME_PAGE) {
        startUseCaseSettingsAtPage(initSettingPage);
    }
    else {
        startUseCaseHome();
    }
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
    UNUSED(operationType);   // TODO adapt accept and reject text depending on this value?
    UNUSED(reviewSubTitle);  // TODO dedicated screen for it?
    UNUSED(finishTitle);     // TODO dedicated screen for it?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type                = REVIEW_USE_CASE;
    context.review.tagValueList = tagValueList;
    context.review.reviewTitle  = reviewTitle;
    context.review.icon         = icon;
    context.review.onChoice     = choiceCallback;
    context.currentPage         = 0;
    // + 3 because 1 page for title and 2 pages at the end for accept/reject
    context.nbPages = tagValueList->nbPairs + 3;

    displayReviewPage(FORWARD_DIRECTION);
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
    UNUSED(reviewSubTitle);  // TODO dedicated screen for it?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type               = ADDRESS_REVIEW_USE_CASE;
    context.review.address     = address;
    context.review.reviewTitle = reviewTitle;
    context.review.icon        = icon;
    context.review.onChoice    = choiceCallback;
    context.currentPage        = 0;
    // + 4 because 1 page for title, 1 for address and 2 pages at the end for approve/reject
    context.nbPages = 4;
    if (additionalTagValueList) {
        memcpy(&context.review.tagValueList,
               additionalTagValueList,
               sizeof(nbgl_contentTagValueList_t));
        context.nbPages += additionalTagValueList->nbPairs;
    }

    displayReviewPage(FORWARD_DIRECTION);
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
    UNUSED(isSuccess);  // TODO add icon depending on isSuccess?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.stepCallback = quitCallback;
    context.currentPage  = 0;
    context.nbPages      = 1;

    nbgl_screenTickerConfiguration_t ticker = {
        .tickerCallback  = PIC(statusTickerCallback),
        .tickerIntervale = 0,    // not periodic
        .tickerValue     = 3000  // 3 seconds
    };

    nbgl_stepDrawText(
        SINGLE_STEP, statusButtonCallback, &ticker, message, NULL, BOLD_TEXT1_INFO, false);
    nbgl_refresh();
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
            msg       = "Verification\ncancelled";
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
    UNUSED(operationType);   // TODO adapt accept and reject text depending on this value?
    UNUSED(reviewSubTitle);  // TODO dedicated screen for it?

    memset(&context, 0, sizeof(UseCaseContext_t));
    context.type               = STREAMING_START_REVIEW_USE_CASE;
    context.review.reviewTitle = reviewTitle;
    context.review.icon        = icon;
    context.review.onChoice    = choiceCallback;
    context.currentPage        = 0;
    context.nbPages            = 1 + 1;  // Start page + trick for review continue

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
    drawStep(SINGLE_STEP, &C_icon_processing, text, NULL, NULL);
    nbgl_refresh();
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

#endif  // HAVE_SE_TOUCH
#endif  // NBGL_USE_CASE
