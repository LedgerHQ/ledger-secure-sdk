/**
 * @file nbgl_use_case.c
 * @brief Implementation of typical pages (or sets of pages) for Applications
 */

#ifdef NBGL_USE_CASE
#ifdef HAVE_SE_TOUCH
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdio.h>
#include "nbgl_debug.h"
#include "nbgl_use_case.h"
#include "glyphs.h"
#include "os_pic.h"
#include "os_print.h"
#include "os_helpers.h"

/*********************
 *      DEFINES
 *********************/

/* Defines for definition and usage of genericContextPagesInfo */
#define PAGE_NB_ELEMENTS_BITS            3
#define GET_PAGE_NB_ELEMENTS(pageData)   ((pageData) &0x07)
#define SET_PAGE_NB_ELEMENTS(nbElements) ((nbElements) &0x07)

#define PAGE_FLAG_BITS          1
#define GET_PAGE_FLAG(pageData) (((pageData) &0x08) >> 3)
#define SET_PAGE_FLAG(flag)     (((flag) &0x01) << 3)

#define PAGE_DATA_BITS  (PAGE_NB_ELEMENTS_BITS + PAGE_FLAG_BITS)
#define PAGES_PER_UINT8 (8 / PAGE_DATA_BITS)
#define SET_PAGE_DATA(pageIdx, pageData)             \
    (pagesData[pageIdx / PAGES_PER_UINT8] = pageData \
                                            << ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS))

#define MAX_PAGE_NB 256

/* Alias to clarify usage of genericContext hasStartingContent and hasFinishingContent feature */
#define STARTING_CONTENT  localContentsList[0]
#define FINISHING_CONTENT localContentsList[1]

/* max number of lines for address under QR Code */
#define QRCODE_NB_MAX_LINES     3
/* max number of char for reduced QR Code address */
#define QRCODE_REDUCED_ADDR_LEN 128

/**********************
 *      TYPEDEFS
 **********************/
enum {
    BACK_TOKEN = 0,
    NEXT_TOKEN,
    QUIT_TOKEN,
    NAV_TOKEN,
    SKIP_TOKEN,
    CONTINUE_TOKEN,
    ADDRESS_QRCODE_BUTTON_TOKEN,
    ACTION_BUTTON_TOKEN,
    CHOICE_TOKEN,
    DETAILS_BUTTON_TOKEN,
    CONFIRM_TOKEN,
    REJECT_TOKEN,
    VALUE_ALIAS_TOKEN,
    BLIND_WARNING_TOKEN
};

typedef enum {
    REVIEW_NAV = 0,
    SETTINGS_NAV,
    GENERIC_NAV,
    STREAMING_NAV
} NavType_t;

typedef struct DetailsContext_s {
    uint8_t     nbPages;
    uint8_t     currentPage;
    bool        wrapping;
    const char *tag;
    const char *value;
    const char *nextPageStart;
} DetailsContext_t;

typedef struct AddressConfirmationContext_s {
    nbgl_layoutTagValue_t tagValuePair;
    nbgl_layout_t         modalLayout;
} AddressConfirmationContext_t;

#ifdef NBGL_KEYPAD
typedef struct KeypadContext_s {
    uint8_t        pinEntry[KEYPAD_MAX_DIGITS];
    uint8_t        pinLen;
    uint8_t        pinMinDigits;
    uint8_t        pinMaxDigits;
    nbgl_layout_t *layoutCtx;
    bool           hidden;
} KeypadContext_t;
#endif

typedef struct {
    nbgl_genericContents_t genericContents;
    int8_t                 currentContentIdx;
    uint8_t                currentContentElementNb;
    uint8_t                currentElementIdx;
    bool                   hasStartingContent;
    bool                   hasFinishingContent;
    const char            *detailsItem;
    const char            *detailsvalue;
    bool                   detailsWrapping;
    const nbgl_contentTagValue_t
        *currentPairs;  // to be used to retrieve the pairs with value alias
    nbgl_contentTagValueCallback_t
        currentCallback;  // to be used to retrieve the pairs with value alias

    nbgl_layout_t modalLayout;
} GenericContext_t;

typedef struct {
    const char                   *appName;
    const nbgl_icon_details_t    *appIcon;
    const char                   *tagline;
    const nbgl_genericContents_t *settingContents;
    const nbgl_contentInfoList_t *infosList;
    const char                   *actionText;
    nbgl_callback_t               actionCallback;
    nbgl_callback_t               quitCallback;
} nbgl_homeAndSettingsContext_t;

typedef struct {
    nbgl_operationType_t  operationType;
    nbgl_choiceCallback_t choiceCallback;
} nbgl_reviewContext_t;

typedef struct {
    nbgl_operationType_t       operationType;
    nbgl_choiceCallback_t      choiceCallback;
    nbgl_callback_t            skipCallback;
    const nbgl_icon_details_t *icon;
    uint8_t                    stepPageNb;
} nbgl_reviewStreamingContext_t;

typedef union {
    nbgl_homeAndSettingsContext_t homeAndSettings;
    nbgl_reviewContext_t          review;
    nbgl_reviewStreamingContext_t reviewStreaming;
} nbgl_BundleNavContext_t;

/**********************
 *  STATIC VARIABLES
 **********************/

// char buffers to build some strings
static char appDescription[APP_DESCRIPTION_MAX_LEN];
static char plugInDescription[APP_DESCRIPTION_MAX_LEN];

// multi-purposes callbacks
static nbgl_callback_t              onQuit;
static nbgl_callback_t              onContinue;
static nbgl_callback_t              onAction;
static nbgl_navCallback_t           onNav;
static nbgl_layoutTouchCallback_t   onControls;
static nbgl_contentActionCallback_t onContentAction;
static nbgl_choiceCallback_t        onChoice;
static nbgl_callback_t              onModalConfirm;
#ifdef NBGL_KEYPAD
static nbgl_pinValidCallback_t onValidatePin;
#endif

// contexts for background and modal pages
static nbgl_page_t *pageContext;
static nbgl_page_t *modalPageContext;

// context for pages
static const char *pageTitle;

// context for navigation use case
static nbgl_pageNavigationInfo_t navInfo;
static bool                      forwardNavOnly;
static NavType_t                 navType;

static DetailsContext_t detailsContext;

static AddressConfirmationContext_t addressConfirmationContext;

#ifdef NBGL_KEYPAD
static KeypadContext_t keypadContext;
#endif

// contexts for generic navigation
static GenericContext_t genericContext;
static nbgl_content_t
    localContentsList[3];  // 3 needed for nbgl_useCaseReview (starting page / tags / final page)
static uint8_t genericContextPagesInfo[MAX_PAGE_NB / PAGES_PER_UINT8];

// contexts for bundle navigation
static nbgl_BundleNavContext_t bundleNavContext;

// indexed by nbgl_contentType_t
static const uint8_t nbMaxElementsPerContentType[] = {
#ifdef TARGET_STAX
    1,  // CENTERED_INFO
    1,  // INFO_LONG_PRESS
    1,  // INFO_BUTTON
    1,  // TAG_VALUE_LIST (computed dynamically)
    1,  // TAG_VALUE_DETAILS
    1,  // TAG_VALUE_CONFIRM
    3,  // SWITCHES_LIST
    3,  // INFOS_LIST
    5,  // CHOICES_LIST
    5,  // BARS_LIST
#else   // TARGET_STAX
    1,  // CENTERED_INFO
    1,  // INFO_LONG_PRESS
    1,  // INFO_BUTTON
    1,  // TAG_VALUE_LIST (computed dynamically)
    1,  // TAG_VALUE_DETAILS
    1,  // TAG_VALUE_CONFIRM
    2,  // SWITCHES_LIST
    2,  // INFOS_LIST
    4,  // CHOICES_LIST
    4,  // BARS_LIST
#endif  // TARGET_STAX
};

#ifdef NBGL_QRCODE
/* buffer to store reduced address under QR Code */
static char reducedAddress[QRCODE_REDUCED_ADDR_LEN];
#endif  // NBGL_QRCODE

/**********************
 *  STATIC FUNCTIONS
 **********************/
static void displayReviewPage(uint8_t page, bool forceFullRefresh);
static void displayDetailsPage(uint8_t page, bool forceFullRefresh);
static void displayFullValuePage(const nbgl_contentTagValue_t *pair);
static void displayBlindWarning(nbgl_opType_t opType);
static void displaySettingsPage(uint8_t page, bool forceFullRefresh);
static void displayGenericContextPage(uint8_t pageIdx, bool forceFullRefresh);
static void pageCallback(int token, uint8_t index);
#ifdef NBGL_QRCODE
static void displayAddressQRCode(void);
static void addressLayoutTouchCallbackQR(int token, uint8_t index);
#endif  // NBGL_QRCODE
static void modalLayoutTouchCallback(int token, uint8_t index);
static void displaySkipWarning(void);

static void bundleNavStartHome(void);
static void bundleNavStartSettingsAtPage(uint8_t initSettingPage);
static void bundleNavStartSettings(void);

static void bundleNavReviewStreamingChoice(bool confirm);

static void reset_callbacks(void)
{
    onQuit          = NULL;
    onContinue      = NULL;
    onAction        = NULL;
    onNav           = NULL;
    onControls      = NULL;
    onContentAction = NULL;
    onChoice        = NULL;
    onModalConfirm  = NULL;
#ifdef NBGL_KEYPAD
    onValidatePin = NULL;
#endif
}

// Helper to set genericContext page info
static void genericContextSetPageInfo(uint8_t pageIdx, uint8_t nbElements, bool flag)
{
    uint8_t pageData = SET_PAGE_NB_ELEMENTS(nbElements) + SET_PAGE_FLAG(flag);

    genericContextPagesInfo[pageIdx / PAGES_PER_UINT8]
        &= ~(0x0F << ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS));
    genericContextPagesInfo[pageIdx / PAGES_PER_UINT8]
        |= pageData << ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS);
}

// Helper to get genericContext page info
static void genericContextGetPageInfo(uint8_t pageIdx, uint8_t *nbElements, bool *flag)
{
    uint8_t pageData = genericContextPagesInfo[pageIdx / PAGES_PER_UINT8]
                       >> ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS);
    if (nbElements != NULL) {
        *nbElements = GET_PAGE_NB_ELEMENTS(pageData);
    }
    if (flag != NULL) {
        *flag = GET_PAGE_FLAG(pageData);
    }
}

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
        case CHOICES_LIST:
            return content->content.choicesList.nbChoices;
        case BARS_LIST:
            return content->content.barsList.nbBars;
        default:
            return 1;
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

static void prepareNavInfo(bool isReview, uint8_t nbPages, const char *rejectText)
{
    memset(&navInfo, 0, sizeof(navInfo));

    navInfo.nbPages           = nbPages;
    navInfo.tuneId            = TUNE_TAP_CASUAL;
    navInfo.progressIndicator = false;
    navInfo.navType           = NAV_WITH_BUTTONS;

    if (isReview == false) {
        navInfo.navWithButtons.navToken   = NAV_TOKEN;
        navInfo.navWithButtons.backButton = true;
    }
    else {
        navInfo.quitToken               = REJECT_TOKEN;
        navInfo.navWithButtons.quitText = rejectText;
        navInfo.navWithButtons.navToken = NAV_TOKEN;
        navInfo.navWithButtons.backButton
            = ((navType == STREAMING_NAV) && (nbPages < 2)) ? false : true;
        navInfo.navWithButtons.visiblePageIndicator = (navType != STREAMING_NAV);
    }
}

static void prepareReviewFirstPage(nbgl_contentCenteredInfo_t *centeredInfo,
                                   const nbgl_icon_details_t  *icon,
                                   const char                 *reviewTitle,
                                   const char                 *reviewSubTitle)
{
    centeredInfo->icon    = icon;
    centeredInfo->text1   = reviewTitle;
    centeredInfo->text2   = reviewSubTitle;
    centeredInfo->text3   = "Swipe to review";
    centeredInfo->style   = LARGE_CASE_GRAY_INFO;
    centeredInfo->offsetY = 0;
}

static void prepareReviewLastPage(nbgl_contentInfoLongPress_t *infoLongPress,
                                  const nbgl_icon_details_t   *icon,
                                  const char                  *finishTitle)
{
    infoLongPress->text           = finishTitle;
    infoLongPress->icon           = icon;
    infoLongPress->longPressText  = "Hold to sign";
    infoLongPress->longPressToken = CONFIRM_TOKEN;
}

static void prepareReviewLightLastPage(nbgl_contentInfoButton_t  *infoButton,
                                       const nbgl_icon_details_t *icon,
                                       const char                *finishTitle)
{
    infoButton->text        = finishTitle;
    infoButton->icon        = icon;
    infoButton->buttonText  = "Approve";
    infoButton->buttonToken = CONFIRM_TOKEN;
}

static const char *getRejectReviewText(nbgl_operationType_t operationType)
{
    UNUSED(operationType);
    return "Reject";
}

// function called when navigating (or exiting) modal details pages
// or when skip choice is displayed
static void pageModalCallback(int token, uint8_t index)
{
    LOG_DEBUG(USE_CASE_LOGGER, "pageModalCallback, token = %d, index = %d\n", token, index);
    nbgl_pageRelease(modalPageContext);
    modalPageContext = NULL;
    if (token == NAV_TOKEN) {
        if (index == EXIT_PAGE) {
            // redraw the background layer
            nbgl_screenRedraw();
            nbgl_refresh();
        }
        else {
            displayDetailsPage(index, false);
        }
    }
    if (token == QUIT_TOKEN) {
        // redraw the background layer
        nbgl_screenRedraw();
        nbgl_refresh();
    }
    else if (token == SKIP_TOKEN) {
        if (index == 0) {
            // display the last forward only review page, whatever it is
            displayGenericContextPage(LAST_PAGE_FOR_REVIEW, true);
        }
        else {
            // display background, which should be the page where skip has been touched
            nbgl_screenRedraw();
            nbgl_refresh();
        }
    }
    else if (token == CHOICE_TOKEN) {
        if (index == 0) {
            if (onModalConfirm != NULL) {
                onModalConfirm();
            }
        }
        else {
            // display background, which should be the page where skip has been touched
            nbgl_screenRedraw();
            nbgl_refresh();
        }
    }
}

// generic callback for all pages except modal
static void pageCallback(int token, uint8_t index)
{
    LOG_DEBUG(USE_CASE_LOGGER, "pageCallback, token = %d, index = %d\n", token, index);
    if (token == QUIT_TOKEN) {
        if (onQuit != NULL) {
            onQuit();
        }
    }
    else if (token == CONTINUE_TOKEN) {
        if (onContinue != NULL) {
            onContinue();
        }
    }
    else if (token == CHOICE_TOKEN) {
        if (onChoice != NULL) {
            onChoice((index == 0) ? true : false);
        }
    }
    else if (token == ACTION_BUTTON_TOKEN) {
        if ((index == 0) && (onAction != NULL)) {
            onAction();
        }
        else if ((index == 1) && (onQuit != NULL)) {
            onQuit();
        }
    }
#ifdef NBGL_QRCODE
    else if (token == ADDRESS_QRCODE_BUTTON_TOKEN) {
        displayAddressQRCode();
    }
#endif  // NBGL_QRCODE
    else if (token == CONFIRM_TOKEN) {
        if (onChoice != NULL) {
            onChoice(true);
        }
    }
    else if (token == REJECT_TOKEN) {
        if (onChoice != NULL) {
            onChoice(false);
        }
    }
    else if (token == DETAILS_BUTTON_TOKEN) {
        nbgl_useCaseViewDetails(genericContext.detailsItem,
                                genericContext.detailsvalue,
                                genericContext.detailsWrapping);
    }
    else if (token == NAV_TOKEN) {
        if (index == EXIT_PAGE) {
            if (onQuit != NULL) {
                onQuit();
            }
        }
        else {
            if (navType == GENERIC_NAV || navType == STREAMING_NAV) {
                displayGenericContextPage(index, false);
            }
            else if (navType == REVIEW_NAV) {
                displayReviewPage(index, false);
            }
            else {
                displaySettingsPage(index, false);
            }
        }
    }
    else if (token == NEXT_TOKEN) {
        if (onNav != NULL) {
            displayReviewPage(navInfo.activePage + 1, false);
        }
        else {
            displayGenericContextPage(navInfo.activePage + 1, false);
        }
    }
    else if (token == BACK_TOKEN) {
        if (onNav != NULL) {
            displayReviewPage(navInfo.activePage - 1, true);
        }
        else {
            displayGenericContextPage(navInfo.activePage - 1, false);
        }
    }
    else if (token == SKIP_TOKEN) {
        // display a modal warning to confirm skip
        displaySkipWarning();
    }
    else if (token == VALUE_ALIAS_TOKEN) {
        // the icon next to value alias has been touched
        const nbgl_contentTagValue_t *pair;
        if (genericContext.currentPairs != NULL) {
            pair = &genericContext.currentPairs[genericContext.currentElementIdx + index];
        }
        else {
            pair = genericContext.currentCallback(genericContext.currentElementIdx + index);
        }
        displayFullValuePage(pair);
    }
    else if (token == BLIND_WARNING_TOKEN) {
        if (navType == STREAMING_NAV) {
            displayBlindWarning(bundleNavContext.reviewStreaming.operationType
                                & ~(SKIPPABLE_OPERATION | BLIND_OPERATION));
        }
        else {
            displayBlindWarning(bundleNavContext.review.operationType
                                & ~(SKIPPABLE_OPERATION | BLIND_OPERATION));
        }
    }
    else {  // probably a control provided by caller
        if (onContentAction != NULL) {
            onContentAction(token, index, navInfo.activePage);
        }
        if (onControls != NULL) {
            onControls(token, index);
        }
    }
}

// callback used for confirmation
static void tickerCallback(void)
{
    nbgl_pageRelease(pageContext);
    if (onQuit != NULL) {
        onQuit();
    }
}

// function used to display the current page in review
static void displaySettingsPage(uint8_t page, bool forceFullRefresh)
{
    nbgl_pageContent_t content = {0};

    if ((onNav == NULL) || (onNav(page, &content) == false)) {
        return;
    }

    // override some fields
    content.title            = pageTitle;
    content.isTouchableTitle = true;
    content.titleToken       = QUIT_TOKEN;
    content.tuneId           = TUNE_TAP_CASUAL;

    navInfo.activePage = page;
    pageContext        = nbgl_pageDrawGenericContent(&pageCallback, &navInfo, &content);

    if (forceFullRefresh) {
        nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
    }
    else {
        nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
    }
}

// function used to display the current page in review
static void displayReviewPage(uint8_t page, bool forceFullRefresh)
{
    nbgl_pageContent_t content = {0};

    // ensure the page is valid
    if ((navInfo.nbPages != 0) && (page >= (navInfo.nbPages))) {
        return;
    }
    navInfo.activePage = page;
    if ((onNav == NULL) || (onNav(navInfo.activePage, &content) == false)) {
        return;
    }

    // override some fields
    content.title            = NULL;
    content.isTouchableTitle = false;
    content.tuneId           = TUNE_TAP_CASUAL;

    if (content.type == INFO_LONG_PRESS) {  // last page
        // for forward only review without known length...
        // if we don't do that we cannot remove the '>' in the navigation bar at the last page
        navInfo.nbPages                      = navInfo.activePage + 1;
        content.infoLongPress.longPressToken = CONFIRM_TOKEN;
        if (forwardNavOnly) {
            // remove the "Skip" button
            navInfo.skipText = NULL;
        }
    }

    // override smallCaseForValue for tag/value types to false
    if (content.type == TAG_VALUE_DETAILS) {
        content.tagValueDetails.tagValueList.smallCaseForValue = false;
        // the maximum displayable number of lines for value is NB_MAX_LINES_IN_REVIEW (without More
        // button)
        content.tagValueDetails.tagValueList.nbMaxLinesForValue = NB_MAX_LINES_IN_REVIEW;
    }
    else if (content.type == TAG_VALUE_LIST) {
        content.tagValueList.smallCaseForValue = false;
    }
    else if (content.type == TAG_VALUE_CONFIRM) {
        content.tagValueConfirm.tagValueList.smallCaseForValue = false;
        // use confirm token for black button
        content.tagValueConfirm.confirmationToken = CONFIRM_TOKEN;
    }

    pageContext = nbgl_pageDrawGenericContent(&pageCallback, &navInfo, &content);

    if (forceFullRefresh) {
        nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
    }
    else {
        nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
    }
}

// Helper that does the computing of which nbgl_content_t and which element in the content should be
// displayed for the next generic context navigation page
static const nbgl_content_t *genericContextComputeNextPageParams(uint8_t         pageIdx,
                                                                 nbgl_content_t *content,
                                                                 uint8_t *p_nbElementsInNextPage,
                                                                 bool    *p_flag)
{
    int8_t  nextContentIdx = genericContext.currentContentIdx;
    int16_t nextElementIdx = genericContext.currentElementIdx;
    uint8_t nbElementsInNextPage;

    // Retrieve info on the next page
    genericContextGetPageInfo(pageIdx, &nbElementsInNextPage, p_flag);
    *p_nbElementsInNextPage = nbElementsInNextPage;

    // Handle forward navigation:
    // add to current index the number of pairs of the currently active page
    if (pageIdx > navInfo.activePage) {
        uint8_t nbElementsInCurrentPage;

        genericContextGetPageInfo(navInfo.activePage, &nbElementsInCurrentPage, NULL);
        nextElementIdx += nbElementsInCurrentPage;

        // Handle case where the content to be displayed is in the next content
        // In such case start at element index 0.
        if (nextElementIdx >= genericContext.currentContentElementNb) {
            nextContentIdx += 1;
            nextElementIdx = 0;
        }
    }

    // Handle backward navigation:
    // add to current index the number of pairs of the currently active page
    if (pageIdx < navInfo.activePage) {
        // Backward navigation: remove to current index the number of pairs of the current page
        nextElementIdx -= nbElementsInNextPage;

        // Handle case where the content to be displayed is in the previous content
        // In such case set a negative number as element index so that it is handled
        // later once the previous content is accessible so that its elements number
        // can be retrieved.
        if (nextElementIdx < 0) {
            nextContentIdx -= 1;
            nextElementIdx = -nbElementsInNextPage;
        }
    }

    const nbgl_content_t *p_content;
    // Retrieve next content
    if ((nextContentIdx == -1) && (genericContext.hasStartingContent)) {
        p_content = &STARTING_CONTENT;
    }
    else if ((nextContentIdx == genericContext.genericContents.nbContents)
             && (genericContext.hasFinishingContent)) {
        p_content = &FINISHING_CONTENT;
    }
    else {
        p_content = getContentAtIdx(&genericContext.genericContents, nextContentIdx, content);

        if (p_content == NULL) {
            LOG_DEBUG(USE_CASE_LOGGER, "Fail to retrieve content\n");
            return NULL;
        }
    }

    // Handle cases where we are going to display data from a new content:
    // - navigation to a previous or to the next content
    // - First page display (genericContext.currentContentElementNb == 0)
    //
    // In such case we need to:
    // - Update genericContext.currentContentIdx
    // - Update genericContext.currentContentElementNb
    // - Update onContentAction callback
    // - Update nextElementIdx in case it was previously not calculable
    if ((nextContentIdx != genericContext.currentContentIdx)
        || (genericContext.currentContentElementNb == 0)) {
        genericContext.currentContentIdx       = nextContentIdx;
        genericContext.currentContentElementNb = getContentNbElement(p_content);
        onContentAction                        = PIC(p_content->contentActionCallback);
        if (nextElementIdx < 0) {
            nextElementIdx = genericContext.currentContentElementNb + nextElementIdx;
        }
    }

    // Sanity check
    if ((nextElementIdx < 0) || (nextElementIdx >= genericContext.currentContentElementNb)) {
        LOG_DEBUG(USE_CASE_LOGGER,
                  "Invalid element index %d / %d\n",
                  nextElementIdx,
                  genericContext.currentContentElementNb);
        return NULL;
    }

    // Update genericContext elements
    genericContext.currentElementIdx = nextElementIdx;
    navInfo.activePage               = pageIdx;

    return p_content;
}

// Helper that generates a nbgl_pageContent_t to be displayed for the next generic context
// navigation page
static bool genericContextPreparePageContent(const nbgl_content_t *p_content,
                                             uint8_t               nbElementsInPage,
                                             bool                  flag,
                                             nbgl_pageContent_t   *pageContent)
{
    uint8_t nextElementIdx = genericContext.currentElementIdx;

    pageContent->title            = pageTitle;
    pageContent->isTouchableTitle = false;
    pageContent->titleToken       = QUIT_TOKEN;
    pageContent->tuneId           = TUNE_TAP_CASUAL;

    pageContent->type = p_content->type;
    switch (pageContent->type) {
        case CENTERED_INFO:
            memcpy(&pageContent->centeredInfo,
                   &p_content->content.centeredInfo,
                   sizeof(pageContent->centeredInfo));
            break;
        case INFO_LONG_PRESS:
            memcpy(&pageContent->infoLongPress,
                   &p_content->content.infoLongPress,
                   sizeof(pageContent->infoLongPress));
            break;

        case INFO_BUTTON:
            memcpy(&pageContent->infoButton,
                   &p_content->content.infoButton,
                   sizeof(pageContent->infoButton));
            break;

        case TAG_VALUE_LIST: {
            nbgl_contentTagValueList_t *p_tagValueList = &pageContent->tagValueList;

            // memorize pairs (or callback) for usage when alias is used
            genericContext.currentPairs    = p_content->content.tagValueList.pairs;
            genericContext.currentCallback = p_content->content.tagValueList.callback;

            if (flag) {
                // Flag can be set if the pair is too long to fit or because it needs
                // to be displayed as centered info.

                // First retrieve the pair
                const nbgl_layoutTagValue_t *pair;

                if (p_content->content.tagValueList.pairs != NULL) {
                    pair = PIC(&p_content->content.tagValueList.pairs[nextElementIdx]);
                }
                else {
                    pair = PIC(p_content->content.tagValueList.callback(nextElementIdx));
                }

                if (pair->centeredInfo) {
                    pageContent->type = CENTERED_INFO;
                    prepareReviewFirstPage(
                        &pageContent->centeredInfo, pair->valueIcon, pair->item, pair->value);

                    // Skip population of nbgl_contentTagValueList_t structure
                    p_tagValueList = NULL;
                }
                else {
                    // if the pair is too long to fit, we use a TAG_VALUE_DETAILS content
                    pageContent->type                               = TAG_VALUE_DETAILS;
                    pageContent->tagValueDetails.detailsButtonText  = "More";
                    pageContent->tagValueDetails.detailsButtonIcon  = NULL;
                    pageContent->tagValueDetails.detailsButtonToken = DETAILS_BUTTON_TOKEN;

                    p_tagValueList = &pageContent->tagValueDetails.tagValueList;

                    // Backup pair info for easy data access upon click on button
                    genericContext.detailsItem     = pair->item;
                    genericContext.detailsvalue    = pair->value;
                    genericContext.detailsWrapping = p_content->content.tagValueList.wrapping;
                }
            }

            if (p_tagValueList != NULL) {
                p_tagValueList->nbPairs = nbElementsInPage;
                p_tagValueList->token   = p_content->content.tagValueList.token;
                if (p_content->content.tagValueList.pairs != NULL) {
                    p_tagValueList->pairs
                        = PIC(&p_content->content.tagValueList.pairs[nextElementIdx]);
                    // parse pairs to check if any contains an alias for value
                    for (uint8_t i = 0; i < nbElementsInPage; i++) {
                        if (p_tagValueList->pairs[i].aliasValue) {
                            p_tagValueList->token = VALUE_ALIAS_TOKEN;
                            break;
                        }
                    }
                }
                else {
                    p_tagValueList->pairs      = NULL;
                    p_tagValueList->callback   = p_content->content.tagValueList.callback;
                    p_tagValueList->startIndex = nextElementIdx;
                    // parse pairs to check if any contains an alias for value
                    for (uint8_t i = 0; i < nbElementsInPage; i++) {
                        const nbgl_layoutTagValue_t *pair
                            = PIC(p_content->content.tagValueList.callback(nextElementIdx + i));
                        if (pair->aliasValue) {
                            p_tagValueList->token = VALUE_ALIAS_TOKEN;
                            break;
                        }
                    }
                }
                p_tagValueList->smallCaseForValue  = false;
                p_tagValueList->nbMaxLinesForValue = NB_MAX_LINES_IN_REVIEW;
                p_tagValueList->wrapping           = p_content->content.tagValueList.wrapping;
            }

            break;
        }
        case TAG_VALUE_CONFIRM:
            memcpy(&pageContent->tagValueConfirm,
                   &p_content->content.tagValueConfirm,
                   sizeof(pageContent->tagValueConfirm));
            break;
        case SWITCHES_LIST:
            pageContent->switchesList.nbSwitches = nbElementsInPage;
            pageContent->switchesList.switches
                = PIC(&p_content->content.switchesList.switches[nextElementIdx]);
            break;
        case INFOS_LIST:
            pageContent->infosList.nbInfos = nbElementsInPage;
            pageContent->infosList.infoTypes
                = PIC(&p_content->content.infosList.infoTypes[nextElementIdx]);
            pageContent->infosList.infoContents
                = PIC(&p_content->content.infosList.infoContents[nextElementIdx]);
            break;
        case CHOICES_LIST:
            memcpy(&pageContent->choicesList,
                   &p_content->content.choicesList,
                   sizeof(pageContent->choicesList));
            pageContent->choicesList.nbChoices = nbElementsInPage;
            pageContent->choicesList.names
                = PIC(&p_content->content.choicesList.names[nextElementIdx]);
            if ((p_content->content.choicesList.initChoice > nextElementIdx)
                && (p_content->content.choicesList.initChoice
                    <= nextElementIdx + nbElementsInPage)) {
                pageContent->choicesList.initChoice
                    = p_content->content.choicesList.initChoice - nextElementIdx;
            }
            else {
                pageContent->choicesList.initChoice = nbElementsInPage;
            }
            break;
        case BARS_LIST:
            pageContent->barsList.nbBars = nbElementsInPage;
            pageContent->barsList.barTexts
                = PIC(&p_content->content.barsList.barTexts[nextElementIdx]);
            pageContent->barsList.tokens = PIC(&p_content->content.barsList.tokens[nextElementIdx]);
            pageContent->barsList.tuneId = p_content->content.barsList.tuneId;
            break;
        default:
            LOG_DEBUG(USE_CASE_LOGGER, "Unsupported type %d\n", pageContent->type);
            return false;
    }
    // if first or last page of review and blind operation, add the top-right button
    if (((p_content == &STARTING_CONTENT) || (p_content->type == INFO_LONG_PRESS))
        && (((navType == STREAMING_NAV)
             && (bundleNavContext.reviewStreaming.operationType & BLIND_OPERATION))
            || ((navType == GENERIC_NAV)
                && (bundleNavContext.review.operationType & BLIND_OPERATION)))) {
        pageContent->topRightIcon  = &WARNING_ICON;
        pageContent->topRightToken = BLIND_WARNING_TOKEN;
    }

    return true;
}

// function used to display the current page in generic context navigation mode
static void displayGenericContextPage(uint8_t pageIdx, bool forceFullRefresh)
{
    // Retrieve next page parameters
    nbgl_content_t        content;
    uint8_t               nbElementsInPage;
    bool                  flag;
    const nbgl_content_t *p_content = NULL;

    if (navType == STREAMING_NAV) {
        if (pageIdx == LAST_PAGE_FOR_REVIEW) {
            if (bundleNavContext.reviewStreaming.skipCallback != NULL) {
                bundleNavContext.reviewStreaming.skipCallback();
            }
            return;
        }
        else if (pageIdx >= bundleNavContext.reviewStreaming.stepPageNb) {
            bundleNavReviewStreamingChoice(true);
            return;
        }
    }

    if (navInfo.activePage == pageIdx) {
        p_content
            = genericContextComputeNextPageParams(pageIdx, &content, &nbElementsInPage, &flag);
    }
    else if (navInfo.activePage < pageIdx) {
        // Support going more than one step forward.
        // It occurs when initializing a navigation on an arbitrary page
        for (int i = navInfo.activePage + 1; i <= pageIdx; i++) {
            p_content = genericContextComputeNextPageParams(i, &content, &nbElementsInPage, &flag);
        }
    }
    else {
        if (pageIdx - navInfo.activePage > 1) {
            // We don't support going more than one step backward as it doesn't occurs for now?
            LOG_DEBUG(USE_CASE_LOGGER, "Unsupported navigation\n");
            return;
        }
        p_content
            = genericContextComputeNextPageParams(pageIdx, &content, &nbElementsInPage, &flag);
    }

    if (p_content == NULL) {
        return;
    }

    // Create next page content
    nbgl_pageContent_t pageContent = {0};
    if (!genericContextPreparePageContent(p_content, nbElementsInPage, flag, &pageContent)) {
        return;
    }

    pageContext = nbgl_pageDrawGenericContent(&pageCallback, &navInfo, &pageContent);

    if (forceFullRefresh) {
        nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
    }
    else {
        nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
    }
}

// from the current details context, return a pointer on the details at the given page
static const char *getDetailsPageAt(uint8_t detailsPage)
{
    uint8_t     page        = 0;
    const char *currentChar = detailsContext.value;
    while (page < detailsPage) {
        uint16_t nbLines
            = nbgl_getTextNbLinesInWidth(SMALL_BOLD_FONT, currentChar, AVAILABLE_WIDTH, false);
        if (nbLines > NB_MAX_LINES_IN_DETAILS) {
            uint16_t len;
            nbgl_getTextMaxLenInNbLines(SMALL_BOLD_FONT,
                                        currentChar,
                                        AVAILABLE_WIDTH,
                                        NB_MAX_LINES_IN_DETAILS,
                                        &len,
                                        false);
            len -= 3;
            currentChar = currentChar + len;
        }
        page++;
    }
    return currentChar;
}

// function used to display the current page in details review mode
static void displayDetailsPage(uint8_t detailsPage, bool forceFullRefresh)
{
    static nbgl_layoutTagValue_t currentPair;
    nbgl_pageNavigationInfo_t    info    = {.activePage                = detailsPage,
                                            .nbPages                   = detailsContext.nbPages,
                                            .navType                   = NAV_WITH_BUTTONS,
                                            .quitToken                 = QUIT_TOKEN,
                                            .navWithButtons.navToken   = NAV_TOKEN,
                                            .navWithButtons.quitButton = true,
                                            .navWithButtons.backButton = true,
                                            .navWithButtons.quitText   = NULL,
                                            .progressIndicator         = false,
                                            .tuneId                    = TUNE_TAP_CASUAL};
    nbgl_pageContent_t           content = {.type                           = TAG_VALUE_LIST,
                                            .topRightIcon                   = NULL,
                                            .tagValueList.nbPairs           = 1,
                                            .tagValueList.pairs             = &currentPair,
                                            .tagValueList.smallCaseForValue = true,
                                            .tagValueList.wrapping = detailsContext.wrapping};

    if (modalPageContext != NULL) {
        nbgl_pageRelease(modalPageContext);
    }
    currentPair.item = detailsContext.tag;
    // if move backward or first page
    if (detailsPage <= detailsContext.currentPage) {
        // recompute current start from beginning
        currentPair.value = getDetailsPageAt(detailsPage);
        forceFullRefresh  = true;
    }
    // else move forward
    else {
        currentPair.value = detailsContext.nextPageStart;
    }
    detailsContext.currentPage = detailsPage;
    uint16_t nbLines
        = nbgl_getTextNbLinesInWidth(SMALL_BOLD_FONT, currentPair.value, AVAILABLE_WIDTH, false);

    if (nbLines > NB_MAX_LINES_IN_DETAILS) {
        uint16_t len;
        nbgl_getTextMaxLenInNbLines(SMALL_BOLD_FONT,
                                    currentPair.value,
                                    AVAILABLE_WIDTH,
                                    NB_MAX_LINES_IN_DETAILS,
                                    &len,
                                    false);
        len -= 3;
        // memorize next position to save processing
        detailsContext.nextPageStart = currentPair.value + len;
        // use special feature to keep only NB_MAX_LINES_IN_DETAILS lines and replace the last 3
        // chars by "..."
        content.tagValueList.nbMaxLinesForValue = NB_MAX_LINES_IN_DETAILS;
    }
    else {
        detailsContext.nextPageStart            = NULL;
        content.tagValueList.nbMaxLinesForValue = 0;
    }
    if (info.nbPages == 1) {
        // if only one page, no navigation bar, and use a footer instead
        info.navWithButtons.quitText = "Close";
    }
    modalPageContext = nbgl_pageDrawGenericContentExt(&pageModalCallback, &info, &content, true);

    if (forceFullRefresh) {
        nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
    }
    else {
        nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
    }
}

// function used to display the content of a full value, when touching an alias of a tag/value pair
static void displayFullValuePage(const nbgl_contentTagValue_t *pair)
{
    nbgl_layoutDescription_t layoutDescription = {.modal            = true,
                                                  .withLeftBorder   = true,
                                                  .onActionCallback = &modalLayoutTouchCallback,
                                                  .tapActionText    = NULL};
    nbgl_layoutHeader_t      headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                  .separationLine     = false,
                                                  .backAndText.token  = 0,
                                                  .backAndText.tuneId = TUNE_TAP_CASUAL,
                                                  .backAndText.text   = PIC(pair->item)};
    const char              *info;
    genericContext.modalLayout = nbgl_layoutGet(&layoutDescription);
    // add header with the tag part of the pair, to go back
    nbgl_layoutAddHeader(genericContext.modalLayout, &headerDesc);
    // add full value text
    if (pair->extension->explanation == NULL) {
        if (pair->extension->aliasType == ENS_ALIAS) {
            info = "ENS names are resolved by Ledger backend.";
        }
        else if (pair->extension->aliasType == ADDRESS_BOOK_ALIAS) {
            info = "This account label comes from your Address Book in Ledger Live.";
        }
        else {
            info = "";
        }
    }
    else {
        info = pair->extension->explanation;
    }
    nbgl_layoutAddTextContent(
        genericContext.modalLayout, pair->value, pair->extension->fullValue, info);

    // draw & refresh
    nbgl_layoutDraw(genericContext.modalLayout);
    nbgl_refresh();
}

// function used to display the modal warning when touching the alert symbol of a blind review
static void displayBlindWarning(nbgl_opType_t opType)
{
    nbgl_layoutDescription_t  layoutDescription = {.modal            = true,
                                                   .withLeftBorder   = true,
                                                   .onActionCallback = &modalLayoutTouchCallback,
                                                   .tapActionText    = NULL};
    nbgl_layoutHeader_t       headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                   .separationLine     = false,
                                                   .backAndText.token  = 0,
                                                   .backAndText.tuneId = TUNE_TAP_CASUAL,
                                                   .backAndText.text   = NULL};
    nbgl_layoutCenteredInfo_t centeredInfo
        = {.icon = NULL, .text3 = NULL, .style = LARGE_CASE_INFO, .offsetY = 0, .onTop = false};
    if (opType == TYPE_TRANSACTION) {
        centeredInfo.text1 = "This transaction cannot be trusted";
        centeredInfo.text2
            = "Your Ledger cannot decode this transaction. If you sign it, you could be "
              "authorizing "
              "malicious actions that can drain your wallet.\n\n"
              "Learn more: ledger.com/e8";
    }
    else {
        centeredInfo.text1 = "This message cannot be trusted";
        centeredInfo.text2
            = "Your Ledger cannot decode this message. If you sign it, you could be authorizing "
              "malicious actions that can drain your wallet.\n\n"
              "Learn more: ledger.com/e8";
    }
    genericContext.modalLayout = nbgl_layoutGet(&layoutDescription);
    // add header with the tag part of the pair, to go back
    nbgl_layoutAddHeader(genericContext.modalLayout, &headerDesc);
    // add full value text
    nbgl_layoutAddCenteredInfo(genericContext.modalLayout, &centeredInfo);

    // draw & refresh
    nbgl_layoutDraw(genericContext.modalLayout);
    nbgl_refresh();
}

#ifdef NBGL_QRCODE
static void displayAddressQRCode(void)
{
    // display the address as QR Code
    nbgl_layoutDescription_t layoutDescription = {.modal            = true,
                                                  .withLeftBorder   = true,
                                                  .onActionCallback = &addressLayoutTouchCallbackQR,
                                                  .tapActionText    = NULL};
    nbgl_layoutHeader_t      headerDesc        = {
                    .type = HEADER_EMPTY, .separationLine = false, .emptySpace.height = SMALL_CENTERING_HEADER};
    nbgl_layoutQRCode_t qrCode = {.url      = addressConfirmationContext.tagValuePair.value,
                                  .text1    = NULL,
                                  .centered = true,
                                  .offsetY  = 0};

    addressConfirmationContext.modalLayout = nbgl_layoutGet(&layoutDescription);
    // add empty header for better look
    nbgl_layoutAddHeader(addressConfirmationContext.modalLayout, &headerDesc);
    // compute nb lines to check whether it shall be shorten (max is 3 lines)
    uint16_t nbLines = nbgl_getTextNbLinesInWidth(
        SMALL_REGULAR_FONT, addressConfirmationContext.tagValuePair.value, AVAILABLE_WIDTH, false);

    if (nbLines <= QRCODE_NB_MAX_LINES) {
        qrCode.text2 = addressConfirmationContext.tagValuePair.value;  // in gray
    }
    else {
        // only keep beginning and end of text, and add ... in the middle
        nbgl_textReduceOnNbLines(SMALL_REGULAR_FONT,
                                 addressConfirmationContext.tagValuePair.value,
                                 AVAILABLE_WIDTH,
                                 QRCODE_NB_MAX_LINES,
                                 reducedAddress,
                                 QRCODE_REDUCED_ADDR_LEN);
        qrCode.text2 = reducedAddress;  // in gray
    }

    nbgl_layoutAddQRCode(addressConfirmationContext.modalLayout, &qrCode);

    nbgl_layoutAddFooter(addressConfirmationContext.modalLayout, "Close", 0, TUNE_TAP_CASUAL);
    nbgl_layoutDraw(addressConfirmationContext.modalLayout);
    nbgl_refresh();
}

// called when quit button is touched on Address verification page
static void addressLayoutTouchCallbackQR(int token, uint8_t index)
{
    UNUSED(token);
    UNUSED(index);

    // dismiss modal
    nbgl_layoutRelease(addressConfirmationContext.modalLayout);
    nbgl_screenRedraw();
    nbgl_refresh();
}
#endif  // NBGL_QRCODE

// called when header is touched on modal page, to dismiss it
static void modalLayoutTouchCallback(int token, uint8_t index)
{
    UNUSED(token);
    UNUSED(index);

    // dismiss modal
    nbgl_layoutRelease(genericContext.modalLayout);
    nbgl_screenRedraw();
    nbgl_refresh();
}

// called when skip button is touched in footer, during forward only review
static void displaySkipWarning(void)
{
    nbgl_pageConfirmationDescription_t info = {
        .cancelText         = "Go back to review",
        .centeredInfo.text1 = "Skip review?",
        .centeredInfo.text2
        = "If you're sure you don't need to review all fields, you can skip straight to signing.",
        .centeredInfo.text3   = NULL,
        .centeredInfo.style   = LARGE_CASE_INFO,
        .centeredInfo.icon    = &C_Important_Circle_64px,
        .centeredInfo.offsetY = 0,
        .confirmationText     = "Yes, skip",
        .confirmationToken    = SKIP_TOKEN,
        .tuneId               = TUNE_TAP_CASUAL,
        .modal                = true};
    modalPageContext = nbgl_pageDrawConfirmation(&pageModalCallback, &info);
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
}

#ifdef NBGL_KEYPAD
// called to update the keypad and automatically show / hide:
// - backspace key if no digits are present
// - validation key if the min digit is reached
// - keypad if the max number of digit is reached
static void updateKeyPad(bool add)
{
    bool                enableValidate, enableBackspace, enableDigits;
    bool                redrawKeypad = false;
    nbgl_refresh_mode_t mode         = BLACK_AND_WHITE_FAST_REFRESH;

    enableDigits    = (keypadContext.pinLen < keypadContext.pinMaxDigits);
    enableValidate  = (keypadContext.pinLen >= keypadContext.pinMinDigits);
    enableBackspace = (keypadContext.pinLen > 0);
    if (add) {
        if ((keypadContext.pinLen == keypadContext.pinMinDigits)
            ||  // activate "validate" button on keypad
            (keypadContext.pinLen == keypadContext.pinMaxDigits)
            ||                              // deactivate "digits" on keypad
            (keypadContext.pinLen == 1)) {  // activate "backspace"
            redrawKeypad = true;
        }
    }
    else {                                  // remove
        if ((keypadContext.pinLen == 0) ||  // deactivate "backspace" button on keypad
            (keypadContext.pinLen == (keypadContext.pinMinDigits - 1))
            ||  // deactivate "validate" button on keypad
            (keypadContext.pinLen
             == (keypadContext.pinMaxDigits - 1))) {  // reactivate "digits" on keypad
            redrawKeypad = true;
        }
    }
    if (keypadContext.hidden == true) {
        nbgl_layoutUpdateKeypadContent(keypadContext.layoutCtx, true, keypadContext.pinLen, NULL);
    }
    else {
        nbgl_layoutUpdateKeypadContent(
            keypadContext.layoutCtx, false, 0, (const char *) keypadContext.pinEntry);
    }
    if (redrawKeypad) {
        nbgl_layoutUpdateKeypad(
            keypadContext.layoutCtx, 0, enableValidate, enableBackspace, enableDigits);
    }

    if ((!add) && (keypadContext.pinLen == 0)) {
        // Full refresh to fully clean the bullets when reaching 0 digits
        mode = FULL_COLOR_REFRESH;
    }
    nbgl_refreshSpecialWithPostRefresh(mode, POST_REFRESH_FORCE_POWER_ON);
}

// called when a key is touched on the keypad
static void keypadCallback(char touchedKey)
{
    switch (touchedKey) {
        case BACKSPACE_KEY:
            if (keypadContext.pinLen > 0) {
                keypadContext.pinLen--;
                keypadContext.pinEntry[keypadContext.pinLen] = 0;
            }
            updateKeyPad(false);
            break;

        case VALIDATE_KEY:
            // Gray out keyboard / buttons as a first user feedback
            nbgl_layoutUpdateKeypad(keypadContext.layoutCtx, 0, false, false, true);
            nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_FAST_REFRESH,
                                               POST_REFRESH_FORCE_POWER_ON);

            onValidatePin(keypadContext.pinEntry, keypadContext.pinLen);
            break;

        default:
            if ((touchedKey >= 0x30) && (touchedKey < 0x40)) {
                if (keypadContext.pinLen < keypadContext.pinMaxDigits) {
                    keypadContext.pinEntry[keypadContext.pinLen] = touchedKey;
                    keypadContext.pinLen++;
                }
                updateKeyPad(true);
            }
            break;
    }
}

// called to create a keypad, with either hidden or visible digits
static void keypadGenericUseCase(const char                *title,
                                 uint8_t                    minDigits,
                                 uint8_t                    maxDigits,
                                 uint8_t                    backToken,
                                 bool                       shuffled,
                                 bool                       hidden,
                                 tune_index_e               tuneId,
                                 nbgl_pinValidCallback_t    validatePinCallback,
                                 nbgl_layoutTouchCallback_t actionCallback)
{
    nbgl_layoutDescription_t layoutDescription = {0};
    nbgl_layoutHeader_t      headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                  .separationLine     = true,
                                                  .backAndText.token  = backToken,
                                                  .backAndText.tuneId = tuneId,
                                                  .backAndText.text   = NULL};
    int                      status            = -1;

    if ((minDigits > KEYPAD_MAX_DIGITS) || (maxDigits > KEYPAD_MAX_DIGITS)) {
        return;
    }

    reset_callbacks();
    // reset the keypad context
    memset(&keypadContext, 0, sizeof(KeypadContext_t));

    // get a layout
    layoutDescription.onActionCallback = actionCallback;
    layoutDescription.modal            = false;
    layoutDescription.withLeftBorder   = false;
    keypadContext.layoutCtx            = nbgl_layoutGet(&layoutDescription);
    keypadContext.hidden               = hidden;

    // set back key in header
    nbgl_layoutAddHeader(keypadContext.layoutCtx, &headerDesc);

    // add keypad
    status = nbgl_layoutAddKeypad(keypadContext.layoutCtx, keypadCallback, shuffled);
    if (status < 0) {
        return;
    }
    // add keypad content
    status = nbgl_layoutAddKeypadContent(
        keypadContext.layoutCtx, title, keypadContext.hidden, maxDigits, "");

    if (status < 0) {
        return;
    }

    // validation pin callback
    onValidatePin = validatePinCallback;
    // pin code acceptable lengths
    keypadContext.pinMinDigits = minDigits;
    keypadContext.pinMaxDigits = maxDigits;

    nbgl_layoutDraw(keypadContext.layoutCtx);
    nbgl_refreshSpecialWithPostRefresh(FULL_COLOR_CLEAN_REFRESH, POST_REFRESH_FORCE_POWER_ON);
}
#endif

static uint8_t nbgl_useCaseGetNbPagesForContent(const nbgl_content_t *content, uint8_t pageIdxStart)
{
    uint8_t nbElements = 0;
    uint8_t nbPages    = 0;
    uint8_t nbElementsInPage;
    uint8_t elemIdx = 0;
    bool    flag;

    nbElements = getContentNbElement(content);

    while (nbElements > 0) {
        if (content->type == TAG_VALUE_LIST) {
            nbElementsInPage = nbgl_useCaseGetNbTagValuesInPage(
                nbElements, &content->content.tagValueList, elemIdx, &flag);
        }
        else {
            nbElementsInPage = MIN(
                nbMaxElementsPerContentType[content->type],
                nbElements);  // TODO hardcoded to 3 for now but should be dynamically computed
            flag = 0;
        }

        elemIdx += nbElementsInPage;
        genericContextSetPageInfo(pageIdxStart + nbPages, nbElementsInPage, flag);
        nbElements -= nbElementsInPage;
        nbPages++;
    }

    return nbPages;
}

static uint8_t nbgl_useCaseGetNbPagesForGenericContents(
    const nbgl_genericContents_t *genericContents,
    uint8_t                       pageIdxStart)
{
    uint8_t               nbPages = 0;
    nbgl_content_t        content;
    const nbgl_content_t *p_content;

    for (int i = 0; i < genericContents->nbContents; i++) {
        p_content = getContentAtIdx(genericContents, i, &content);
        if (p_content == NULL) {
            return 0;
        }
        nbPages += nbgl_useCaseGetNbPagesForContent(p_content, pageIdxStart + nbPages);
    }

    return nbPages;
}

static void prepareAddressConfirmationPages(const char                       *address,
                                            const nbgl_contentTagValueList_t *tagValueList,
                                            nbgl_content_t                   *firstPageContent,
                                            nbgl_content_t                   *secondPageContent)
{
    nbgl_contentTagValueConfirm_t *tagValueConfirm;

    addressConfirmationContext.tagValuePair.item  = "Address";
    addressConfirmationContext.tagValuePair.value = address;

    // First page
    firstPageContent->type = TAG_VALUE_CONFIRM;
    tagValueConfirm        = &firstPageContent->content.tagValueConfirm;

#ifdef NBGL_QRCODE
    tagValueConfirm->detailsButtonIcon = &QRCODE_ICON;
    // only use "Show as QR" when it's not the last page
    if (tagValueList != NULL) {
        tagValueConfirm->detailsButtonText = "Show as QR";
    }
    else {
        tagValueConfirm->detailsButtonText = NULL;
    }
    tagValueConfirm->detailsButtonToken = ADDRESS_QRCODE_BUTTON_TOKEN;
#else   // NBGL_QRCODE
    tagValueConfirm->detailsButtonText = NULL;
    tagValueConfirm->detailsButtonIcon = NULL;
#endif  // NBGL_QRCODE
    tagValueConfirm->tuneId                          = TUNE_TAP_CASUAL;
    tagValueConfirm->tagValueList.nbPairs            = 1;
    tagValueConfirm->tagValueList.pairs              = &addressConfirmationContext.tagValuePair;
    tagValueConfirm->tagValueList.smallCaseForValue  = false;
    tagValueConfirm->tagValueList.nbMaxLinesForValue = 0;
    tagValueConfirm->tagValueList.wrapping           = false;
    // if it's an extended address verif, it takes 2 pages, so display a "Tap to continue", and
    // no confirmation button
    if (tagValueList != NULL) {
        tagValueConfirm->confirmationText = NULL;
    }
    else {
        // otherwise no tap to continue but a confirmation button
        tagValueConfirm->confirmationText  = "Confirm";
        tagValueConfirm->confirmationToken = CONFIRM_TOKEN;
    }

    // Second page if any:
    if (tagValueList != NULL) {
        // the second page is dedicated to the extended tag/value pairs
        secondPageContent->type            = TAG_VALUE_CONFIRM;
        tagValueConfirm                    = &secondPageContent->content.tagValueConfirm;
        tagValueConfirm->confirmationText  = "Confirm";
        tagValueConfirm->confirmationToken = CONFIRM_TOKEN;
        tagValueConfirm->detailsButtonText = NULL;
        tagValueConfirm->detailsButtonIcon = NULL;
        tagValueConfirm->tuneId            = TUNE_TAP_CASUAL;
        memcpy(&tagValueConfirm->tagValueList, tagValueList, sizeof(nbgl_contentTagValueList_t));
    }
}

static void bundleNavStartHome(void)
{
    nbgl_homeAndSettingsContext_t *context = &bundleNavContext.homeAndSettings;

    nbgl_useCaseHomeExt(context->appName,
                        context->appIcon,
                        context->tagline,
                        context->settingContents != NULL ? true : false,
                        context->actionText,
                        context->actionCallback,
                        bundleNavStartSettings,
                        context->quitCallback);
}

static void bundleNavStartSettingsAtPage(uint8_t initSettingPage)
{
    nbgl_homeAndSettingsContext_t *context = &bundleNavContext.homeAndSettings;

    nbgl_useCaseGenericSettings(context->appName,
                                initSettingPage,
                                context->settingContents,
                                context->infosList,
                                bundleNavStartHome);
}

static void bundleNavStartSettings(void)
{
    bundleNavStartSettingsAtPage(0);
}

static void bundleNavReviewConfirmRejection(void)
{
    bundleNavContext.review.choiceCallback(false);
}

static void bundleNavReviewAskRejectionConfirmation(nbgl_operationType_t operationType,
                                                    nbgl_callback_t      callback)
{
    const char *title;
    const char *confirmText;
    // clear skip and blind bits
    operationType &= ~(SKIPPABLE_OPERATION | BLIND_OPERATION);
    if (operationType == TYPE_TRANSACTION) {
        title       = "Reject transaction?";
        confirmText = "Go back to transaction";
    }
    else if (operationType == TYPE_MESSAGE) {
        title       = "Reject message?";
        confirmText = "Go back to message";
    }
    else {
        title       = "Reject operation?";
        confirmText = "Go back to operation";
    }

    // display a choice to confirm/cancel rejection
    nbgl_useCaseConfirm(title, NULL, "Yes, reject", confirmText, callback);
}

static void bundleNavReviewChoice(bool confirm)
{
    if (confirm) {
        bundleNavContext.review.choiceCallback(true);
    }
    else {
        bundleNavReviewAskRejectionConfirmation(bundleNavContext.review.operationType,
                                                bundleNavReviewConfirmRejection);
    }
}

static void bundleNavReviewStreamingConfirmRejection(void)
{
    bundleNavContext.reviewStreaming.choiceCallback(false);
}

static void bundleNavReviewStreamingChoice(bool confirm)
{
    if (confirm) {
        // Display a spinner if it wasn't the finish step
        if (STARTING_CONTENT.type != INFO_LONG_PRESS) {
            nbgl_useCaseSpinner("Processing");
        }
        bundleNavContext.reviewStreaming.choiceCallback(true);
    }
    else {
        bundleNavReviewAskRejectionConfirmation(bundleNavContext.reviewStreaming.operationType,
                                                bundleNavReviewStreamingConfirmRejection);
    }
}

static void useCaseReview(nbgl_operationType_t              operationType,
                          const nbgl_contentTagValueList_t *tagValueList,
                          const nbgl_icon_details_t        *icon,
                          const char                       *reviewTitle,
                          const char                       *reviewSubTitle,
                          const char                       *finishTitle,
                          nbgl_choiceCallback_t             choiceCallback,
                          bool                              isLight)
{
    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));

    bundleNavContext.review.operationType  = operationType;
    bundleNavContext.review.choiceCallback = choiceCallback;

    // memorize context
    onChoice  = bundleNavReviewChoice;
    navType   = GENERIC_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    genericContext.genericContents.nbContents   = 3;
    memset(localContentsList, 0, 3 * sizeof(nbgl_content_t));

    // First a centered info
    STARTING_CONTENT.type = CENTERED_INFO;
    prepareReviewFirstPage(
        &STARTING_CONTENT.content.centeredInfo, icon, reviewTitle, reviewSubTitle);

    // Then the tag/value pairs
    localContentsList[1].type = TAG_VALUE_LIST;
    memcpy(&localContentsList[1].content.tagValueList,
           tagValueList,
           sizeof(nbgl_contentTagValueList_t));
    localContentsList[1].contentActionCallback = tagValueList->actionCallback;

    // The last page
    if (isLight) {
        localContentsList[2].type = INFO_BUTTON;
        prepareReviewLightLastPage(&localContentsList[2].content.infoButton, icon, finishTitle);
    }
    else {
        localContentsList[2].type = INFO_LONG_PRESS;
        prepareReviewLastPage(&localContentsList[2].content.infoLongPress, icon, finishTitle);
    }

    // compute number of pages & fill navigation structure
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    prepareNavInfo(true, nbPages, getRejectReviewText(operationType));

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    io_seproxyhal_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    displayGenericContextPage(0, true);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief computes the number of tag/values pairs displayable in a page, with the given list of
 * tag/value pairs
 *
 * @param nbPairs number of tag/value pairs to use in \b tagValueList
 * @param tagValueList list of tag/value pairs
 * @param startIndex first index to consider in \b tagValueList
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
    uint8_t nbPairsInPage = 0;
#ifdef TARGET_STAX
    uint16_t currentHeight = 24;  // upper margin
#else                             // TARGET_STAX
    uint16_t currentHeight             = 0;  // upper margin
#endif                            // TARGET_STAX

    *requireSpecificDisplay = false;
    while (nbPairsInPage < nbPairs) {
        const nbgl_layoutTagValue_t *pair;
        nbgl_font_id_e               value_font;

        // margin between pairs
        // 12 or 24 px between each tag/value pair
        if (nbPairsInPage > 0) {
#ifdef TARGET_STAX
            currentHeight += 12;
#else   // TARGET_STAX
            currentHeight += 24;
#endif  // TARGET_STAX
        }
        // fetch tag/value pair strings.
        if (tagValueList->pairs != NULL) {
            pair = PIC(&tagValueList->pairs[startIndex + nbPairsInPage]);
        }
        else {
            pair = PIC(tagValueList->callback(startIndex + nbPairsInPage));
        }

        if (pair->forcePageStart && nbPairsInPage > 0) {
            // This pair must be at the top of a page
            break;
        }

        if (pair->centeredInfo) {
            if (nbPairsInPage > 0) {
                // This pair must be at the top of a page
                break;
            }
            else {
                // This pair is the only one of the page and has a specific display behavior
                nbPairsInPage           = 1;
                *requireSpecificDisplay = true;
                break;
            }
        }

        // tag height
        currentHeight += nbgl_getTextHeightInWidth(
            SMALL_REGULAR_FONT, pair->item, AVAILABLE_WIDTH, tagValueList->wrapping);
        // space between tag and value
        currentHeight += 4;
        // set value font
        if (tagValueList->smallCaseForValue) {
            value_font = SMALL_REGULAR_FONT;
        }
        else {
            value_font = LARGE_MEDIUM_FONT;
        }
        // value height
        currentHeight += nbgl_getTextHeightInWidth(
            value_font, pair->value, AVAILABLE_WIDTH, tagValueList->wrapping);
        if (currentHeight >= TAG_VALUE_AREA_HEIGHT) {
            if (nbPairsInPage == 0) {
                // Pair too long to fit in a single screen
                // It will be the only one of the page and has a specific display behavior
                nbPairsInPage           = 1;
                *requireSpecificDisplay = true;
            }
            break;
        }
        nbPairsInPage++;
    }
    return nbPairsInPage;
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
        nbPairsInPage = nbgl_useCaseGetNbTagValuesInPage(nbPairs, tagValueList, i, &flag);
        i += nbPairsInPage;
        nbPairs -= nbPairsInPage;
        nbPages++;
    }
    return nbPages;
}

/**
 * @brief draws the home page of an app (page on which we land when launching it from dashboard)
 *
 * @param appName app name
 * @param appIcon app icon
 * @param tagline text under app name (if NULL, it will be "This app enables signing transactions on
 * the <appName> network.")
 * @param withSettings if true, use a "settings" (wheel) icon in bottom button, otherwise a "info"
 * (i)
 * @param topRightCallback callback called when top-right button is touched
 * @param quitCallback callback called when quit button is touched
 */
void nbgl_useCaseHome(const char                *appName,
                      const nbgl_icon_details_t *appIcon,
                      const char                *tagline,
                      bool                       withSettings,
                      nbgl_callback_t            topRightCallback,
                      nbgl_callback_t            quitCallback)
{
    nbgl_useCaseHomeExt(
        appName, appIcon, tagline, withSettings, NULL, NULL, topRightCallback, quitCallback);
}

/**
 * @brief draws the extended version of home page of an app (page on which we land when launching it
 * from dashboard)
 * @note it enables to use an action button (black on Stax, white on Flex)
 *
 * @param appName app name
 * @param appIcon app icon
 * @param tagline text under app name (if NULL, it will be "This app enables signing transactions on
 * the <appName> network.")
 * @param withSettings if true, use a "settings" (wheel) icon in bottom button, otherwise a "info"
 * (i)
 * @param actionButtonText if not NULL, text used for an action button (on top of "Quit
 * App" button/footer)
 * @param actionCallback callback called when action button is touched (if actionButtonText is not
 * NULL)
 * @param topRightCallback callback called when top-right button is touched
 * @param quitCallback callback called when quit button is touched
 */
void nbgl_useCaseHomeExt(const char                *appName,
                         const nbgl_icon_details_t *appIcon,
                         const char                *tagline,
                         bool                       withSettings,
                         const char                *actionButtonText,
                         nbgl_callback_t            actionCallback,
                         nbgl_callback_t            topRightCallback,
                         nbgl_callback_t            quitCallback)
{
    reset_callbacks();

    nbgl_pageInfoDescription_t info = {.centeredInfo.icon    = appIcon,
                                       .centeredInfo.text1   = appName,
                                       .centeredInfo.text3   = NULL,
                                       .centeredInfo.style   = LARGE_CASE_INFO,
                                       .centeredInfo.offsetY = 0,
                                       .footerText           = NULL,
                                       .bottomButtonStyle    = QUIT_APP_TEXT,
                                       .tapActionText        = NULL,
                                       .topRightStyle    = withSettings ? SETTINGS_ICON : INFO_ICON,
                                       .topRightToken    = CONTINUE_TOKEN,
                                       .actionButtonText = actionButtonText,
                                       .tuneId           = TUNE_TAP_CASUAL};
    if (actionButtonText != NULL) {
        // trick to use ACTION_BUTTON_TOKEN for action and quit, with index used to distinguish
        info.bottomButtonsToken = ACTION_BUTTON_TOKEN;
        onAction                = actionCallback;
    }
    else {
        info.bottomButtonsToken = QUIT_TOKEN;
        onAction                = actionCallback;
    }
    if (tagline == NULL) {
        if (strlen(appName) > MAX_APP_NAME_FOR_SDK_TAGLINE) {
            snprintf(appDescription,
                     APP_DESCRIPTION_MAX_LEN,
                     "This app enables signing\ntransactions on its network.");
        }
        else {
            snprintf(appDescription,
                     APP_DESCRIPTION_MAX_LEN,
                     "%s %s\n%s",
                     TAGLINE_PART1,
                     appName,
                     TAGLINE_PART2);
        }

        // If there is more than 3 lines, it means the appName was split, so we put it on the next
        // line
        if (nbgl_getTextNbLinesInWidth(SMALL_REGULAR_FONT, appDescription, AVAILABLE_WIDTH, false)
            > 3) {
            snprintf(appDescription,
                     APP_DESCRIPTION_MAX_LEN,
                     "%s\n%s %s",
                     TAGLINE_PART1,
                     appName,
                     TAGLINE_PART2);
        }
        info.centeredInfo.text2 = appDescription;
    }
    else {
        info.centeredInfo.text2 = tagline;
    }

    onContinue = topRightCallback;
    onQuit     = quitCallback;
    if (actionButtonText != NULL) {
        info.centeredInfo.offsetY -= 40;
    }
    pageContext = nbgl_pageDrawInfo(&pageCallback, NULL, &info);
    nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
}

/**
 * @brief draws the home page of a plug-in app (page on which we land when launching it from
 * dashboard)
 *
 * @param plugInName plug-in app name
 * @param appName master app name (app used by plug-in)
 * @param appIcon master app icon
 * @param tagline text under plug-in name (if NULL, it will be "This app confirms actions
 * for\n<plugInName>.")
 * @param subTagline text under master app icon (if NULL, it will be "This app relies
 * on\n<appName>")
 * @param withSettings if true, use a "settings" (wheel) icon in bottom button, otherwise a "info"
 * (i)
 * @param topRightCallback callback called when top-right button is touched
 * @param quitCallback callback called when quit button is touched
 */
void nbgl_useCasePlugInHome(const char                *plugInName,
                            const char                *appName,
                            const nbgl_icon_details_t *appIcon,
                            const char                *tagline,
                            const char                *subTagline,
                            bool                       withSettings,
                            nbgl_callback_t            topRightCallback,
                            nbgl_callback_t            quitCallback)
{
    reset_callbacks();

    nbgl_pageInfoDescription_t info = {.centeredInfo.icon    = appIcon,
                                       .centeredInfo.text1   = plugInName,
                                       .centeredInfo.style   = PLUGIN_INFO,
                                       .centeredInfo.offsetY = -16,
                                       .footerText           = NULL,
                                       .bottomButtonStyle    = QUIT_APP_TEXT,
                                       .tapActionText        = NULL,
                                       .topRightStyle    = withSettings ? SETTINGS_ICON : INFO_ICON,
                                       .topRightToken    = CONTINUE_TOKEN,
                                       .actionButtonText = NULL,
                                       .tuneId           = TUNE_TAP_CASUAL};
    info.bottomButtonsToken         = QUIT_TOKEN;
    onAction                        = NULL;
    if (tagline == NULL) {
        snprintf(appDescription,
                 APP_DESCRIPTION_MAX_LEN,
                 "This app confirms actions for\n%s.",
                 plugInName);
        info.centeredInfo.text2 = appDescription;
    }
    else {
        info.centeredInfo.text2 = tagline;
    }
    if (subTagline == NULL) {
        snprintf(plugInDescription, APP_DESCRIPTION_MAX_LEN, "This app relies on\n%s", appName);
        info.centeredInfo.text3 = plugInDescription;
    }
    else {
        info.centeredInfo.text3 = subTagline;
    }

    onContinue  = topRightCallback;
    onQuit      = quitCallback;
    pageContext = nbgl_pageDrawInfo(&pageCallback, NULL, &info);
    nbgl_refresh();
}

/**
 * @brief Draws the settings pages of an app with as many pages as given
 *        For each page, the given navCallback will be called to get the content. Only 'type' and
 * union has to be set in this content
 *
 * @param title string to set in touchable title
 * @param initPage page on which to start [0->(nbPages-1)]
 * @param nbPages number of pages
 * @param touchable unused, it is always on
 * @param quitCallback callback called when quit button (or title) is pressed
 * @param navCallback callback called when navigation arrows are pressed
 * @param controlsCallback callback called when any controls in the settings (radios, switches) is
 * called (the tokens must be >= @ref FIRST_USER_TOKEN)
 */
void nbgl_useCaseSettings(const char                *title,
                          uint8_t                    initPage,
                          uint8_t                    nbPages,
                          bool                       touchable,
                          nbgl_callback_t            quitCallback,
                          nbgl_navCallback_t         navCallback,
                          nbgl_layoutTouchCallback_t controlsCallback)
{
    UNUSED(touchable);
    reset_callbacks();

    // memorize context
    onQuit     = quitCallback;
    onNav      = navCallback;
    onControls = controlsCallback;
    pageTitle  = title;
    navType    = SETTINGS_NAV;

    // fill navigation structure
    prepareNavInfo(false, nbPages, NULL);

    displaySettingsPage(initPage, true);
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
    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));

    // memorize context
    onQuit    = quitCallback;
    pageTitle = appName;
    navType   = GENERIC_NAV;

    if (settingContents != NULL) {
        memcpy(&genericContext.genericContents, settingContents, sizeof(nbgl_genericContents_t));
    }
    if (infosList != NULL) {
        genericContext.hasFinishingContent = true;
        memset(&FINISHING_CONTENT, 0, sizeof(nbgl_content_t));
        FINISHING_CONTENT.type = INFOS_LIST;
        memcpy(&FINISHING_CONTENT.content, infosList, sizeof(nbgl_content_u));
    }

    // fill navigation structure
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    if (infosList != NULL) {
        nbPages += nbgl_useCaseGetNbPagesForContent(&FINISHING_CONTENT, nbPages);
    }

    prepareNavInfo(false, nbPages, NULL);

    displayGenericContextPage(initPage, true);
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
 * @brief Draws the extended version of home page of an app (page on which we land when launching it
 *        from dashboard) with automatic support of setting display.
 * @note it enables to use an action button (black on Stax, white on Flex)
 *
 * @param appName app name
 * @param appIcon app icon
 * @param tagline text under app name (if NULL, it will be "This app enables signing transactions on
 * the <appName> network.")
 * @param initSettingPage if not INIT_HOME_PAGE, start directly the corresponding setting page
 * @param settingContents setting contents to be displayed
 * @param infosList infos to be displayed (version, license, developer, ...)
 * @param action if not NULL, info used for an action button (on top of "Quit
 * App" button/footer)
 * @param quitCallback callback called when quit button is touched
 */
void nbgl_useCaseHomeAndSettings(
    const char                *appName,
    const nbgl_icon_details_t *appIcon,
    const char                *tagline,
    const uint8_t
        initSettingPage,  // if not INIT_HOME_PAGE, start directly the corresponding setting page
    const nbgl_genericContents_t *settingContents,
    const nbgl_contentInfoList_t *infosList,
    const nbgl_homeAction_t      *action,  // Set to NULL if no additional action
    nbgl_callback_t               quitCallback)
{
    nbgl_homeAndSettingsContext_t *context = &bundleNavContext.homeAndSettings;

    context->appName         = appName;
    context->appIcon         = appIcon;
    context->tagline         = tagline;
    context->settingContents = settingContents;
    context->infosList       = infosList;
    if (action != NULL) {
        context->actionText     = action->text;
        context->actionCallback = action->callback;
    }
    else {
        context->actionText     = NULL;
        context->actionCallback = NULL;
    }
    context->quitCallback = quitCallback;

    if (initSettingPage != INIT_HOME_PAGE) {
        bundleNavStartSettingsAtPage(initSettingPage);
    }
    else {
        bundleNavStartHome();
    }
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
    reset_callbacks();

    nbgl_screenTickerConfiguration_t ticker = {
        .tickerCallback  = &tickerCallback,
        .tickerIntervale = 0,    // not periodic
        .tickerValue     = 3000  // 3 seconds
    };
    onQuit = quitCallback;
    if (isSuccess) {
#ifdef HAVE_PIEZO_SOUND
        io_seproxyhal_play_tune(TUNE_LEDGER_MOMENT);
#endif  // HAVE_PIEZO_SOUND

        pageContext = nbgl_pageDrawLedgerInfo(&pageCallback, &ticker, message, QUIT_TOKEN);
    }
    else {
        nbgl_pageInfoDescription_t info = {.bottomButtonStyle    = NO_BUTTON_STYLE,
                                           .footerText           = NULL,
                                           .centeredInfo.icon    = &C_Denied_Circle_64px,
                                           .centeredInfo.offsetY = SMALL_FOOTER_HEIGHT / 2,
                                           .centeredInfo.onTop   = false,
                                           .centeredInfo.style   = LARGE_CASE_INFO,
                                           .centeredInfo.text1   = message,
                                           .centeredInfo.text2   = NULL,
                                           .centeredInfo.text3   = NULL,
                                           .tapActionText        = "",
                                           .isSwipeable          = false,
                                           .tapActionToken       = QUIT_TOKEN,
                                           .topRightStyle        = NO_BUTTON_STYLE,
                                           .actionButtonText     = NULL,
                                           .tuneId               = TUNE_TAP_CASUAL};
        pageContext                     = nbgl_pageDrawInfo(&pageCallback, &ticker, &info);
    }
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
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
            msg       = "Address verification\ncancelled";
            isSuccess = false;
            break;
        default:
            return;
    }
    nbgl_useCaseStatus(msg, isSuccess, quitCallback);
}

/**
 * @brief Draws a generic choice page, described in a centered info (with configurable icon), thanks
 * to a button and a footer at the bottom of the page. The given callback is called with true as
 * argument if the button is touched, false if footer is touched
 *
 * @param icon icon to set in center of page
 * @param message string to set in center of page (32px)
 * @param subMessage string to set under message (24px) (can be NULL)
 * @param confirmText string to set in button, to confirm (cannot be NULL)
 * @param cancelText string to set in footer, to reject (cannot be NULL)
 * @param callback callback called when button or footer is touched
 */
void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *cancelText,
                        nbgl_choiceCallback_t      callback)
{
    reset_callbacks();

    nbgl_pageConfirmationDescription_t info = {.cancelText           = cancelText,
                                               .centeredInfo.text1   = message,
                                               .centeredInfo.text2   = subMessage,
                                               .centeredInfo.text3   = NULL,
                                               .centeredInfo.style   = LARGE_CASE_INFO,
                                               .centeredInfo.icon    = icon,
                                               .centeredInfo.offsetY = 0,
                                               .confirmationText     = confirmText,
                                               .confirmationToken    = CHOICE_TOKEN,
                                               .tuneId               = TUNE_TAP_CASUAL,
                                               .modal                = false};
    // check params
    if ((confirmText == NULL) || (cancelText == NULL)) {
        return;
    }
    onChoice    = callback;
    pageContext = nbgl_pageDrawConfirmation(&pageCallback, &info);
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
}

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
    // Don't reset callback or nav context as this is just a modal.

    nbgl_pageConfirmationDescription_t info = {.cancelText           = cancelText,
                                               .centeredInfo.text1   = message,
                                               .centeredInfo.text2   = subMessage,
                                               .centeredInfo.text3   = NULL,
                                               .centeredInfo.style   = LARGE_CASE_INFO,
                                               .centeredInfo.icon    = &C_Important_Circle_64px,
                                               .centeredInfo.offsetY = 0,
                                               .confirmationText     = confirmText,
                                               .confirmationToken    = CHOICE_TOKEN,
                                               .tuneId               = TUNE_TAP_CASUAL,
                                               .modal                = true};
    onModalConfirm                          = callback;
    modalPageContext                        = nbgl_pageDrawConfirmation(&pageModalCallback, &info);
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
}

/**
 * @brief Draws a review start page, with a centered message, a "tap to continue" container and a
 * "reject" footer
 *
 * @param icon icon to use in centered info
 * @param reviewTitle string to set in middle of page (in 32px font)
 * @param reviewSubTitle string to set under reviewTitle (in 24px font) (can be NULL)
 * @param rejectText string to set in footer, to reject review
 * @param continueCallback callback called when main panel is touched
 * @param rejectCallback callback called when footer is touched
 */
void nbgl_useCaseReviewStart(const nbgl_icon_details_t *icon,
                             const char                *reviewTitle,
                             const char                *reviewSubTitle,
                             const char                *rejectText,
                             nbgl_callback_t            continueCallback,
                             nbgl_callback_t            rejectCallback)
{
    reset_callbacks();

    nbgl_pageInfoDescription_t info = {.footerText       = rejectText,
                                       .footerToken      = QUIT_TOKEN,
                                       .tapActionText    = NULL,
                                       .isSwipeable      = true,
                                       .tapActionToken   = CONTINUE_TOKEN,
                                       .topRightStyle    = NO_BUTTON_STYLE,
                                       .actionButtonText = NULL,
                                       .tuneId           = TUNE_TAP_CASUAL};
    prepareReviewFirstPage(&info.centeredInfo, icon, reviewTitle, reviewSubTitle);
    onQuit     = rejectCallback;
    onContinue = continueCallback;

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    io_seproxyhal_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    pageContext = nbgl_pageDrawInfo(&pageCallback, NULL, &info);
    nbgl_refresh();
}
/**
 * @brief Draws a flow of pages of a review. A back key is available on top-left of the screen,
 * except in first page It is possible to go to next page thanks to "tap to continue". For each
 * page, the given navCallback will be called to get the content. Only 'type' and union has to be
 * set in this content
 *
 * @param initPage page on which to start [0->(nbPages-1)]
 * @param nbPages number of pages. If 0, no progress indicator is used
 * @param rejectText text to use in footer
 * @param navCallback callback called when navigation (back or "tap to continue") is touched
 * @param buttonCallback callback called when a potential button (details) in the content is touched
 * @param choiceCallback callback called when either long_press or footer is called (param is true
 * for long press)
 */
void nbgl_useCaseRegularReview(uint8_t                    initPage,
                               uint8_t                    nbPages,
                               const char                *rejectText,
                               nbgl_layoutTouchCallback_t buttonCallback,
                               nbgl_navCallback_t         navCallback,
                               nbgl_choiceCallback_t      choiceCallback)
{
    reset_callbacks();

    // memorize context
    onChoice       = choiceCallback;
    onNav          = navCallback;
    onControls     = buttonCallback;
    forwardNavOnly = false;
    navType        = REVIEW_NAV;

    // fill navigation structure
    UNUSED(rejectText);
    prepareNavInfo(true, nbPages, getRejectReviewText(TYPE_OPERATION));

    displayReviewPage(initPage, true);
}

/**
 * @brief Draws a flow of pages of a review, without back key.
 *        It is possible to go to next page thanks to "tap to continue".
 *        For each page, the given navCallback will be called to get the content. Only 'type' and
 *        union has to be set in this content.
 *        Note that this is not a standard use case, it should only be used on very specific
 *        situations.
 *
 * @param rejectText text to use in footer
 * @param buttonCallback callback called when a potential button (details or long press) in the
 * content is touched
 * @param navCallback callback called when navigation "tap to continue" is touched, to get the
 * content of next page
 * @param choiceCallback callback called when either long_press or footer is called (param is true
 * for long press)
 */
void nbgl_useCaseForwardOnlyReview(const char                *rejectText,
                                   nbgl_layoutTouchCallback_t buttonCallback,
                                   nbgl_navCallback_t         navCallback,
                                   nbgl_choiceCallback_t      choiceCallback)
{
    reset_callbacks();

    // memorize context
    onChoice       = choiceCallback;
    onNav          = navCallback;
    onControls     = buttonCallback;
    forwardNavOnly = true;
    navType        = REVIEW_NAV;

    // fill navigation structure
    UNUSED(rejectText);
    prepareNavInfo(true, NBGL_NO_PROGRESS_INDICATOR, getRejectReviewText(TYPE_OPERATION));

    navInfo.progressIndicator = false;
    navInfo.skipText          = "Skip";
    navInfo.skipToken         = SKIP_TOKEN;

    displayReviewPage(0, true);
}

/**
 * @brief Draws a flow of pages of a review, without back key.
 *        It is possible to go to next page thanks to "tap to continue".
 *        For each page, the given navCallback will be called to get the content. Only 'type' and
 *        union has to be set in this content.
 *        Note that this is not a standard use case, it should only be used on very specific
 *        situations.
 *
 * @param rejectText text to use in footer
 * @param buttonCallback callback called when a potential button (details or long press) in the
 * content is touched
 * @param navCallback callback called when navigation "tap to continue" is touched, to get the
 * content of next page
 * @param choiceCallback callback called when either long_press or footer is called (param is true
 * for long press)
 */
void nbgl_useCaseForwardOnlyReviewNoSkip(const char                *rejectText,
                                         nbgl_layoutTouchCallback_t buttonCallback,
                                         nbgl_navCallback_t         navCallback,
                                         nbgl_choiceCallback_t      choiceCallback)
{
    reset_callbacks();

    // memorize context
    onChoice       = choiceCallback;
    onNav          = navCallback;
    onControls     = buttonCallback;
    forwardNavOnly = true;
    navType        = REVIEW_NAV;

    // fill navigation structure
    UNUSED(rejectText);
    prepareNavInfo(true, NBGL_NO_PROGRESS_INDICATOR, getRejectReviewText(TYPE_OPERATION));
    navInfo.progressIndicator = false;
    displayReviewPage(0, false);
}

/**
 * @brief Draws a flow of pages of a review. A back key is available on top-left of the screen,
 * except in first page It is possible to go to next page thanks to "tap to continue".
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param tagValueList list of tag/value pairs
 * @param infoLongPress information to build the last page
 * @param rejectText text to use in footer
 * @param callback callback called when transaction is accepted (param is true) or rejected (param
 * is false)
 */
void nbgl_useCaseStaticReview(const nbgl_contentTagValueList_t *tagValueList,
                              const nbgl_pageInfoLongPress_t   *infoLongPress,
                              const char                       *rejectText,
                              nbgl_choiceCallback_t             callback)
{
    uint8_t offset = 0;

    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));

    // memorize context
    onChoice  = callback;
    navType   = GENERIC_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    memset(localContentsList, 0, 2 * sizeof(nbgl_content_t));

    if (tagValueList != NULL && tagValueList->nbPairs != 0) {
        localContentsList[offset].type = TAG_VALUE_LIST;
        memcpy(&localContentsList[offset].content.tagValueList,
               tagValueList,
               sizeof(nbgl_contentTagValueList_t));
        offset++;
    }

    localContentsList[offset].type = INFO_LONG_PRESS;
    memcpy(&localContentsList[offset].content.infoLongPress,
           infoLongPress,
           sizeof(nbgl_pageInfoLongPress_t));
    localContentsList[offset].content.infoLongPress.longPressToken = CONFIRM_TOKEN;
    offset++;

    genericContext.genericContents.nbContents = offset;

    // compute number of pages & fill navigation structure
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    UNUSED(rejectText);
    prepareNavInfo(true, nbPages, getRejectReviewText(TYPE_OPERATION));

    displayGenericContextPage(0, true);
}

/**
 * @brief Similar to @ref nbgl_useCaseStaticReview() but with a simple button/footer pair instead of
 * a long press button/footer pair.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param tagValueList list of tag/value pairs
 * @param infoLongPress information to build the last page (even if not a real long press, the info
 * is the same)
 * @param rejectText text to use in footer
 * @param callback callback called when transaction is accepted (param is true) or rejected (param
 * is false)
 */
void nbgl_useCaseStaticReviewLight(const nbgl_contentTagValueList_t *tagValueList,
                                   const nbgl_pageInfoLongPress_t   *infoLongPress,
                                   const char                       *rejectText,
                                   nbgl_choiceCallback_t             callback)
{
    uint8_t offset = 0;

    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));

    // memorize context
    onChoice  = callback;
    navType   = GENERIC_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    memset(localContentsList, 0, 2 * sizeof(nbgl_content_t));

    if (tagValueList != NULL && tagValueList->nbPairs != 0) {
        localContentsList[offset].type = TAG_VALUE_LIST;
        memcpy(&localContentsList[offset].content.tagValueList,
               tagValueList,
               sizeof(nbgl_contentTagValueList_t));
        offset++;
    }

    localContentsList[offset].type                           = INFO_BUTTON;
    localContentsList[offset].content.infoButton.text        = infoLongPress->text;
    localContentsList[offset].content.infoButton.icon        = infoLongPress->icon;
    localContentsList[offset].content.infoButton.buttonText  = infoLongPress->longPressText;
    localContentsList[offset].content.infoButton.buttonToken = CONFIRM_TOKEN;
    localContentsList[offset].content.infoButton.tuneId      = TUNE_TAP_CASUAL;
    offset++;

    genericContext.genericContents.nbContents = offset;

    // compute number of pages & fill navigation structure
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    UNUSED(rejectText);
    prepareNavInfo(true, nbPages, getRejectReviewText(TYPE_OPERATION));

    displayGenericContextPage(0, true);
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
    useCaseReview(operationType,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  choiceCallback,
                  false);
}

/**
 * @brief Draws a flow of pages of a light review. Navigation operates with either swipe or
 * navigation keys at bottom right. The last page contains a button/footer with the given
 * finishTitle and the given icon.
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a short press one
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
    useCaseReview(operationType,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  choiceCallback,
                  true);
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
    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));

    // memorize context
    onQuit                                = rejectCallback;
    navType                               = GENERIC_NAV;
    pageTitle                             = NULL;
    bundleNavContext.review.operationType = TYPE_OPERATION;

    memcpy(&genericContext.genericContents, contents, sizeof(nbgl_genericContents_t));

    // compute number of pages & fill navigation structure
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    prepareNavInfo(true, nbPages, rejectText);
    navInfo.quitToken = QUIT_TOKEN;

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    io_seproxyhal_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    displayGenericContextPage(0, true);
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
    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));

    bundleNavContext.reviewStreaming.operationType  = operationType;
    bundleNavContext.reviewStreaming.choiceCallback = choiceCallback;
    bundleNavContext.reviewStreaming.icon           = icon;

    // memorize context
    onChoice  = bundleNavReviewStreamingChoice;
    navType   = STREAMING_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    genericContext.genericContents.nbContents   = 1;
    memset(localContentsList, 0, 1 * sizeof(nbgl_content_t));

    // First a centered info
    STARTING_CONTENT.type = CENTERED_INFO;
    prepareReviewFirstPage(
        &STARTING_CONTENT.content.centeredInfo, icon, reviewTitle, reviewSubTitle);

    // compute number of pages & fill navigation structure
    bundleNavContext.reviewStreaming.stepPageNb
        = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    prepareNavInfo(true, NBGL_NO_PROGRESS_INDICATOR, getRejectReviewText(operationType));
    // no back button on first page
    navInfo.navWithButtons.backButton = false;

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    io_seproxyhal_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    displayGenericContextPage(0, true);
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
    // Should follow a call to nbgl_useCaseReviewStreamingStart
    memset(&genericContext, 0, sizeof(genericContext));

    bundleNavContext.reviewStreaming.choiceCallback = choiceCallback;
    bundleNavContext.reviewStreaming.skipCallback   = skipCallback;

    // memorize context
    onChoice  = bundleNavReviewStreamingChoice;
    navType   = STREAMING_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    genericContext.genericContents.nbContents   = 1;
    memset(localContentsList, 0, 1 * sizeof(nbgl_content_t));

    // Then the tag/value pairs
    STARTING_CONTENT.type = TAG_VALUE_LIST;
    memcpy(
        &STARTING_CONTENT.content.tagValueList, tagValueList, sizeof(nbgl_contentTagValueList_t));

    // compute number of pages & fill navigation structure
    bundleNavContext.reviewStreaming.stepPageNb
        = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    prepareNavInfo(true,
                   NBGL_NO_PROGRESS_INDICATOR,
                   getRejectReviewText(bundleNavContext.reviewStreaming.operationType));
    // if the operation is skippable
    if (bundleNavContext.reviewStreaming.operationType & SKIPPABLE_OPERATION) {
        navInfo.progressIndicator = false;
        navInfo.skipText          = "Skip";
        navInfo.skipToken         = SKIP_TOKEN;
    }

    displayGenericContextPage(0, true);
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

/**
 * @brief finish drawing the flow of pages of a review.
 * @note  This should be called after a call to nbgl_useCaseReviewStreamingContinue.
 *
 * @param finishTitle string used in the last review page
 * @param choiceCallback callback called when more operation is approved (param is true) or is
 * rejected (param is false)
 */
void nbgl_useCaseReviewStreamingFinish(const char           *finishTitle,
                                       nbgl_choiceCallback_t choiceCallback)
{
    // Should follow a call to nbgl_useCaseReviewStreamingContinue
    memset(&genericContext, 0, sizeof(genericContext));

    bundleNavContext.reviewStreaming.choiceCallback = choiceCallback;

    // memorize context
    onChoice  = bundleNavReviewStreamingChoice;
    navType   = STREAMING_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    genericContext.genericContents.nbContents   = 1;
    memset(localContentsList, 0, 1 * sizeof(nbgl_content_t));

    // Eventually the long press page
    STARTING_CONTENT.type = INFO_LONG_PRESS;
    prepareReviewLastPage(&STARTING_CONTENT.content.infoLongPress,
                          bundleNavContext.reviewStreaming.icon,
                          finishTitle);

    // compute number of pages & fill navigation structure
    bundleNavContext.reviewStreaming.stepPageNb
        = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);
    prepareNavInfo(true, 1, getRejectReviewText(bundleNavContext.reviewStreaming.operationType));

    displayGenericContextPage(0, true);
}

/**
 * @brief Draws a flow of pages to view details on a given tag/value pair that doesn't fit in a
 * single page
 *
 * @param tag tag name (in gray)
 * @param value full value string, that will be split in multiple pages
 * @param wrapping if set to true, value text is wrapped on ' ' characters
 */
void nbgl_useCaseViewDetails(const char *tag, const char *value, bool wrapping)
{
    memset(&detailsContext, 0, sizeof(detailsContext));

    uint16_t nbLines
        = nbgl_getTextNbLinesInWidth(SMALL_REGULAR_FONT, value, AVAILABLE_WIDTH, wrapping);

    // initialize context
    detailsContext.tag         = tag;
    detailsContext.value       = value;
    detailsContext.nbPages     = (nbLines + NB_MAX_LINES_IN_DETAILS - 1) / NB_MAX_LINES_IN_DETAILS;
    detailsContext.currentPage = 0;
    detailsContext.wrapping    = wrapping;
    // add some spare for room lost with "..." substitution
    if (detailsContext.nbPages > 1) {
        uint16_t nbLostChars = (detailsContext.nbPages - 1) * 3;
        uint16_t nbLostLines = (nbLostChars + ((AVAILABLE_WIDTH) / 16) - 1)
                               / ((AVAILABLE_WIDTH) / 16);  // 16 for average char width
        uint8_t nbLinesInLastPage
            = nbLines - ((detailsContext.nbPages - 1) * NB_MAX_LINES_IN_DETAILS);

        detailsContext.nbPages += nbLostLines / NB_MAX_LINES_IN_DETAILS;
        if ((nbLinesInLastPage + (nbLostLines % NB_MAX_LINES_IN_DETAILS))
            > NB_MAX_LINES_IN_DETAILS) {
            detailsContext.nbPages++;
        }
    }

    displayDetailsPage(0, true);
}

/**
 * @brief draws an address confirmation page. This page contains the given address in a tag/value
 * layout, with a button to open a modal to see address as a QR Code, and at the bottom a button to
 * confirm and a footer to cancel
 *
 * @param address address to confirm (NULL terminated string)
 * @param callback callback called when button or footer is touched (if true, button, if false
 * footer)
 */
void nbgl_useCaseAddressConfirmation(const char *address, nbgl_choiceCallback_t callback)
{
    nbgl_useCaseAddressConfirmationExt(address, callback, NULL);
}

/**
 * @brief draws an extended address verification page. This page contains the given address in a
 * tag/value layout, with a button to open a modal to see address as a QR Code. A "tap to continue"
 * enables to open a second review page to display the other given tag/value pairs, with a button to
 * confirm and a footer to cancel
 *
 * @param address address to confirm (NULL terminated string)
 * @param callback callback called when button or footer is touched (if true, button, if false
 * footer)
 * @param tagValueList list of tag/value pairs (must fit in a single page, and be persistent because
 * no copy)
 */
void nbgl_useCaseAddressConfirmationExt(const char                       *address,
                                        nbgl_choiceCallback_t             callback,
                                        const nbgl_contentTagValueList_t *tagValueList)
{
    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));
    memset(&addressConfirmationContext, 0, sizeof(addressConfirmationContext));

    // save context
    onChoice  = callback;
    navType   = GENERIC_NAV;
    pageTitle = NULL;

    genericContext.genericContents.contentsList = localContentsList;
    genericContext.genericContents.nbContents   = (tagValueList == NULL) ? 1 : 2;
    memset(localContentsList, 0, 2 * sizeof(nbgl_content_t));
    prepareAddressConfirmationPages(
        address, tagValueList, &STARTING_CONTENT, &localContentsList[1]);

    // fill navigation structure, common to all pages
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);

    prepareNavInfo(true, nbPages, "Cancel");

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    io_seproxyhal_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    displayGenericContextPage(0, true);
}

/**
 * @brief Draws a flow of pages of an extended address verification page.
 * A back key is available on top-left of the screen,
 * except in first page It is possible to go to next page thanks to "tap to continue".
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param address address to confirm (NULL terminated string)
 * @param additionalTagValueList list of tag/value pairs (can be NULL) (must be persistent because
 * no copy)
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
    reset_callbacks();
    memset(&genericContext, 0, sizeof(genericContext));
    memset(&addressConfirmationContext, 0, sizeof(addressConfirmationContext));

    // save context
    onChoice                              = choiceCallback;
    navType                               = GENERIC_NAV;
    pageTitle                             = NULL;
    bundleNavContext.review.operationType = TYPE_OPERATION;

    genericContext.genericContents.contentsList = localContentsList;
    genericContext.genericContents.nbContents   = (additionalTagValueList == NULL) ? 2 : 3;
    memset(localContentsList, 0, 3 * sizeof(nbgl_content_t));

    // First a centered info
    STARTING_CONTENT.type = CENTERED_INFO;
    prepareReviewFirstPage(
        &STARTING_CONTENT.content.centeredInfo, icon, reviewTitle, reviewSubTitle);
    STARTING_CONTENT.content.centeredInfo.text3 = "Swipe to continue";

    // Then the address confirmation pages
    prepareAddressConfirmationPages(
        address, additionalTagValueList, &localContentsList[1], &localContentsList[2]);

    // fill navigation structure, common to all pages
    uint8_t nbPages = nbgl_useCaseGetNbPagesForGenericContents(&genericContext.genericContents, 0);

    prepareNavInfo(true, nbPages, "Cancel");

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    io_seproxyhal_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    displayGenericContextPage(0, true);
}

/**
 * @brief draw a spinner page with the given parameters. The spinner will "turn" automatically every
 * 800 ms
 *
 * @param text text to use under spinner
 */
void nbgl_useCaseSpinner(const char *text)
{
    pageContext = nbgl_pageDrawSpinner(NULL, (const char *) text);
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
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
 * @param backToken token used with actionCallback (0 if unused))
 * @param shuffled if set to true, digits are shuffled in keypad
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when back button is pressed
 * @param validatePinCallback function calledto validate the pin code
 * @param onActionCallback callback called on any action on the layout
 */
void nbgl_useCaseKeypadDigits(const char                *title,
                              uint8_t                    minDigits,
                              uint8_t                    maxDigits,
                              uint8_t                    backToken,
                              bool                       shuffled,
                              tune_index_e               tuneId,
                              nbgl_pinValidCallback_t    validatePinCallback,
                              nbgl_layoutTouchCallback_t actionCallback)
{
    keypadGenericUseCase(title,
                         minDigits,
                         maxDigits,
                         backToken,
                         shuffled,
                         false,
                         tuneId,
                         validatePinCallback,
                         actionCallback);
}
/**
 * @brief draws a standard keypad modal page with hidden digits. It contains
 *        - a navigation bar at the top
 *        - a title for the pin code
 *        - a hidden digit entry
 *        - the keypad at the bottom
 *
 * @note callbacks allow to control the behavior.
 *       backspace and validation button are shown/hidden automatically
 *
 * @param title string to set in pin code title
 * @param minDigits pin minimum number of digits
 * @param maxDigits maximum number of digits to be displayed
 * @param backToken token used with actionCallback (0 if unused))
 * @param shuffled if set to true, digits are shuffled in keypad
 * @param tuneId if not @ref NBGL_NO_TUNE, a tune will be played when back button is pressed
 * @param validatePinCallback function calledto validate the pin code
 * @param onActionCallback callback called on any action on the layout
 */
void nbgl_useCaseKeypadPIN(const char                *title,
                           uint8_t                    minDigits,
                           uint8_t                    maxDigits,
                           uint8_t                    backToken,
                           bool                       shuffled,
                           tune_index_e               tuneId,
                           nbgl_pinValidCallback_t    validatePinCallback,
                           nbgl_layoutTouchCallback_t actionCallback)
{
    keypadGenericUseCase(title,
                         minDigits,
                         maxDigits,
                         backToken,
                         shuffled,
                         true,
                         tuneId,
                         validatePinCallback,
                         actionCallback);
}
#endif  // NBGL_KEYPAD

#endif  // HAVE_SE_TOUCH
#endif  // NBGL_USE_CASE
