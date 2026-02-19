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
#include "os_io_seph_ux.h"
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

#define MAX_PAGE_NB       256
#define MAX_MODAL_PAGE_NB 32

/* Alias to clarify usage of genericContext hasStartingContent and hasFinishingContent feature */
#define STARTING_CONTENT  localContentsList[0]
#define FINISHING_CONTENT localContentsList[1]

/* max number of lines for address under QR Code */
#define QRCODE_NB_MAX_LINES     3
/* max number of char for reduced QR Code address */
#define QRCODE_REDUCED_ADDR_LEN 128

/* maximum number of pairs in a page of address confirm */
#define ADDR_VERIF_NB_PAIRS 3

// macros to ease access to shared contexts
#define sharedContext        genericContext.sharedCtx
#define keypadContext        sharedContext.keypad
#define reviewWithWarnCtx    sharedContext.reviewWithWarning
#define choiceWithDetailsCtx sharedContext.choiceWithDetails

/* max length of the string displaying the description of the Web3 Checks report */
#define W3C_DESCRIPTION_MAX_LEN 128

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
enum {
    BACK_TOKEN = 0,
    NEXT_TOKEN,
    QUIT_TOKEN,
    NAV_TOKEN,
    MODAL_NAV_TOKEN,
    SKIP_TOKEN,
    CONTINUE_TOKEN,
    ADDRESS_QRCODE_BUTTON_TOKEN,
    ACTION_BUTTON_TOKEN,
    CHOICE_TOKEN,
    DETAILS_BUTTON_TOKEN,
    CONFIRM_TOKEN,
    REJECT_TOKEN,
    VALUE_ALIAS_TOKEN,
    INFO_ALIAS_TOKEN,
    INFOS_TIP_BOX_TOKEN,
    BLIND_WARNING_TOKEN,
    WARNING_BUTTON_TOKEN,
    CHOICE_DETAILS_TOKEN,
    TIP_BOX_TOKEN,
    QUIT_TIPBOX_MODAL_TOKEN,
    WARNING_CHOICE_TOKEN,
    DISMISS_QR_TOKEN,
    DISMISS_WARNING_TOKEN,
    DISMISS_DETAILS_TOKEN,
    PRELUDE_CHOICE_TOKEN,
    FIRST_WARN_BAR_TOKEN,
    LAST_WARN_BAR_TOKEN = (FIRST_WARN_BAR_TOKEN + NB_WARNING_TYPES - 1),
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
    uint8_t     currentPairIdx;
    bool        wrapping;
    const char *tag;
    const char *value;
    const char *nextPageStart;
} DetailsContext_t;

typedef struct AddressConfirmationContext_s {
    nbgl_layoutTagValue_t tagValuePairs[ADDR_VERIF_NB_PAIRS];
    nbgl_layout_t        *modalLayout;
    uint8_t               nbPairs;
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

typedef struct ReviewWithWarningContext_s {
    bool                              isStreaming;
    nbgl_operationType_t              operationType;
    const nbgl_contentTagValueList_t *tagValueList;
    const nbgl_icon_details_t        *icon;
    const char                       *reviewTitle;
    const char                       *reviewSubTitle;
    const char                       *finishTitle;
    const nbgl_warning_t             *warning;
    nbgl_choiceCallback_t             choiceCallback;
    nbgl_layout_t                    *layoutCtx;
    nbgl_layout_t                    *modalLayout;
    uint8_t                           securityReportLevel;  // level 1 is the first level of menus
    bool                              isIntro;  // set to true during intro (before actual review)
} ReviewWithWarningContext_t;

typedef struct ChoiceWithDetailsContext_s {
    const nbgl_genericDetails_t *details;
    nbgl_layout_t               *layoutCtx;
    nbgl_layout_t               *modalLayout;
    uint8_t                      level;  // level 1 is the first level of menus
} ChoiceWithDetailsContext_t;

typedef enum {
    SHARE_CTX_KEYPAD,
    SHARE_CTX_REVIEW_WITH_WARNING,
    SHARE_CTX_CHOICE_WITH_DETAILS,
} SharedContextUsage_t;

// this union is intended to save RAM for context storage
// indeed, these 3 contexts cannot happen simultaneously
typedef struct {
    union {
#ifdef NBGL_KEYPAD
        KeypadContext_t keypad;
#endif
        ReviewWithWarningContext_t reviewWithWarning;
        ChoiceWithDetailsContext_t choiceWithDetails;
    };
    SharedContextUsage_t usage;
} SharedContext_t;

typedef enum {
    USE_CASE_GENERIC = 0,
    USE_CASE_SPINNER
} GenericContextType_t;

typedef struct {
    GenericContextType_t   type;  // type of Generic context usage
    uint8_t                spinnerPosition;
    nbgl_genericContents_t genericContents;
    int8_t                 currentContentIdx;
    uint8_t                currentContentElementNb;
    uint8_t                currentElementIdx;
    bool                   hasStartingContent;
    bool                   hasFinishingContent;
    const char            *detailsItem;
    const char            *detailsvalue;
    bool                   detailsWrapping;
    bool                   validWarningCtx;  // set to true if the WarningContext is valid
    const nbgl_contentTagValue_t
        *currentPairs;  // to be used to retrieve the pairs with value alias
    nbgl_contentTagValueCallback_t
                  currentCallback;  // to be used to retrieve the pairs with value alias
    nbgl_layout_t modalLayout;
    nbgl_layout_t backgroundLayout;
    const nbgl_contentInfoList_t     *currentInfos;
    const nbgl_contentTagValueList_t *currentTagValues;
    nbgl_tipBox_t                     tipBox;
    SharedContext_t sharedCtx;  // multi-purpose context shared for non-concurrent usages
} GenericContext_t;

typedef struct {
    const char                   *appName;
    const nbgl_icon_details_t    *appIcon;
    const char                   *tagline;
    const nbgl_genericContents_t *settingContents;
    const nbgl_contentInfoList_t *infosList;
    nbgl_homeAction_t             homeAction;
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

typedef struct {
    const nbgl_icon_details_t *icon;
    const char                *text;
    const char                *subText;
} SecurityReportItem_t;

/**********************
 *  STATIC VARIABLES
 **********************/

// char buffers to build some strings
static char tmpString[W3C_DESCRIPTION_MAX_LEN];

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

// context for tip-box
#define activeTipBox genericContext.tipBox

// context for navigation use case
static nbgl_pageNavigationInfo_t navInfo;
static bool                      forwardNavOnly;
static NavType_t                 navType;

static DetailsContext_t detailsContext;

// context for address review
static AddressConfirmationContext_t addressConfirmationContext;

// contexts for generic navigation
static GenericContext_t genericContext;
static nbgl_content_t
    localContentsList[3];  // 3 needed for nbgl_useCaseReview (starting page / tags / final page)
static uint8_t genericContextPagesInfo[MAX_PAGE_NB / PAGES_PER_UINT8];
static uint8_t modalContextPagesInfo[MAX_MODAL_PAGE_NB / PAGES_PER_UINT8];

// contexts for bundle navigation
static nbgl_BundleNavContext_t bundleNavContext;

// indexed by nbgl_contentType_t
static const uint8_t nbMaxElementsPerContentType[] = {
    1,  // CENTERED_INFO
    1,  // EXTENDED_CENTER
    1,  // INFO_LONG_PRESS
    1,  // INFO_BUTTON
    1,  // TAG_VALUE_LIST (computed dynamically)
    1,  // TAG_VALUE_DETAILS
    1,  // TAG_VALUE_CONFIRM
    3,  // SWITCHES_LIST (computed dynamically)
    3,  // INFOS_LIST (computed dynamically)
    5,  // CHOICES_LIST (computed dynamically)
    5,  // BARS_LIST (computed dynamically)
};

// clang-format off
static const SecurityReportItem_t securityReportItems[NB_WARNING_TYPES] = {
    [BLIND_SIGNING_WARN] = {
        .icon    = &WARNING_ICON,
        .text    = "Blind signing required",
        .subText = "This transaction's details are not fully verifiable. If "
                   "you sign, you could lose all your assets."
    },
    [W3C_ISSUE_WARN] = {
        .icon    = &WARNING_ICON,
        .text    = "Transaction Check unavailable",
        .subText = NULL
    },
    [W3C_RISK_DETECTED_WARN] = {
        .icon    = &WARNING_ICON,
        .text    = "Risk detected",
        .subText = "This transaction was scanned as risky by Web3 Checks."
    },
    [W3C_THREAT_DETECTED_WARN] = {
        .icon    = &WARNING_ICON,
        .text    = "Critical threat",
        .subText = "This transaction was scanned as malicious by Web3 Checks."
    },
    [W3C_NO_THREAT_WARN] = {
        .icon    = NULL,
        .text    = "No threat detected",
        .subText = "Transaction Check didn't find any threat, but always "
                    "review transaction details carefully."
    }
};
// clang-format on

// configuration of warning when using @ref nbgl_useCaseReviewBlindSigning()
static const nbgl_warning_t blindSigningWarning = {.predefinedSet = (1 << BLIND_SIGNING_WARN)};

#ifdef NBGL_QRCODE
/* buffer to store reduced address under QR Code */
static char reducedAddress[QRCODE_REDUCED_ADDR_LEN];
#endif  // NBGL_QRCODE

/**********************
 *  STATIC FUNCTIONS
 **********************/
static void displayReviewPage(uint8_t page, bool forceFullRefresh);
static void displayDetailsPage(uint8_t page, bool forceFullRefresh);
static void displayTagValueListModalPage(uint8_t pageIdx, bool forceFullRefresh);
static void displayFullValuePage(const char                   *backText,
                                 const char                   *aliasText,
                                 const nbgl_contentValueExt_t *extension);
static void displayInfosListModal(const char                   *modalTitle,
                                  const nbgl_contentInfoList_t *infos,
                                  bool                          fromReview);
static void displayTagValueListModal(const nbgl_contentTagValueList_t *tagValues);
static void displaySettingsPage(uint8_t page, bool forceFullRefresh);
static void displayGenericContextPage(uint8_t pageIdx, bool forceFullRefresh);
static void pageCallback(int token, uint8_t index);
#ifdef NBGL_QRCODE
static void displayAddressQRCode(void);
#endif  // NBGL_QRCODE
static void modalLayoutTouchCallback(int token, uint8_t index);
static void displaySkipWarning(void);

static void bundleNavStartHome(void);
static void bundleNavStartSettingsAtPage(uint8_t initSettingPage);
static void bundleNavStartSettings(void);

static void           bundleNavReviewStreamingChoice(bool confirm);
static nbgl_layout_t *displayModalDetails(const nbgl_warningDetails_t *details, uint8_t token);
static void           displaySecurityReport(uint32_t set);
static void           displayCustomizedSecurityReport(const nbgl_warningDetails_t *details);
static void           displayInitialWarning(void);
static void           useCaseReview(nbgl_operationType_t              operationType,
                                    const nbgl_contentTagValueList_t *tagValueList,
                                    const nbgl_icon_details_t        *icon,
                                    const char                       *reviewTitle,
                                    const char                       *reviewSubTitle,
                                    const char                       *finishTitle,
                                    const nbgl_tipBox_t              *tipBox,
                                    nbgl_choiceCallback_t             choiceCallback,
                                    bool                              isLight,
                                    bool                              playNotifSound);
static void           useCaseReviewStreamingStart(nbgl_operationType_t       operationType,
                                                  const nbgl_icon_details_t *icon,
                                                  const char                *reviewTitle,
                                                  const char                *reviewSubTitle,
                                                  nbgl_choiceCallback_t      choiceCallback,
                                                  bool                       playNotifSound);
static void           useCaseHomeExt(const char                *appName,
                                     const nbgl_icon_details_t *appIcon,
                                     const char                *tagline,
                                     bool                       withSettings,
                                     nbgl_homeAction_t         *homeAction,
                                     nbgl_callback_t            topRightCallback,
                                     nbgl_callback_t            quitCallback);
static void           displayDetails(const char *tag, const char *value, bool wrapping);

static void reset_callbacks_and_context(void)
{
    onQuit          = NULL;
    onContinue      = NULL;
    onAction        = NULL;
    onNav           = NULL;
    onControls      = NULL;
    onContentAction = NULL;
    onChoice        = NULL;
#ifdef NBGL_KEYPAD
    onValidatePin = NULL;
#endif
    memset(&genericContext, 0, sizeof(genericContext));
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

// Helper to set modalContext page info
static void modalContextSetPageInfo(uint8_t pageIdx, uint8_t nbElements)
{
    uint8_t pageData = SET_PAGE_NB_ELEMENTS(nbElements);

    modalContextPagesInfo[pageIdx / PAGES_PER_UINT8]
        &= ~(0x0F << ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS));
    modalContextPagesInfo[pageIdx / PAGES_PER_UINT8]
        |= pageData << ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS);
}

// Helper to get modalContext page info
static void modalContextGetPageInfo(uint8_t pageIdx, uint8_t *nbElements)
{
    uint8_t pageData = modalContextPagesInfo[pageIdx / PAGES_PER_UINT8]
                       >> ((pageIdx % PAGES_PER_UINT8) * PAGE_DATA_BITS);
    if (nbElements != NULL) {
        *nbElements = GET_PAGE_NB_ELEMENTS(pageData);
    }
}

// Simple helper to get the number of elements inside a nbgl_content_t
static uint8_t getContentNbElement(const nbgl_content_t *content)
{
    switch (content->type) {
        case TAG_VALUE_LIST:
            return content->content.tagValueList.nbPairs;
        case TAG_VALUE_CONFIRM:
            return content->content.tagValueConfirm.tagValueList.nbPairs;
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

static void prepareReviewFirstPage(nbgl_contentCenter_t      *contentCenter,
                                   const nbgl_icon_details_t *icon,
                                   const char                *reviewTitle,
                                   const char                *reviewSubTitle)
{
    contentCenter->icon        = icon;
    contentCenter->title       = reviewTitle;
    contentCenter->description = reviewSubTitle;
    contentCenter->subText     = "Swipe to review";
    contentCenter->smallTitle  = NULL;
    contentCenter->iconHug     = 0;
    contentCenter->padding     = false;
    contentCenter->illustrType = ICON_ILLUSTRATION;
}

static const char *getFinishTitle(nbgl_operationType_t operationType, const char *finishTitle)
{
    if (finishTitle != NULL) {
        return finishTitle;
    }
    switch (operationType & REAL_TYPE_MASK) {
        case TYPE_TRANSACTION:
            return "Sign transaction";
        case TYPE_MESSAGE:
            return "Sign message";
        default:
            return "Sign operation";
    }
}

static void prepareReviewLastPage(nbgl_operationType_t         operationType,
                                  nbgl_contentInfoLongPress_t *infoLongPress,
                                  const nbgl_icon_details_t   *icon,
                                  const char                  *finishTitle)
{
    infoLongPress->text           = getFinishTitle(operationType, finishTitle);
    infoLongPress->icon           = icon;
    infoLongPress->longPressText  = "Hold to sign";
    infoLongPress->longPressToken = CONFIRM_TOKEN;
}

static void prepareReviewLightLastPage(nbgl_operationType_t       operationType,
                                       nbgl_contentInfoButton_t  *infoButton,
                                       const nbgl_icon_details_t *icon,
                                       const char                *finishTitle)
{
    infoButton->text        = getFinishTitle(operationType, finishTitle);
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

    if (token == INFOS_TIP_BOX_TOKEN) {
        // the icon next to info type alias has been touched
        displayFullValuePage(activeTipBox.infos.infoTypes[index],
                             activeTipBox.infos.infoContents[index],
                             &activeTipBox.infos.infoExtensions[index]);
        return;
    }
    else if (token == INFO_ALIAS_TOKEN) {
        // the icon next to info type alias has been touched, but coming from review
        displayFullValuePage(genericContext.currentInfos->infoTypes[index],
                             genericContext.currentInfos->infoContents[index],
                             &genericContext.currentInfos->infoExtensions[index]);
        return;
    }
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
    if (token == MODAL_NAV_TOKEN) {
        if (index == EXIT_PAGE) {
            // redraw the background layer
            nbgl_screenRedraw();
            nbgl_refresh();
        }
        else {
            displayTagValueListModalPage(index, false);
        }
    }
    else if (token == QUIT_TOKEN) {
        // redraw the background layer
        nbgl_screenRedraw();
        nbgl_refresh();
    }
    else if (token == QUIT_TIPBOX_MODAL_TOKEN) {
        // if in "warning context", go back to security report
        if (reviewWithWarnCtx.securityReportLevel == 2) {
            reviewWithWarnCtx.securityReportLevel = 1;
            displaySecurityReport(reviewWithWarnCtx.warning->predefinedSet);
        }
        else {
            // redraw the background layer
            nbgl_screenRedraw();
            nbgl_refresh();
        }
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
                onModalConfirm = NULL;
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
        displayDetails(genericContext.detailsItem,
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
        displayFullValuePage(pair->item, pair->value, pair->extension);
    }
    else if (token == BLIND_WARNING_TOKEN) {
        reviewWithWarnCtx.isIntro = false;
        reviewWithWarnCtx.warning = NULL;
        displaySecurityReport(1 << BLIND_SIGNING_WARN);
    }
    else if (token == WARNING_BUTTON_TOKEN) {
        // top-right button, display the security modal
        reviewWithWarnCtx.securityReportLevel = 1;
        // if preset is used
        if (reviewWithWarnCtx.warning->predefinedSet) {
            displaySecurityReport(reviewWithWarnCtx.warning->predefinedSet);
        }
        else {
            // use customized warning
            if (reviewWithWarnCtx.isIntro) {
                displayCustomizedSecurityReport(reviewWithWarnCtx.warning->introDetails);
            }
            else {
                displayCustomizedSecurityReport(reviewWithWarnCtx.warning->reviewDetails);
            }
        }
    }
    else if (token == TIP_BOX_TOKEN) {
        // if warning context is valid and if W3C directly display same as top-right
        if (genericContext.validWarningCtx && (reviewWithWarnCtx.warning->predefinedSet != 0)) {
            reviewWithWarnCtx.securityReportLevel = 1;
            displaySecurityReport(reviewWithWarnCtx.warning->predefinedSet);
        }
        else {
            displayInfosListModal(activeTipBox.modalTitle, &activeTipBox.infos, false);
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
        content.tagValueDetails.tagValueList.hideEndOfLastLine  = true;
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
        // If currentContentElementNb == 0, means not initialized, so skip
        if ((nextElementIdx >= genericContext.currentContentElementNb)
            && (genericContext.currentContentElementNb > 0)) {
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
        case EXTENDED_CENTER:
            memcpy(&pageContent->extendedCenter,
                   &p_content->content.extendedCenter,
                   sizeof(pageContent->extendedCenter));
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
                    pageContent->type = EXTENDED_CENTER;
                    prepareReviewFirstPage(&pageContent->extendedCenter.contentCenter,
                                           pair->valueIcon,
                                           pair->item,
                                           pair->value);

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
                p_tagValueList->hideEndOfLastLine  = true;
                p_tagValueList->wrapping           = p_content->content.tagValueList.wrapping;
            }

            break;
        }
        case TAG_VALUE_CONFIRM: {
            nbgl_contentTagValueList_t *p_tagValueList;
            // only display a TAG_VALUE_CONFIRM if we are at the last page
            if ((nextElementIdx + nbElementsInPage)
                == p_content->content.tagValueConfirm.tagValueList.nbPairs) {
                memcpy(&pageContent->tagValueConfirm,
                       &p_content->content.tagValueConfirm,
                       sizeof(pageContent->tagValueConfirm));
                p_tagValueList = &pageContent->tagValueConfirm.tagValueList;
            }
            else {
                // else display it as a TAG_VALUE_LIST
                pageContent->type = TAG_VALUE_LIST;
                p_tagValueList    = &pageContent->tagValueList;
            }
            p_tagValueList->nbPairs = nbElementsInPage;
            p_tagValueList->pairs
                = PIC(&p_content->content.tagValueConfirm.tagValueList.pairs[nextElementIdx]);
            // parse pairs to check if any contains an alias for value
            for (uint8_t i = 0; i < nbElementsInPage; i++) {
                if (p_tagValueList->pairs[i].aliasValue) {
                    p_tagValueList->token = VALUE_ALIAS_TOKEN;
                    break;
                }
            }
            // memorize pairs (or callback) for usage when alias is used
            genericContext.currentPairs    = p_tagValueList->pairs;
            genericContext.currentCallback = p_tagValueList->callback;
            break;
        }
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
            if ((p_content->content.choicesList.initChoice >= nextElementIdx)
                && (p_content->content.choicesList.initChoice
                    < nextElementIdx + nbElementsInPage)) {
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

    bool isFirstOrLastPage
        = ((p_content->type == CENTERED_INFO) || (p_content->type == EXTENDED_CENTER))
          || (p_content->type == INFO_LONG_PRESS);
    nbgl_operationType_t operationType
        = (navType == STREAMING_NAV)
              ? bundleNavContext.reviewStreaming.operationType
              : ((navType == GENERIC_NAV) ? bundleNavContext.review.operationType : 0);

    // if first or last page of review and blind/risky operation, add the warning top-right button
    if (isFirstOrLastPage
        && (operationType & (BLIND_OPERATION | RISKY_OPERATION | NO_THREAT_OPERATION))) {
        // if issue is only Web3Checks "no threat", use privacy icon
        if ((operationType & NO_THREAT_OPERATION)
            && !(reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN))) {
            pageContent->topRightIcon = &PRIVACY_ICON;
        }
        else {
            pageContent->topRightIcon = &WARNING_ICON;
        }

        pageContent->topRightToken
            = (operationType & BLIND_OPERATION) ? BLIND_WARNING_TOKEN : WARNING_BUTTON_TOKEN;
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
    else {
        // if coming from Skip modal, let's say we are at the last page
        if (pageIdx == LAST_PAGE_FOR_REVIEW) {
            pageIdx = navInfo.nbPages - 1;
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
    // if the operation is skippable
    if ((navType != STREAMING_NAV)
        && (bundleNavContext.review.operationType & SKIPPABLE_OPERATION)) {
        // only present the "Skip" header on actual tag/value pages
        if ((pageIdx > 0) && (pageIdx < (navInfo.nbPages - 1))) {
            navInfo.progressIndicator = false;
            navInfo.skipText          = "Skip";
            navInfo.skipToken         = SKIP_TOKEN;
        }
        else {
            navInfo.skipText = NULL;
        }
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
                                        detailsContext.wrapping);
            // don't start next page on a \n
            if (currentChar[len] == '\n') {
                len++;
            }
            else if (detailsContext.wrapping == false) {
                len -= 3;
            }
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
    uint16_t nbLines           = nbgl_getTextNbLinesInWidth(
        SMALL_BOLD_FONT, currentPair.value, AVAILABLE_WIDTH, detailsContext.wrapping);

    if (nbLines > NB_MAX_LINES_IN_DETAILS) {
        uint16_t len;
        nbgl_getTextMaxLenInNbLines(SMALL_BOLD_FONT,
                                    currentPair.value,
                                    AVAILABLE_WIDTH,
                                    NB_MAX_LINES_IN_DETAILS,
                                    &len,
                                    detailsContext.wrapping);
        content.tagValueDetails.tagValueList.hideEndOfLastLine = false;
        // don't start next page on a \n
        if (currentPair.value[len] == '\n') {
            len++;
        }
        else if (!detailsContext.wrapping) {
            content.tagValueDetails.tagValueList.hideEndOfLastLine = true;
            len -= 3;
        }
        // memorize next position to save processing
        detailsContext.nextPageStart = currentPair.value + len;
        // use special feature to keep only NB_MAX_LINES_IN_DETAILS lines and replace the last 3
        // chars by "...", only if the next char to display in next page is not '\n'
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
static void displayFullValuePage(const char                   *backText,
                                 const char                   *aliasText,
                                 const nbgl_contentValueExt_t *extension)
{
    const char *modalTitle
        = (extension->backText != NULL) ? PIC(extension->backText) : PIC(backText);
    if (extension->aliasType == INFO_LIST_ALIAS) {
        genericContext.currentInfos = extension->infolist;
        displayInfosListModal(modalTitle, extension->infolist, true);
    }
    else if (extension->aliasType == TAG_VALUE_LIST_ALIAS) {
        genericContext.currentTagValues = extension->tagValuelist;
        displayTagValueListModal(extension->tagValuelist);
    }
    else {
        nbgl_layoutDescription_t layoutDescription = {.modal            = true,
                                                      .withLeftBorder   = true,
                                                      .onActionCallback = &modalLayoutTouchCallback,
                                                      .tapActionText    = NULL};
        nbgl_layoutHeader_t      headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                      .separationLine     = false,
                                                      .backAndText.token  = 0,
                                                      .backAndText.tuneId = TUNE_TAP_CASUAL,
                                                      .backAndText.text   = modalTitle};
        genericContext.modalLayout                 = nbgl_layoutGet(&layoutDescription);
        // add header with the tag part of the pair, to go back
        nbgl_layoutAddHeader(genericContext.modalLayout, &headerDesc);
        // add either QR Code or full value text
        if (extension->aliasType == QR_CODE_ALIAS) {
#ifdef NBGL_QRCODE
            nbgl_layoutQRCode_t qrCode
                = {.url      = extension->fullValue,
                   .text1    = (extension->title != NULL) ? extension->title : extension->fullValue,
                   .text2    = extension->explanation,
                   .centered = true,
                   .offsetY  = 0};

            nbgl_layoutAddQRCode(genericContext.modalLayout, &qrCode);
#endif  // NBGL_QRCODE
        }
        else {
            nbgl_layoutTextContent_t content = {0};
            content.title                    = aliasText;
            if (extension->aliasType == ENS_ALIAS) {
                content.info = "ENS names are resolved by Ledger backend.";
            }
            else if ((extension->aliasType == ADDRESS_BOOK_ALIAS)
                     && (extension->aliasSubName != NULL)) {
                content.descriptions[content.nbDescriptions] = extension->aliasSubName;
                content.nbDescriptions++;
            }
            else {
                content.info = extension->explanation;
            }
            // add full value text
            content.descriptions[content.nbDescriptions] = extension->fullValue;
            content.nbDescriptions++;
            nbgl_layoutAddTextContent(genericContext.modalLayout, &content);
        }
        // draw & refresh
        nbgl_layoutDraw(genericContext.modalLayout);
        nbgl_refresh();
    }
}

// function used to display the modal containing tip-box infos
static void displayInfosListModal(const char                   *modalTitle,
                                  const nbgl_contentInfoList_t *infos,
                                  bool                          fromReview)
{
    nbgl_pageNavigationInfo_t info = {.activePage                = 0,
                                      .nbPages                   = 1,
                                      .navType                   = NAV_WITH_BUTTONS,
                                      .quitToken                 = QUIT_TIPBOX_MODAL_TOKEN,
                                      .navWithButtons.navToken   = NAV_TOKEN,
                                      .navWithButtons.quitButton = false,
                                      .navWithButtons.backButton = true,
                                      .navWithButtons.quitText   = NULL,
                                      .progressIndicator         = false,
                                      .tuneId                    = TUNE_TAP_CASUAL};
    nbgl_pageContent_t        content
        = {.type                     = INFOS_LIST,
           .topRightIcon             = NULL,
           .infosList.nbInfos        = infos->nbInfos,
           .infosList.withExtensions = infos->withExtensions,
           .infosList.infoTypes      = infos->infoTypes,
           .infosList.infoContents   = infos->infoContents,
           .infosList.infoExtensions = infos->infoExtensions,
           .infosList.token          = fromReview ? INFO_ALIAS_TOKEN : INFOS_TIP_BOX_TOKEN,
           .title                    = modalTitle,
           .titleToken               = QUIT_TIPBOX_MODAL_TOKEN,
           .tuneId                   = TUNE_TAP_CASUAL};

    if (modalPageContext != NULL) {
        nbgl_pageRelease(modalPageContext);
    }
    modalPageContext = nbgl_pageDrawGenericContentExt(&pageModalCallback, &info, &content, true);

    nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
}

// function used to display the modal containing alias tag-value pairs
static void displayTagValueListModalPage(uint8_t pageIdx, bool forceFullRefresh)
{
    nbgl_pageNavigationInfo_t info    = {.activePage                = pageIdx,
                                         .nbPages                   = detailsContext.nbPages,
                                         .navType                   = NAV_WITH_BUTTONS,
                                         .quitToken                 = QUIT_TOKEN,
                                         .navWithButtons.navToken   = MODAL_NAV_TOKEN,
                                         .navWithButtons.quitButton = true,
                                         .navWithButtons.backButton = true,
                                         .navWithButtons.quitText   = NULL,
                                         .progressIndicator         = false,
                                         .tuneId                    = TUNE_TAP_CASUAL};
    nbgl_pageContent_t        content = {.type                           = TAG_VALUE_LIST,
                                         .topRightIcon                   = NULL,
                                         .tagValueList.smallCaseForValue = true,
                                         .tagValueList.wrapping          = detailsContext.wrapping};
    uint8_t                   nbElementsInPage;

    // if first page or forward
    if (detailsContext.currentPage <= pageIdx) {
        modalContextGetPageInfo(pageIdx, &nbElementsInPage);
    }
    else {
        // backward direction
        modalContextGetPageInfo(pageIdx + 1, &nbElementsInPage);
        detailsContext.currentPairIdx -= nbElementsInPage;
        modalContextGetPageInfo(pageIdx, &nbElementsInPage);
        detailsContext.currentPairIdx -= nbElementsInPage;
    }
    detailsContext.currentPage = pageIdx;

    content.tagValueList.pairs
        = &genericContext.currentTagValues->pairs[detailsContext.currentPairIdx];
    content.tagValueList.nbPairs = nbElementsInPage;
    detailsContext.currentPairIdx += nbElementsInPage;
    if (info.nbPages == 1) {
        // if only one page, no navigation bar, and use a footer instead
        info.navWithButtons.quitText = "Close";
    }

    if (modalPageContext != NULL) {
        nbgl_pageRelease(modalPageContext);
    }
    modalPageContext = nbgl_pageDrawGenericContentExt(&pageModalCallback, &info, &content, true);

    if (forceFullRefresh) {
        nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
    }
    else {
        nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
    }
}

#ifdef NBGL_QRCODE
static void displayAddressQRCode(void)
{
    // display the address as QR Code
    nbgl_layoutDescription_t layoutDescription = {.modal            = true,
                                                  .withLeftBorder   = true,
                                                  .onActionCallback = &modalLayoutTouchCallback,
                                                  .tapActionText    = NULL};
    nbgl_layoutHeader_t      headerDesc        = {
                    .type = HEADER_EMPTY, .separationLine = false, .emptySpace.height = SMALL_CENTERING_HEADER};
    nbgl_layoutQRCode_t qrCode = {.url      = addressConfirmationContext.tagValuePairs[0].value,
                                  .text1    = NULL,
                                  .centered = true,
                                  .offsetY  = 0};

    addressConfirmationContext.modalLayout = nbgl_layoutGet(&layoutDescription);
    // add empty header for better look
    nbgl_layoutAddHeader(addressConfirmationContext.modalLayout, &headerDesc);
    // compute nb lines to check whether it shall be shorten (max is 3 lines)
    uint16_t nbLines = nbgl_getTextNbLinesInWidth(SMALL_REGULAR_FONT,
                                                  addressConfirmationContext.tagValuePairs[0].value,
                                                  AVAILABLE_WIDTH,
                                                  false);

    if (nbLines <= QRCODE_NB_MAX_LINES) {
        qrCode.text2 = addressConfirmationContext.tagValuePairs[0].value;  // in gray
    }
    else {
        // only keep beginning and end of text, and add ... in the middle
        nbgl_textReduceOnNbLines(SMALL_REGULAR_FONT,
                                 addressConfirmationContext.tagValuePairs[0].value,
                                 AVAILABLE_WIDTH,
                                 QRCODE_NB_MAX_LINES,
                                 reducedAddress,
                                 QRCODE_REDUCED_ADDR_LEN);
        qrCode.text2 = reducedAddress;  // in gray
    }

    nbgl_layoutAddQRCode(addressConfirmationContext.modalLayout, &qrCode);

    nbgl_layoutAddFooter(
        addressConfirmationContext.modalLayout, "Close", DISMISS_QR_TOKEN, TUNE_TAP_CASUAL);
    nbgl_layoutDraw(addressConfirmationContext.modalLayout);
    nbgl_refresh();
}

#endif  // NBGL_QRCODE

// called when header is touched on modal page, to dismiss it
static void modalLayoutTouchCallback(int token, uint8_t index)
{
    UNUSED(index);
    if (token == DISMISS_QR_TOKEN) {
        // dismiss modal
        nbgl_layoutRelease(addressConfirmationContext.modalLayout);
        addressConfirmationContext.modalLayout = NULL;
        nbgl_screenRedraw();
    }
    else if (token == DISMISS_WARNING_TOKEN) {
        // dismiss modal
        nbgl_layoutRelease(reviewWithWarnCtx.modalLayout);
        // if already at first level, simply redraw the background
        if (reviewWithWarnCtx.securityReportLevel <= 1) {
            reviewWithWarnCtx.modalLayout = NULL;
            nbgl_screenRedraw();
        }
        else {
            // We are at level 2 of warning modal, so re-display the level 1
            reviewWithWarnCtx.securityReportLevel = 1;
            // if preset is used
            if (reviewWithWarnCtx.warning->predefinedSet) {
                displaySecurityReport(reviewWithWarnCtx.warning->predefinedSet);
            }
            else {
                // use customized warning
                const nbgl_warningDetails_t *details
                    = (reviewWithWarnCtx.isIntro) ? reviewWithWarnCtx.warning->introDetails
                                                  : reviewWithWarnCtx.warning->reviewDetails;
                displayCustomizedSecurityReport(details);
            }
            return;
        }
    }
    else if (token == DISMISS_DETAILS_TOKEN) {
        // dismiss modal
        nbgl_layoutRelease(choiceWithDetailsCtx.modalLayout);
        // if already at first level, simply redraw the background
        if (choiceWithDetailsCtx.level <= 1) {
            choiceWithDetailsCtx.modalLayout = NULL;
            nbgl_screenRedraw();
        }
        else {
            // We are at level 2 of details modal, so re-display the level 1
            choiceWithDetailsCtx.level = 1;
            choiceWithDetailsCtx.modalLayout
                = displayModalDetails(choiceWithDetailsCtx.details, DISMISS_DETAILS_TOKEN);
        }
    }
    else if ((token >= FIRST_WARN_BAR_TOKEN) && (token <= LAST_WARN_BAR_TOKEN)) {
        if (sharedContext.usage == SHARE_CTX_REVIEW_WITH_WARNING) {
            // dismiss modal before creating a new one
            nbgl_layoutRelease(reviewWithWarnCtx.modalLayout);
            reviewWithWarnCtx.securityReportLevel = 2;
            // if preset is used
            if (reviewWithWarnCtx.warning->predefinedSet) {
                displaySecurityReport(1 << (token - FIRST_WARN_BAR_TOKEN));
            }
            else {
                // use customized warning
                const nbgl_warningDetails_t *details
                    = (reviewWithWarnCtx.isIntro) ? reviewWithWarnCtx.warning->introDetails
                                                  : reviewWithWarnCtx.warning->reviewDetails;
                if (details->type == BAR_LIST_WARNING) {
                    displayCustomizedSecurityReport(
                        &details->barList.details[token - FIRST_WARN_BAR_TOKEN]);
                }
            }
        }
        else if (sharedContext.usage == SHARE_CTX_CHOICE_WITH_DETAILS) {
            const nbgl_warningDetails_t *details
                = &choiceWithDetailsCtx.details->barList.details[token - FIRST_WARN_BAR_TOKEN];
            if (details->title != NO_TYPE_WARNING) {
                // dismiss modal before creating a new one
                nbgl_layoutRelease(choiceWithDetailsCtx.modalLayout);
                choiceWithDetailsCtx.level = 2;
                choiceWithDetailsCtx.modalLayout
                    = displayModalDetails(details, DISMISS_DETAILS_TOKEN);
            }
        }
        return;
    }
    else {
        // dismiss modal
        nbgl_layoutRelease(genericContext.modalLayout);
        genericContext.modalLayout = NULL;
        nbgl_screenRedraw();
    }
    nbgl_refresh();
}

// called when item are touched in layout
static void layoutTouchCallback(int token, uint8_t index)
{
    // choice buttons in initial warning page
    if (token == WARNING_CHOICE_TOKEN) {
        if (index == 0) {  // top button to exit
            reviewWithWarnCtx.choiceCallback(false);
        }
        else {  // bottom button to continue to review
            reviewWithWarnCtx.isIntro = false;
            if (reviewWithWarnCtx.isStreaming) {
                useCaseReviewStreamingStart(reviewWithWarnCtx.operationType,
                                            reviewWithWarnCtx.icon,
                                            reviewWithWarnCtx.reviewTitle,
                                            reviewWithWarnCtx.reviewSubTitle,
                                            reviewWithWarnCtx.choiceCallback,
                                            false);
            }
            else {
                useCaseReview(reviewWithWarnCtx.operationType,
                              reviewWithWarnCtx.tagValueList,
                              reviewWithWarnCtx.icon,
                              reviewWithWarnCtx.reviewTitle,
                              reviewWithWarnCtx.reviewSubTitle,
                              reviewWithWarnCtx.finishTitle,
                              &activeTipBox,
                              reviewWithWarnCtx.choiceCallback,
                              false,
                              false);
            }
        }
    }
    // top-right button in initial warning page
    else if (token == WARNING_BUTTON_TOKEN) {
        // start directly at first level of security report
        reviewWithWarnCtx.securityReportLevel = 1;
        // if preset is used
        if (reviewWithWarnCtx.warning->predefinedSet) {
            displaySecurityReport(reviewWithWarnCtx.warning->predefinedSet);
        }
        else {
            // use customized warning
            const nbgl_warningDetails_t *details = (reviewWithWarnCtx.isIntro)
                                                       ? reviewWithWarnCtx.warning->introDetails
                                                       : reviewWithWarnCtx.warning->reviewDetails;
            displayCustomizedSecurityReport(details);
        }
    }
    // top-right button in choice with details page
    else if (token == CHOICE_DETAILS_TOKEN) {
        choiceWithDetailsCtx.level = 1;
        choiceWithDetailsCtx.modalLayout
            = displayModalDetails(choiceWithDetailsCtx.details, DISMISS_DETAILS_TOKEN);
    }
    // main prelude page
    else if (token == PRELUDE_CHOICE_TOKEN) {
        if (index == 0) {  // top button to display details about prelude
            displayCustomizedSecurityReport(reviewWithWarnCtx.warning->prelude->details);
        }
        else {  // footer to continue to review
            if (HAS_INITIAL_WARNING(reviewWithWarnCtx.warning)) {
                displayInitialWarning();
            }
            else {
                reviewWithWarnCtx.isIntro = false;
                if (reviewWithWarnCtx.isStreaming) {
                    useCaseReviewStreamingStart(reviewWithWarnCtx.operationType,
                                                reviewWithWarnCtx.icon,
                                                reviewWithWarnCtx.reviewTitle,
                                                reviewWithWarnCtx.reviewSubTitle,
                                                reviewWithWarnCtx.choiceCallback,
                                                false);
                }
                else {
                    useCaseReview(reviewWithWarnCtx.operationType,
                                  reviewWithWarnCtx.tagValueList,
                                  reviewWithWarnCtx.icon,
                                  reviewWithWarnCtx.reviewTitle,
                                  reviewWithWarnCtx.reviewSubTitle,
                                  reviewWithWarnCtx.finishTitle,
                                  &activeTipBox,
                                  reviewWithWarnCtx.choiceCallback,
                                  false,
                                  false);
                }
            }
        }
    }
    // choice buttons
    else if (token == CHOICE_TOKEN) {
        if (onChoice != NULL) {
            onChoice((index == 0) ? true : false);
        }
    }
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
        .centeredInfo.icon    = &IMPORTANT_CIRCLE_ICON,
        .centeredInfo.offsetY = 0,
        .confirmationText     = "Yes, skip",
        .confirmationToken    = SKIP_TOKEN,
        .tuneId               = TUNE_TAP_CASUAL,
        .modal                = true};
    if (modalPageContext != NULL) {
        nbgl_pageRelease(modalPageContext);
    }
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

static void keypadGeneric_cb(int token, uint8_t index)
{
    UNUSED(index);
    if (token != BACK_TOKEN) {
        return;
    }
    onQuit();
}
#endif

/**
 * @brief computes the number of tag/values pairs displayable in a page, with the given list of
 * tag/value pairs
 *
 * @param nbPairs number of tag/value pairs to use in \b tagValueList
 * @param tagValueList list of tag/value pairs
 * @param startIndex first index to consider in \b tagValueList
 * @param isSkippable if true, a skip header is added
 * @param hasConfirmationButton if true, a confirmation button is added at last page
 * @param requireSpecificDisplay (output) set to true if the tag/value needs a specific display:
 *        - centeredInfo flag is enabled
 *        - the tag/value doesn't fit in a page
 * @return the number of tag/value pairs fitting in a page
 */
static uint8_t getNbTagValuesInPage(uint8_t                           nbPairs,
                                    const nbgl_contentTagValueList_t *tagValueList,
                                    uint8_t                           startIndex,
                                    bool                              isSkippable,
                                    bool                              hasConfirmationButton,
                                    bool                              hasDetailsButton,
                                    bool                             *requireSpecificDisplay)
{
    uint8_t  nbPairsInPage   = 0;
    uint16_t currentHeight   = PRE_TAG_VALUE_MARGIN;  // upper margin
    uint16_t maxUsableHeight = TAG_VALUE_AREA_HEIGHT;

    // if the review is skippable, it means that there is less height for tag/value pairs
    // the small centering header becomes a touchable header
    if (isSkippable) {
        maxUsableHeight -= TOUCHABLE_HEADER_BAR_HEIGHT - SMALL_CENTERING_HEADER;
    }

    *requireSpecificDisplay = false;
    while (nbPairsInPage < nbPairs) {
        const nbgl_layoutTagValue_t *pair;
        nbgl_font_id_e               value_font;
        uint16_t                     nbLines;

        // margin between pairs
        // 12 or 24 px between each tag/value pair
        if (nbPairsInPage > 0) {
            currentHeight += INTER_TAG_VALUE_MARGIN;
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
        currentHeight += TAG_VALUE_INTERVALE;
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

        // potential subAlias text
        if ((pair->aliasValue) && (pair->extension->aliasSubName)) {
            currentHeight += TAG_VALUE_INTERVALE + nbgl_getFontLineHeight(SMALL_REGULAR_FONT);
        }
        // nb lines for value
        nbLines = nbgl_getTextNbLinesInWidth(
            value_font, pair->value, AVAILABLE_WIDTH, tagValueList->wrapping);
        if ((currentHeight >= maxUsableHeight) || (nbLines > NB_MAX_LINES_IN_REVIEW)) {
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
    // if this is a TAG_VALUE_CONFIRM and we have reached the last pairs,
    // let's check if it still fits with a CONFIRMATION button, and if not,
    // remove the last pair
    if (hasConfirmationButton && (nbPairsInPage == nbPairs)) {
        maxUsableHeight -= UP_FOOTER_BUTTON_HEIGHT;
        if (currentHeight > maxUsableHeight) {
            nbPairsInPage--;
        }
    }
    // do the same with just a details button
    else if (hasDetailsButton) {
        maxUsableHeight -= (SMALL_BUTTON_RADIUS * 2);
        if (currentHeight > maxUsableHeight) {
            nbPairsInPage--;
        }
    }
    return nbPairsInPage;
}

/**
 * @brief computes the number of tag/values pairs displayable in a details page, with the given list
 * of tag/value pairs
 *
 * @param nbPairs number of tag/value pairs to use in \b tagValueList
 * @param tagValueList list of tag/value pairs
 * @param startIndex first index to consider in \b tagValueList
 * @return the number of tag/value pairs fitting in a page
 */
static uint8_t getNbTagValuesInDetailsPage(uint8_t                           nbPairs,
                                           const nbgl_contentTagValueList_t *tagValueList,
                                           uint8_t                           startIndex)
{
    uint8_t  nbPairsInPage   = 0;
    uint16_t currentHeight   = PRE_TAG_VALUE_MARGIN;  // upper margin
    uint16_t maxUsableHeight = TAG_VALUE_AREA_HEIGHT;

    while (nbPairsInPage < nbPairs) {
        const nbgl_layoutTagValue_t *pair;

        // margin between pairs
        // 12 or 24 px between each tag/value pair
        if (nbPairsInPage > 0) {
            currentHeight += INTER_TAG_VALUE_MARGIN;
        }
        // fetch tag/value pair strings.
        if (tagValueList->pairs != NULL) {
            pair = PIC(&tagValueList->pairs[startIndex + nbPairsInPage]);
        }
        else {
            pair = PIC(tagValueList->callback(startIndex + nbPairsInPage));
        }

        // tag height
        currentHeight += nbgl_getTextHeightInWidth(
            SMALL_REGULAR_FONT, pair->item, AVAILABLE_WIDTH, tagValueList->wrapping);
        // space between tag and value
        currentHeight += 4;

        // value height
        currentHeight += nbgl_getTextHeightInWidth(
            SMALL_REGULAR_FONT, pair->value, AVAILABLE_WIDTH, tagValueList->wrapping);

        // we have reached the maximum height, it means than there are to many pairs
        if (currentHeight >= maxUsableHeight) {
            break;
        }
        nbPairsInPage++;
    }
    return nbPairsInPage;
}

static uint8_t getNbPagesForContent(const nbgl_content_t *content,
                                    uint8_t               pageIdxStart,
                                    bool                  isLast,
                                    bool                  isSkippable)
{
    uint8_t nbElements = 0;
    uint8_t nbPages    = 0;
    uint8_t nbElementsInPage;
    uint8_t elemIdx = 0;
    bool    flag;

    nbElements = getContentNbElement(content);

    while (nbElements > 0) {
        flag = 0;
        // if the current page is not the first one (or last), a navigation bar exists
        bool hasNav = !isLast || (pageIdxStart > 0) || (elemIdx > 0);
        if (content->type == TAG_VALUE_LIST) {
            nbElementsInPage = getNbTagValuesInPage(nbElements,
                                                    &content->content.tagValueList,
                                                    elemIdx,
                                                    isSkippable,
                                                    false,
                                                    false,
                                                    &flag);
        }
        else if (content->type == TAG_VALUE_CONFIRM) {
            nbElementsInPage = getNbTagValuesInPage(nbElements,
                                                    &content->content.tagValueConfirm.tagValueList,
                                                    elemIdx,
                                                    isSkippable,
                                                    isLast,
                                                    !isLast,
                                                    &flag);
        }
        else if (content->type == INFOS_LIST) {
            nbElementsInPage = nbgl_useCaseGetNbInfosInPage(
                nbElements, &content->content.infosList, elemIdx, hasNav);
        }
        else if (content->type == SWITCHES_LIST) {
            nbElementsInPage = nbgl_useCaseGetNbSwitchesInPage(
                nbElements, &content->content.switchesList, elemIdx, hasNav);
        }
        else if (content->type == BARS_LIST) {
            nbElementsInPage = nbgl_useCaseGetNbBarsInPage(
                nbElements, &content->content.barsList, elemIdx, hasNav);
        }
        else if (content->type == CHOICES_LIST) {
            nbElementsInPage = nbgl_useCaseGetNbChoicesInPage(
                nbElements, &content->content.choicesList, elemIdx, hasNav);
        }
        else {
            nbElementsInPage = MIN(nbMaxElementsPerContentType[content->type], nbElements);
        }

        elemIdx += nbElementsInPage;
        genericContextSetPageInfo(pageIdxStart + nbPages, nbElementsInPage, flag);
        nbElements -= nbElementsInPage;
        nbPages++;
    }

    return nbPages;
}

static uint8_t getNbPagesForGenericContents(const nbgl_genericContents_t *genericContents,
                                            uint8_t                       pageIdxStart,
                                            bool                          isSkippable)
{
    uint8_t               nbPages = 0;
    nbgl_content_t        content;
    const nbgl_content_t *p_content;

    for (int i = 0; i < genericContents->nbContents; i++) {
        p_content = getContentAtIdx(genericContents, i, &content);
        if (p_content == NULL) {
            return 0;
        }
        nbPages += getNbPagesForContent(p_content,
                                        pageIdxStart + nbPages,
                                        (i == (genericContents->nbContents - 1)),
                                        isSkippable);
    }

    return nbPages;
}

static void prepareAddressConfirmationPages(const char                       *address,
                                            const nbgl_contentTagValueList_t *tagValueList,
                                            nbgl_content_t                   *firstPageContent,
                                            nbgl_content_t                   *secondPageContent)
{
    nbgl_contentTagValueConfirm_t *tagValueConfirm;

    addressConfirmationContext.tagValuePairs[0].item  = "Address";
    addressConfirmationContext.tagValuePairs[0].value = address;
    addressConfirmationContext.nbPairs                = 1;

    // First page
    firstPageContent->type = TAG_VALUE_CONFIRM;
    tagValueConfirm        = &firstPageContent->content.tagValueConfirm;

#ifdef NBGL_QRCODE
    tagValueConfirm->detailsButtonIcon = &QRCODE_ICON;
    // only use "Show as QR" when address & pairs are not fitting in a single page
    if ((tagValueList != NULL) && (tagValueList->nbPairs < ADDR_VERIF_NB_PAIRS)) {
        nbgl_contentTagValueList_t tmpList;
        bool                       flag;
        // copy in intermediate structure
        for (uint8_t i = 0; i < tagValueList->nbPairs; i++) {
            memcpy(&addressConfirmationContext.tagValuePairs[1 + i],
                   &tagValueList->pairs[i],
                   sizeof(nbgl_contentTagValue_t));
            addressConfirmationContext.nbPairs++;
        }
        // check how many can fit in a page
        memcpy(&tmpList, tagValueList, sizeof(nbgl_contentTagValueList_t));
        tmpList.nbPairs                    = addressConfirmationContext.nbPairs;
        tmpList.pairs                      = addressConfirmationContext.tagValuePairs;
        addressConfirmationContext.nbPairs = getNbTagValuesInPage(
            addressConfirmationContext.nbPairs, &tmpList, 0, false, true, true, &flag);
        // if they don't all fit, keep only the address
        if (tmpList.nbPairs > addressConfirmationContext.nbPairs) {
            addressConfirmationContext.nbPairs = 1;
        }
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
    tagValueConfirm->tagValueList.nbPairs            = addressConfirmationContext.nbPairs;
    tagValueConfirm->tagValueList.pairs              = addressConfirmationContext.tagValuePairs;
    tagValueConfirm->tagValueList.smallCaseForValue  = false;
    tagValueConfirm->tagValueList.nbMaxLinesForValue = 0;
    tagValueConfirm->tagValueList.wrapping           = false;
    // if it's an extended address verif, it takes 2 pages, so display a "Tap to continue", and
    // no confirmation button
    if ((tagValueList != NULL)
        && (tagValueList->nbPairs > (addressConfirmationContext.nbPairs - 1))) {
        tagValueConfirm->detailsButtonText = "Show as QR";
        tagValueConfirm->confirmationText  = NULL;
        // the second page is dedicated to the extended tag/value pairs
        secondPageContent->type            = TAG_VALUE_CONFIRM;
        tagValueConfirm                    = &secondPageContent->content.tagValueConfirm;
        tagValueConfirm->confirmationText  = "Confirm";
        tagValueConfirm->confirmationToken = CONFIRM_TOKEN;
        tagValueConfirm->detailsButtonText = NULL;
        tagValueConfirm->detailsButtonIcon = NULL;
        tagValueConfirm->tuneId            = TUNE_TAP_CASUAL;
        memcpy(&tagValueConfirm->tagValueList, tagValueList, sizeof(nbgl_contentTagValueList_t));
        tagValueConfirm->tagValueList.nbPairs
            = tagValueList->nbPairs - (addressConfirmationContext.nbPairs - 1);
        tagValueConfirm->tagValueList.pairs
            = &tagValueList->pairs[addressConfirmationContext.nbPairs - 1];
    }
    else {
        // otherwise no tap to continue but a confirmation button
        tagValueConfirm->confirmationText  = "Confirm";
        tagValueConfirm->confirmationToken = CONFIRM_TOKEN;
    }
}

static void bundleNavStartHome(void)
{
    nbgl_homeAndSettingsContext_t *context = &bundleNavContext.homeAndSettings;

    useCaseHomeExt(context->appName,
                   context->appIcon,
                   context->tagline,
                   context->settingContents != NULL ? true : false,
                   &context->homeAction,
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
    operationType
        &= ~(SKIPPABLE_OPERATION | BLIND_OPERATION | RISKY_OPERATION | NO_THREAT_OPERATION);
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
        bundleNavContext.reviewStreaming.choiceCallback(true);
    }
    else {
        bundleNavReviewAskRejectionConfirmation(bundleNavContext.reviewStreaming.operationType,
                                                bundleNavReviewStreamingConfirmRejection);
    }
}

// function used to display the details in modal (from Choice with details or from Customized
// security report) it can be either a single page with text or QR code, or a list of touchable bars
// leading to single pages
static nbgl_layout_t *displayModalDetails(const nbgl_warningDetails_t *details, uint8_t token)
{
    nbgl_layoutDescription_t layoutDescription = {.modal            = true,
                                                  .withLeftBorder   = true,
                                                  .onActionCallback = modalLayoutTouchCallback,
                                                  .tapActionText    = NULL};
    nbgl_layoutHeader_t      headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                  .separationLine     = true,
                                                  .backAndText.icon   = NULL,
                                                  .backAndText.tuneId = TUNE_TAP_CASUAL,
                                                  .backAndText.token  = token};
    uint8_t                  i;
    nbgl_layout_t           *layout;

    layout                      = nbgl_layoutGet(&layoutDescription);
    headerDesc.backAndText.text = details->title;
    nbgl_layoutAddHeader(layout, &headerDesc);
    if (details->type == BAR_LIST_WARNING) {
        // if more than one warning, so use a list of touchable bars
        for (i = 0; i < details->barList.nbBars; i++) {
            nbgl_layoutBar_t bar;
            bar.text    = details->barList.texts[i];
            bar.subText = details->barList.subTexts[i];
            bar.iconRight
                = (details->barList.details[i].type != NO_TYPE_WARNING) ? &PUSH_ICON : NULL;
            bar.iconLeft = details->barList.icons[i];
            bar.token    = FIRST_WARN_BAR_TOKEN + i;
            bar.tuneId   = TUNE_TAP_CASUAL;
            bar.large    = false;
            bar.inactive = false;
            nbgl_layoutAddTouchableBar(layout, &bar);
            nbgl_layoutAddSeparationLine(layout);
        }
    }
    else if (details->type == QRCODE_WARNING) {
#ifdef NBGL_QRCODE
        // display a QR Code
        nbgl_layoutAddQRCode(layout, &details->qrCode);
#endif  // NBGL_QRCODE
        headerDesc.backAndText.text = details->title;
    }
    else if (details->type == CENTERED_INFO_WARNING) {
        // display a centered info
        nbgl_layoutAddContentCenter(layout, &details->centeredInfo);
        headerDesc.separationLine = false;
    }
    nbgl_layoutDraw(layout);
    nbgl_refresh();
    return layout;
}

// function used to display the security level page in modal
// it can be either a single page with text or QR code, or a list of touchable bars leading
// to single pages
static void displaySecurityReport(uint32_t set)
{
    nbgl_layoutDescription_t layoutDescription = {.modal            = true,
                                                  .withLeftBorder   = true,
                                                  .onActionCallback = modalLayoutTouchCallback,
                                                  .tapActionText    = NULL};
    nbgl_layoutHeader_t      headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                  .separationLine     = true,
                                                  .backAndText.icon   = NULL,
                                                  .backAndText.tuneId = TUNE_TAP_CASUAL,
                                                  .backAndText.token  = DISMISS_WARNING_TOKEN};
    nbgl_layoutFooter_t      footerDesc
        = {.type = FOOTER_EMPTY, .separationLine = false, .emptySpace.height = 0};
    uint8_t     i;
    const char *provider;

    reviewWithWarnCtx.modalLayout = nbgl_layoutGet(&layoutDescription);

    // display a list of touchable bars in some conditions:
    // - we must be in the first level of security report
    // - and be in the review itself (not from the intro/warning screen)
    //
    if ((reviewWithWarnCtx.securityReportLevel == 1) && (!reviewWithWarnCtx.isIntro)) {
        for (i = 0; i < NB_WARNING_TYPES; i++) {
            // Skip GATED_SIGNING_WARN from security report bars
            if ((i != GATED_SIGNING_WARN)
                && (reviewWithWarnCtx.warning->predefinedSet & (1 << i))) {
                nbgl_layoutBar_t bar = {0};
                if ((i == BLIND_SIGNING_WARN) || (i == W3C_NO_THREAT_WARN) || (i == W3C_ISSUE_WARN)
                    || (reviewWithWarnCtx.warning->providerMessage == NULL)) {
                    bar.subText = securityReportItems[i].subText;
                }
                else {
                    bar.subText = reviewWithWarnCtx.warning->providerMessage;
                }
                bar.text = securityReportItems[i].text;
                if (i != W3C_NO_THREAT_WARN) {
                    bar.iconRight = &PUSH_ICON;
                    bar.token     = FIRST_WARN_BAR_TOKEN + i;
                    bar.tuneId    = TUNE_TAP_CASUAL;
                }
                else {
                    bar.token = NBGL_INVALID_TOKEN;
                }
                bar.iconLeft = securityReportItems[i].icon;
                nbgl_layoutAddTouchableBar(reviewWithWarnCtx.modalLayout, &bar);
                nbgl_layoutAddSeparationLine(reviewWithWarnCtx.modalLayout);
            }
        }
        headerDesc.backAndText.text = "Security report";
        nbgl_layoutAddHeader(reviewWithWarnCtx.modalLayout, &headerDesc);
        nbgl_layoutDraw(reviewWithWarnCtx.modalLayout);
        nbgl_refresh();
        return;
    }
    if (reviewWithWarnCtx.warning && reviewWithWarnCtx.warning->reportProvider) {
        provider = reviewWithWarnCtx.warning->reportProvider;
    }
    else {
        provider = "[unknown]";
    }
    if ((set & (1 << W3C_THREAT_DETECTED_WARN)) || (set & (1 << W3C_RISK_DETECTED_WARN))) {
        size_t urlLen = 0;
        char  *destStr
            = tmpString
              + W3C_DESCRIPTION_MAX_LEN / 2;  // use the second half of tmpString for strings
#ifdef NBGL_QRCODE
        // display a QR Code
        nbgl_layoutQRCode_t qrCode = {.url      = destStr,
                                      .text1    = reviewWithWarnCtx.warning->reportUrl,
                                      .text2    = "Scan to view full report",
                                      .centered = true,
                                      .offsetY  = 0};
        // add "https://"" as prefix of the given URL
        snprintf(destStr,
                 W3C_DESCRIPTION_MAX_LEN / 2,
                 "https://%s",
                 reviewWithWarnCtx.warning->reportUrl);
        urlLen = strlen(destStr) + 1;
        nbgl_layoutAddQRCode(reviewWithWarnCtx.modalLayout, &qrCode);
        footerDesc.emptySpace.height = 24;
#endif  // NBGL_QRCODE
        // use the next part of destStr for back bar text
        snprintf(destStr + urlLen, W3C_DESCRIPTION_MAX_LEN / 2 - urlLen, "%s report", provider);
        headerDesc.backAndText.text = destStr + urlLen;
    }
    else if (set & (1 << BLIND_SIGNING_WARN)) {
        // display a centered if in review
        nbgl_contentCenter_t info = {0};
        info.icon                 = &LARGE_WARNING_ICON;
        info.title                = "This transaction cannot be Clear Signed";
        info.description
            = "This transaction or message cannot be decoded fully. If you choose to sign, you "
              "could be authorizing malicious actions that can drain your wallet.\n\nLearn more: "
              "ledger.com/e8";
        nbgl_layoutAddContentCenter(reviewWithWarnCtx.modalLayout, &info);
        footerDesc.emptySpace.height = SMALL_CENTERING_HEADER;
        headerDesc.separationLine    = false;
    }
    else if (set & (1 << W3C_ISSUE_WARN)) {
        // if W3 Checks issue, display a centered info
        nbgl_contentCenter_t info = {0};
        info.icon                 = &LARGE_WARNING_ICON;
        info.title                = "Transaction Check unavailable";
        info.description
            = "If you're not using the Ledger Wallet app, Transaction Check might not work. If "
              "you "
              "are using Ledger Wallet, reject the transaction and try again.\n\nGet help at "
              "ledger.com/e11";
        nbgl_layoutAddContentCenter(reviewWithWarnCtx.modalLayout, &info);
        footerDesc.emptySpace.height = SMALL_CENTERING_HEADER;
        headerDesc.separationLine    = false;
    }
    nbgl_layoutAddHeader(reviewWithWarnCtx.modalLayout, &headerDesc);
    if (footerDesc.emptySpace.height > 0) {
        nbgl_layoutAddExtendedFooter(reviewWithWarnCtx.modalLayout, &footerDesc);
    }
    nbgl_layoutDraw(reviewWithWarnCtx.modalLayout);
    nbgl_refresh();
}

// function used to display the security level page in modal
// it can be either a single page with text or QR code, or a list of touchable bars leading
// to single pages
static void displayCustomizedSecurityReport(const nbgl_warningDetails_t *details)
{
    reviewWithWarnCtx.modalLayout = displayModalDetails(details, DISMISS_WARNING_TOKEN);
}

// function used to display the initial warning page when starting a "review with warning"
static void displayInitialWarning(void)
{
    // Play notification sound
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tune = TUNE_RESERVED;
#endif  // HAVE_PIEZO_SOUND
    nbgl_layoutDescription_t   layoutDescription;
    nbgl_layoutChoiceButtons_t buttonsInfo = {.bottomText = "Continue anyway",
                                              .token      = WARNING_CHOICE_TOKEN,
                                              .topText    = "Back to safety",
                                              .style      = ROUNDED_AND_FOOTER_STYLE,
                                              .tuneId     = TUNE_TAP_CASUAL};
    nbgl_layoutHeader_t        headerDesc  = {.type              = HEADER_EMPTY,
                                              .separationLine    = false,
                                              .emptySpace.height = MEDIUM_CENTERING_HEADER};
    uint32_t                   set         = reviewWithWarnCtx.warning->predefinedSet
                   & ~((1 << W3C_NO_THREAT_WARN) | (1 << W3C_ISSUE_WARN));

    bool isBlindSigningOnly
        = (set != 0) && ((set & ~((1 << BLIND_SIGNING_WARN) | (1 << GATED_SIGNING_WARN))) == 0);
    reviewWithWarnCtx.isIntro = true;

    layoutDescription.modal          = false;
    layoutDescription.withLeftBorder = true;

    layoutDescription.onActionCallback = layoutTouchCallback;
    layoutDescription.tapActionText    = NULL;

    layoutDescription.ticker.tickerCallback = NULL;
    reviewWithWarnCtx.layoutCtx             = nbgl_layoutGet(&layoutDescription);

    nbgl_layoutAddHeader(reviewWithWarnCtx.layoutCtx, &headerDesc);
    if (reviewWithWarnCtx.warning->predefinedSet != 0) {
        // icon is different in casee of Blind Siging or Gated Signing
        nbgl_layoutAddTopRightButton(reviewWithWarnCtx.layoutCtx,
                                     isBlindSigningOnly ? &INFO_I_ICON : &QRCODE_ICON,
                                     WARNING_BUTTON_TOKEN,
                                     TUNE_TAP_CASUAL);
    }
    else if (reviewWithWarnCtx.warning->introTopRightIcon != NULL) {
        nbgl_layoutAddTopRightButton(reviewWithWarnCtx.layoutCtx,
                                     reviewWithWarnCtx.warning->introTopRightIcon,
                                     WARNING_BUTTON_TOKEN,
                                     TUNE_TAP_CASUAL);
    }
    // add main content
    // if predefined content is configured, use it preferably
    if (reviewWithWarnCtx.warning->predefinedSet != 0) {
        nbgl_contentCenter_t info = {0};

        const char *provider;

        // default icon
        info.icon = &LARGE_WARNING_ICON;

        // use small title only if not Blind signing and not Gated signing
        if (isBlindSigningOnly == false) {
            if (reviewWithWarnCtx.warning->reportProvider) {
                provider = reviewWithWarnCtx.warning->reportProvider;
            }
            else {
                provider = "[unknown]";
            }
            info.smallTitle = tmpString;
            snprintf(tmpString, W3C_DESCRIPTION_MAX_LEN, "Detected by %s", provider);
        }

        // If Blind Signing or Gated Signing
        if (isBlindSigningOnly) {
            info.title = "Blind signing ahead";
            if (set & (1 << GATED_SIGNING_WARN)) {
                info.description
                    = "If you sign this transaction, you could loose your assets. "
                      "Explore safer alternatives: ledger.com/integrated-apps";
                buttonsInfo.bottomText = "Accept risk and continue";
            }
            else {
                info.description
                    = "This transaction's details are not fully verifiable. If you sign, you could "
                      "lose all your assets.";
                buttonsInfo.bottomText = "Continue anyway";
            }
#ifdef HAVE_PIEZO_SOUND
            tune = TUNE_NEUTRAL;
#endif  // HAVE_PIEZO_SOUND
        }
        else if (set & (1 << W3C_RISK_DETECTED_WARN)) {
            info.title = "Potential risk";
            if (reviewWithWarnCtx.warning->providerMessage == NULL) {
                info.description = "Unidentified risk";
            }
            else {
                info.description = reviewWithWarnCtx.warning->providerMessage;
            }
            buttonsInfo.bottomText = "Accept risk and continue";
#ifdef HAVE_PIEZO_SOUND
            tune = TUNE_NEUTRAL;
#endif  // HAVE_PIEZO_SOUND
        }
        else if (set & (1 << W3C_THREAT_DETECTED_WARN)) {
            info.title = "Critical threat";
            if (reviewWithWarnCtx.warning->providerMessage == NULL) {
                info.description = "Unidentified threat";
            }
            else {
                info.description = reviewWithWarnCtx.warning->providerMessage;
            }
            buttonsInfo.bottomText = "Accept threat and continue";
#ifdef HAVE_PIEZO_SOUND
            tune = TUNE_ERROR;
#endif  // HAVE_PIEZO_SOUND
        }
        nbgl_layoutAddContentCenter(reviewWithWarnCtx.layoutCtx, &info);
    }
    else if (reviewWithWarnCtx.warning->info != NULL) {
        // if no predefined content, use custom one
        nbgl_layoutAddContentCenter(reviewWithWarnCtx.layoutCtx, reviewWithWarnCtx.warning->info);
#ifdef HAVE_PIEZO_SOUND
        tune = TUNE_LOOK_AT_ME;
#endif  // HAVE_PIEZO_SOUND
    }
    // add button and footer on bottom
    nbgl_layoutAddChoiceButtons(reviewWithWarnCtx.layoutCtx, &buttonsInfo);

#ifdef HAVE_PIEZO_SOUND
    if (tune != TUNE_RESERVED) {
        os_io_seph_cmd_piezo_play_tune(tune);
    }
#endif  // HAVE_PIEZO_SOUND
    nbgl_layoutDraw(reviewWithWarnCtx.layoutCtx);
    nbgl_refresh();
}

// function used to display the prelude when starting a "review with warning"
static void displayPrelude(void)
{
    nbgl_layoutDescription_t   layoutDescription;
    nbgl_layoutChoiceButtons_t buttonsInfo
        = {.bottomText = reviewWithWarnCtx.warning->prelude->footerText,
           .token      = PRELUDE_CHOICE_TOKEN,
           .topText    = reviewWithWarnCtx.warning->prelude->buttonText,
           .style      = ROUNDED_AND_FOOTER_STYLE,
           .tuneId     = TUNE_TAP_CASUAL};
    nbgl_layoutHeader_t headerDesc = {.type              = HEADER_EMPTY,
                                      .separationLine    = false,
                                      .emptySpace.height = MEDIUM_CENTERING_HEADER};

    reviewWithWarnCtx.isIntro = true;

    layoutDescription.modal                 = false;
    layoutDescription.withLeftBorder        = true;
    layoutDescription.onActionCallback      = layoutTouchCallback;
    layoutDescription.tapActionText         = NULL;
    layoutDescription.ticker.tickerCallback = NULL;
    reviewWithWarnCtx.layoutCtx             = nbgl_layoutGet(&layoutDescription);

    nbgl_layoutAddHeader(reviewWithWarnCtx.layoutCtx, &headerDesc);
    // add centered content
    nbgl_contentCenter_t info = {0};

    // default icon
    info.icon = reviewWithWarnCtx.warning->prelude->icon;

    info.title       = reviewWithWarnCtx.warning->prelude->title;
    info.description = reviewWithWarnCtx.warning->prelude->description;
    nbgl_layoutAddContentCenter(reviewWithWarnCtx.layoutCtx, &info);

    // add button and footer on bottom
    nbgl_layoutAddChoiceButtons(reviewWithWarnCtx.layoutCtx, &buttonsInfo);

#ifdef HAVE_PIEZO_SOUND
    os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND
    nbgl_layoutDraw(reviewWithWarnCtx.layoutCtx);
    nbgl_refresh();
}

// function to factorize code for reviews tipbox
static void initWarningTipBox(const nbgl_tipBox_t *tipBox)
{
    const char *predefinedTipBoxText = NULL;

    // if warning is valid and a warning requires a tip-box
    if (reviewWithWarnCtx.warning) {
        if (reviewWithWarnCtx.warning->predefinedSet & (1 << W3C_ISSUE_WARN)) {
            if (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN)) {
                predefinedTipBoxText = "Transaction Check unavailable.\nBlind signing required.";
            }
            else {
                predefinedTipBoxText = "Transaction Check unavailable";
            }
        }
        else if (reviewWithWarnCtx.warning->predefinedSet & (1 << W3C_THREAT_DETECTED_WARN)) {
            if (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN)) {
                predefinedTipBoxText = "Critical threat detected.\nBlind signing required.";
            }
            else {
                predefinedTipBoxText = "Critical threat detected.";
            }
        }
        else if (reviewWithWarnCtx.warning->predefinedSet & (1 << W3C_RISK_DETECTED_WARN)) {
            if (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN)) {
                predefinedTipBoxText = "Potential risk detected.\nBlind signing required.";
            }
            else {
                predefinedTipBoxText = "Potential risk detected.";
            }
        }
        else if (reviewWithWarnCtx.warning->predefinedSet & (1 << W3C_NO_THREAT_WARN)) {
            if (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN)) {
                predefinedTipBoxText
                    = "No threat detected by Transaction Check but blind signing required.";
            }
            else {
                predefinedTipBoxText = "No threat detected by Transaction Check.";
            }
        }
        else if (reviewWithWarnCtx.warning->predefinedSet & (1 << BLIND_SIGNING_WARN)) {
            predefinedTipBoxText = "Blind signing required.";
        }
    }

    if ((tipBox != NULL) || (predefinedTipBoxText != NULL)) {
        // do not display "Swipe to review" if a tip-box is displayed
        STARTING_CONTENT.content.extendedCenter.contentCenter.subText = NULL;
        if (predefinedTipBoxText != NULL) {
            genericContext.validWarningCtx                      = true;
            STARTING_CONTENT.content.extendedCenter.tipBox.icon = NULL;
            STARTING_CONTENT.content.extendedCenter.tipBox.text = predefinedTipBoxText;
        }
        else {
            STARTING_CONTENT.content.extendedCenter.tipBox.icon = tipBox->icon;
            STARTING_CONTENT.content.extendedCenter.tipBox.text = tipBox->text;
        }
        STARTING_CONTENT.content.extendedCenter.tipBox.token  = TIP_BOX_TOKEN;
        STARTING_CONTENT.content.extendedCenter.tipBox.tuneId = TUNE_TAP_CASUAL;
    }
}

// function to factorize code for all simple reviews
static void useCaseReview(nbgl_operationType_t              operationType,
                          const nbgl_contentTagValueList_t *tagValueList,
                          const nbgl_icon_details_t        *icon,
                          const char                       *reviewTitle,
                          const char                       *reviewSubTitle,
                          const char                       *finishTitle,
                          const nbgl_tipBox_t              *tipBox,
                          nbgl_choiceCallback_t             choiceCallback,
                          bool                              isLight,
                          bool                              playNotifSound)
{
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
    STARTING_CONTENT.type = EXTENDED_CENTER;
    prepareReviewFirstPage(
        &STARTING_CONTENT.content.extendedCenter.contentCenter, icon, reviewTitle, reviewSubTitle);

    // Prepare un tipbox if needed
    initWarningTipBox(tipBox);

    // Then the tag/value pairs
    localContentsList[1].type = TAG_VALUE_LIST;
    memcpy(&localContentsList[1].content.tagValueList,
           tagValueList,
           sizeof(nbgl_contentTagValueList_t));
    localContentsList[1].contentActionCallback = tagValueList->actionCallback;

    // The last page
    if (isLight) {
        localContentsList[2].type = INFO_BUTTON;
        prepareReviewLightLastPage(
            operationType, &localContentsList[2].content.infoButton, icon, finishTitle);
    }
    else {
        localContentsList[2].type = INFO_LONG_PRESS;
        prepareReviewLastPage(
            operationType, &localContentsList[2].content.infoLongPress, icon, finishTitle);
    }

    // compute number of pages & fill navigation structure
    uint8_t nbPages = getNbPagesForGenericContents(
        &genericContext.genericContents, 0, (operationType & SKIPPABLE_OPERATION));
    prepareNavInfo(true, nbPages, getRejectReviewText(operationType));

    // Play notification sound if required
    if (playNotifSound) {
#ifdef HAVE_PIEZO_SOUND
        os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND
    }

    displayGenericContextPage(0, true);
}

// function to factorize code for all streaming reviews
static void useCaseReviewStreamingStart(nbgl_operationType_t       operationType,
                                        const nbgl_icon_details_t *icon,
                                        const char                *reviewTitle,
                                        const char                *reviewSubTitle,
                                        nbgl_choiceCallback_t      choiceCallback,
                                        bool                       playNotifSound)
{
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
    STARTING_CONTENT.type = EXTENDED_CENTER;
    prepareReviewFirstPage(
        &STARTING_CONTENT.content.extendedCenter.contentCenter, icon, reviewTitle, reviewSubTitle);

    // Prepare un tipbox if needed
    initWarningTipBox(NULL);

    // compute number of pages & fill navigation structure
    bundleNavContext.reviewStreaming.stepPageNb = getNbPagesForGenericContents(
        &genericContext.genericContents, 0, (operationType & SKIPPABLE_OPERATION));
    prepareNavInfo(true, NBGL_NO_PROGRESS_INDICATOR, getRejectReviewText(operationType));
    // no back button on first page
    navInfo.navWithButtons.backButton = false;

    // Play notification sound if required
    if (playNotifSound) {
#ifdef HAVE_PIEZO_SOUND
        os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND
    }

    displayGenericContextPage(0, true);
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
 * @param homeAction if not NULL, structure used for an action button (on top of "Quit
 * App" button/footer)
 * @param topRightCallback callback called when top-right button is touched
 * @param quitCallback callback called when quit button is touched
 */
static void useCaseHomeExt(const char                *appName,
                           const nbgl_icon_details_t *appIcon,
                           const char                *tagline,
                           bool                       withSettings,
                           nbgl_homeAction_t         *homeAction,
                           nbgl_callback_t            topRightCallback,
                           nbgl_callback_t            quitCallback)
{
    nbgl_pageInfoDescription_t info = {.centeredInfo.icon    = appIcon,
                                       .centeredInfo.text1   = appName,
                                       .centeredInfo.text3   = NULL,
                                       .centeredInfo.style   = LARGE_CASE_INFO,
                                       .centeredInfo.offsetY = 0,
                                       .footerText           = NULL,
                                       .bottomButtonStyle    = QUIT_APP_TEXT,
                                       .tapActionText        = NULL,
                                       .topRightStyle = withSettings ? SETTINGS_ICON : INFO_ICON,
                                       .topRightToken = CONTINUE_TOKEN,
                                       .tuneId        = TUNE_TAP_CASUAL};
    reset_callbacks_and_context();

    if ((homeAction->text != NULL) || (homeAction->icon != NULL)) {
        // trick to use ACTION_BUTTON_TOKEN for action and quit, with index used to distinguish
        info.bottomButtonsToken = ACTION_BUTTON_TOKEN;
        onAction                = homeAction->callback;
        info.actionButtonText   = homeAction->text;
        info.actionButtonIcon   = homeAction->icon;
        info.actionButtonStyle
            = (homeAction->style == STRONG_HOME_ACTION) ? BLACK_BACKGROUND : WHITE_BACKGROUND;
    }
    else {
        info.bottomButtonsToken = QUIT_TOKEN;
        onAction                = NULL;
        info.actionButtonText   = NULL;
        info.actionButtonIcon   = NULL;
    }
    if (tagline == NULL) {
        if (strlen(appName) > MAX_APP_NAME_FOR_SDK_TAGLINE) {
            snprintf(tmpString,
                     APP_DESCRIPTION_MAX_LEN,
                     "This app enables signing\ntransactions on its network.");
        }
        else {
            snprintf(tmpString,
                     APP_DESCRIPTION_MAX_LEN,
                     "%s %s\n%s",
                     TAGLINE_PART1,
                     appName,
                     TAGLINE_PART2);
        }

        // If there is more than 3 lines, it means the appName was split, so we put it on the next
        // line
        if (nbgl_getTextNbLinesInWidth(SMALL_REGULAR_FONT, tmpString, AVAILABLE_WIDTH, false) > 3) {
            snprintf(tmpString,
                     APP_DESCRIPTION_MAX_LEN,
                     "%s\n%s %s",
                     TAGLINE_PART1,
                     appName,
                     TAGLINE_PART2);
        }
        info.centeredInfo.text2 = tmpString;
    }
    else {
        info.centeredInfo.text2 = tagline;
    }

    onContinue = topRightCallback;
    onQuit     = quitCallback;

    pageContext = nbgl_pageDrawInfo(&pageCallback, NULL, &info);
    nbgl_refreshSpecial(FULL_COLOR_CLEAN_REFRESH);
}

/**
 * @brief Draws a flow of pages to view details on a given tag/value pair that doesn't fit in a
 * single page
 *
 * @param tag tag name (in gray)
 * @param value full value string, that will be split in multiple pages
 * @param wrapping if set to true, value text is wrapped on ' ' characters
 */
static void displayDetails(const char *tag, const char *value, bool wrapping)
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

// function used to display the modal containing alias tag-value pairs
static void displayTagValueListModal(const nbgl_contentTagValueList_t *tagValues)
{
    uint8_t nbElements = 0;
    uint8_t nbElementsInPage;
    uint8_t elemIdx = 0;

    // initialize context
    memset(&detailsContext, 0, sizeof(detailsContext));
    nbElements = tagValues->nbPairs;

    while (nbElements > 0) {
        nbElementsInPage = getNbTagValuesInDetailsPage(nbElements, tagValues, elemIdx);

        elemIdx += nbElementsInPage;
        modalContextSetPageInfo(detailsContext.nbPages, nbElementsInPage);
        nbElements -= nbElementsInPage;
        detailsContext.nbPages++;
    }

    displayTagValueListModalPage(0, true);
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
    return getNbTagValuesInPage(
        nbPairs, tagValueList, startIndex, false, false, false, requireSpecificDisplay);
}

/**
 * @brief computes the number of tag/values pairs displayable in a page, with the given list of
 * tag/value pairs
 *
 * @param nbPairs number of tag/value pairs to use in \b tagValueList
 * @param tagValueList list of tag/value pairs
 * @param startIndex first index to consider in \b tagValueList
 * @param isSkippable if true, a skip header is added
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
    return getNbTagValuesInPage(
        nbPairs, tagValueList, startIndex, isSkippable, false, false, requireSpecificDisplay);
}

/**
 * @brief computes the number of infos displayable in a page, with the given list of
 * infos
 *
 * @param nbInfos number of infos to use in \b infosList
 * @param infosList list of infos
 * @param startIndex first index to consider in \b infosList
 * @return the number of infos fitting in a page
 */
uint8_t nbgl_useCaseGetNbInfosInPage(uint8_t                       nbInfos,
                                     const nbgl_contentInfoList_t *infosList,
                                     uint8_t                       startIndex,
                                     bool                          withNav)
{
    uint8_t            nbInfosInPage = 0;
    uint16_t           currentHeight = 0;
    uint16_t           previousHeight;
    uint16_t           navHeight    = withNav ? SIMPLE_FOOTER_HEIGHT : 0;
    const char *const *infoContents = PIC(infosList->infoContents);

    while (nbInfosInPage < nbInfos) {
        // The type string must be a 1 liner and its height is LIST_ITEM_MIN_TEXT_HEIGHT
        currentHeight
            += LIST_ITEM_MIN_TEXT_HEIGHT + 2 * LIST_ITEM_PRE_HEADING + LIST_ITEM_HEADING_SUB_TEXT;

        // content height
        currentHeight += nbgl_getTextHeightInWidth(SMALL_REGULAR_FONT,
                                                   PIC(infoContents[startIndex + nbInfosInPage]),
                                                   AVAILABLE_WIDTH,
                                                   true);
        // if height is over the limit
        if (currentHeight >= (INFOS_AREA_HEIGHT - navHeight)) {
            // if there was no nav, now there will be, so it can be necessary to remove the last
            // item
            if (!withNav && (previousHeight >= (INFOS_AREA_HEIGHT - SIMPLE_FOOTER_HEIGHT))) {
                nbInfosInPage--;
            }
            break;
        }
        previousHeight = currentHeight;
        nbInfosInPage++;
    }
    return nbInfosInPage;
}

/**
 * @brief computes the number of switches displayable in a page, with the given list of
 * switches
 *
 * @param nbSwitches number of switches to use in \b switchesList
 * @param switchesList list of switches
 * @param startIndex first index to consider in \b switchesList
 * @return the number of switches fitting in a page
 */
uint8_t nbgl_useCaseGetNbSwitchesInPage(uint8_t                           nbSwitches,
                                        const nbgl_contentSwitchesList_t *switchesList,
                                        uint8_t                           startIndex,
                                        bool                              withNav)
{
    uint8_t               nbSwitchesInPage = 0;
    uint16_t              currentHeight    = 0;
    uint16_t              previousHeight   = 0;
    uint16_t              navHeight        = withNav ? SIMPLE_FOOTER_HEIGHT : 0;
    nbgl_contentSwitch_t *switchArray      = (nbgl_contentSwitch_t *) PIC(switchesList->switches);

    while (nbSwitchesInPage < nbSwitches) {
        nbgl_contentSwitch_t *curSwitch = &switchArray[startIndex + nbSwitchesInPage];
        // The text string is either a 1 liner and its height is LIST_ITEM_MIN_TEXT_HEIGHT
        // or we use its height directly
        uint16_t textHeight = MAX(
            LIST_ITEM_MIN_TEXT_HEIGHT,
            nbgl_getTextHeightInWidth(SMALL_BOLD_FONT, curSwitch->text, AVAILABLE_WIDTH, true));
        currentHeight += textHeight + 2 * LIST_ITEM_PRE_HEADING;

        if (curSwitch->subText) {
            currentHeight += LIST_ITEM_HEADING_SUB_TEXT;

            // sub-text height
            currentHeight += nbgl_getTextHeightInWidth(
                SMALL_REGULAR_FONT, curSwitch->subText, AVAILABLE_WIDTH, true);
        }
        // if height is over the limit
        if (currentHeight >= (INFOS_AREA_HEIGHT - navHeight)) {
            break;
        }
        previousHeight = currentHeight;
        nbSwitchesInPage++;
    }
    // if there was no nav, now there will be, so it can be necessary to remove the last
    // item
    if (!withNav && (previousHeight >= (INFOS_AREA_HEIGHT - SIMPLE_FOOTER_HEIGHT))) {
        nbSwitchesInPage--;
    }
    return nbSwitchesInPage;
}

/**
 * @brief computes the number of bars displayable in a page, with the given list of
 * bars
 *
 * @param nbBars number of bars to use in \b barsList
 * @param barsList list of bars
 * @param startIndex first index to consider in \b barsList
 * @return the number of bars fitting in a page
 */
uint8_t nbgl_useCaseGetNbBarsInPage(uint8_t                       nbBars,
                                    const nbgl_contentBarsList_t *barsList,
                                    uint8_t                       startIndex,
                                    bool                          withNav)
{
    uint8_t  nbBarsInPage  = 0;
    uint16_t currentHeight = 0;
    uint16_t previousHeight;
    uint16_t navHeight = withNav ? SIMPLE_FOOTER_HEIGHT : 0;

    UNUSED(barsList);
    UNUSED(startIndex);

    while (nbBarsInPage < nbBars) {
        currentHeight += LIST_ITEM_MIN_TEXT_HEIGHT + 2 * LIST_ITEM_PRE_HEADING;
        // if height is over the limit
        if (currentHeight >= (INFOS_AREA_HEIGHT - navHeight)) {
            break;
        }
        previousHeight = currentHeight;
        nbBarsInPage++;
    }
    // if there was no nav, now there may will be, so it can be necessary to remove the last
    // item
    if (!withNav && (previousHeight >= (INFOS_AREA_HEIGHT - SIMPLE_FOOTER_HEIGHT))) {
        nbBarsInPage--;
    }
    return nbBarsInPage;
}

/**
 * @brief computes the number of radio choices displayable in a page, with the given list of
 * choices
 *
 * @param nbChoices number of radio choices to use in \b choicesList
 * @param choicesList list of choices
 * @param startIndex first index to consider in \b choicesList
 * @return the number of radio choices fitting in a page
 */
uint8_t nbgl_useCaseGetNbChoicesInPage(uint8_t                          nbChoices,
                                       const nbgl_contentRadioChoice_t *choicesList,
                                       uint8_t                          startIndex,
                                       bool                             withNav)
{
    uint8_t  nbChoicesInPage = 0;
    uint16_t currentHeight   = 0;
    uint16_t previousHeight;
    uint16_t navHeight = withNav ? SIMPLE_FOOTER_HEIGHT : 0;

    UNUSED(choicesList);
    UNUSED(startIndex);

    while (nbChoicesInPage < nbChoices) {
        currentHeight += LIST_ITEM_MIN_TEXT_HEIGHT + 2 * LIST_ITEM_PRE_HEADING;
        // if height is over the limit
        if (currentHeight >= (INFOS_AREA_HEIGHT - navHeight)) {
            // if there was no nav, now there will be, so it can be necessary to remove the last
            // item
            if (!withNav && (previousHeight >= (INFOS_AREA_HEIGHT - SIMPLE_FOOTER_HEIGHT))) {
                nbChoicesInPage--;
            }
            break;
        }
        previousHeight = currentHeight;
        nbChoicesInPage++;
    }
    return nbChoicesInPage;
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
 * @deprecated
 * See #nbgl_useCaseHomeAndSettings
 */
void nbgl_useCaseHome(const char                *appName,
                      const nbgl_icon_details_t *appIcon,
                      const char                *tagline,
                      bool                       withSettings,
                      nbgl_callback_t            topRightCallback,
                      nbgl_callback_t            quitCallback)
{
    nbgl_homeAction_t homeAction = {0};
    useCaseHomeExt(
        appName, appIcon, tagline, withSettings, &homeAction, topRightCallback, quitCallback);
}

/**
 * @deprecated
 * See #nbgl_useCaseHomeAndSettings
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
    nbgl_homeAction_t homeAction = {.callback = actionCallback,
                                    .icon     = NULL,
                                    .style    = STRONG_HOME_ACTION,
                                    .text     = actionButtonText};

    useCaseHomeExt(
        appName, appIcon, tagline, withSettings, &homeAction, topRightCallback, quitCallback);
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
    reset_callbacks_and_context();

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
 * @deprecated
 * See #nbgl_useCaseHomeAndSettings if used in a 'Settings' context, or nbgl_useCaseNavigableContent
 * otherwise
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
    nbgl_useCaseNavigableContent(
        title, initPage, nbPages, quitCallback, navCallback, controlsCallback);
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
    reset_callbacks_and_context();

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
    uint8_t nbPages = getNbPagesForGenericContents(&genericContext.genericContents, 0, false);
    if (infosList != NULL) {
        nbPages += getNbPagesForContent(&FINISHING_CONTENT, nbPages, true, false);
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

    reset_callbacks_and_context();

    context->appName         = appName;
    context->appIcon         = appIcon;
    context->tagline         = tagline;
    context->settingContents = settingContents;
    context->infosList       = infosList;
    if (action != NULL) {
        memcpy(&context->homeAction, action, sizeof(nbgl_homeAction_t));
    }
    else {
        memset(&context->homeAction, 0, sizeof(nbgl_homeAction_t));
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
    nbgl_screenTickerConfiguration_t ticker = {.tickerCallback  = &tickerCallback,
                                               .tickerIntervale = 0,  // not periodic
                                               .tickerValue     = STATUS_SCREEN_DURATION};
    nbgl_pageInfoDescription_t       info   = {0};

    reset_callbacks_and_context();

    onQuit = quitCallback;
    if (isSuccess) {
#ifdef HAVE_PIEZO_SOUND
        os_io_seph_cmd_piezo_play_tune(TUNE_LEDGER_MOMENT);
#endif  // HAVE_PIEZO_SOUND
    }
    info.centeredInfo.icon  = isSuccess ? &CHECK_CIRCLE_ICON : &DENIED_CIRCLE_ICON;
    info.centeredInfo.style = LARGE_CASE_INFO;
    info.centeredInfo.text1 = message;
    info.tapActionText      = "";
    info.tapActionToken     = QUIT_TOKEN;
    info.tuneId             = TUNE_TAP_CASUAL;
    pageContext             = nbgl_pageDrawInfo(&pageCallback, &ticker, &info);
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
    nbgl_pageConfirmationDescription_t info = {0};
    // check params
    if ((confirmText == NULL) || (cancelText == NULL)) {
        return;
    }
    reset_callbacks_and_context();

    info.cancelText         = cancelText;
    info.centeredInfo.text1 = message;
    info.centeredInfo.text2 = subMessage;
    info.centeredInfo.style = LARGE_CASE_INFO;
    info.centeredInfo.icon  = icon;
    info.confirmationText   = confirmText;
    info.confirmationToken  = CHOICE_TOKEN;
    info.tuneId             = TUNE_TAP_CASUAL;

    onChoice    = callback;
    pageContext = nbgl_pageDrawConfirmation(&pageCallback, &info);
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
}

/**
 * @brief Draws a generic choice page, described in a centered info (with configurable icon), thanks
 * to a button and a footer at the bottom of the page. The given callback is called with true as
 * argument if the button is touched, false if footer is touched
 * Compared to @ref nbgl_useCaseChoice, it also adds a top-right button to access details
 *
 * @param icon icon to set in center of page
 * @param message string to set in center of page (32px)
 * @param subMessage string to set under message (24px) (can be NULL)
 * @param confirmText string to set in button, to confirm (cannot be NULL)
 * @param cancelText string to set in footer, to reject (cannot be NULL)
 * @param details details to be displayed when pressing top-right button
 * @param callback callback called when button or footer is touched
 */
void nbgl_useCaseChoiceWithDetails(const nbgl_icon_details_t *icon,
                                   const char                *message,
                                   const char                *subMessage,
                                   const char                *confirmText,
                                   const char                *cancelText,
                                   nbgl_genericDetails_t     *details,
                                   nbgl_choiceCallback_t      callback)
{
    nbgl_layoutDescription_t   layoutDescription;
    nbgl_layoutChoiceButtons_t buttonsInfo  = {.bottomText = cancelText,
                                               .token      = CHOICE_TOKEN,
                                               .topText    = confirmText,
                                               .style      = ROUNDED_AND_FOOTER_STYLE,
                                               .tuneId     = TUNE_TAP_CASUAL};
    nbgl_contentCenter_t       centeredInfo = {0};
    nbgl_layoutHeader_t        headerDesc   = {.type              = HEADER_EMPTY,
                                               .separationLine    = false,
                                               .emptySpace.height = MEDIUM_CENTERING_HEADER};

    // check params
    if ((confirmText == NULL) || (cancelText == NULL)) {
        return;
    }

    reset_callbacks_and_context();

    onChoice                         = callback;
    layoutDescription.modal          = false;
    layoutDescription.withLeftBorder = true;

    layoutDescription.onActionCallback = layoutTouchCallback;
    layoutDescription.tapActionText    = NULL;

    layoutDescription.ticker.tickerCallback = NULL;
    sharedContext.usage                     = SHARE_CTX_CHOICE_WITH_DETAILS;
    choiceWithDetailsCtx.layoutCtx          = nbgl_layoutGet(&layoutDescription);
    choiceWithDetailsCtx.details            = details;

    nbgl_layoutAddHeader(choiceWithDetailsCtx.layoutCtx, &headerDesc);
    nbgl_layoutAddChoiceButtons(choiceWithDetailsCtx.layoutCtx, &buttonsInfo);
    centeredInfo.icon        = icon;
    centeredInfo.title       = message;
    centeredInfo.description = subMessage;
    nbgl_layoutAddContentCenter(choiceWithDetailsCtx.layoutCtx, &centeredInfo);

    if (details != NULL) {
        nbgl_layoutAddTopRightButton(
            choiceWithDetailsCtx.layoutCtx, &SEARCH_ICON, CHOICE_DETAILS_TOKEN, TUNE_TAP_CASUAL);
    }

    nbgl_layoutDraw(choiceWithDetailsCtx.layoutCtx);
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
                                               .centeredInfo.icon    = &IMPORTANT_CIRCLE_ICON,
                                               .centeredInfo.offsetY = 0,
                                               .confirmationText     = confirmText,
                                               .confirmationToken    = CHOICE_TOKEN,
                                               .tuneId               = TUNE_TAP_CASUAL,
                                               .modal                = true};
    onModalConfirm                          = callback;
    if (modalPageContext != NULL) {
        nbgl_pageRelease(modalPageContext);
    }
    modalPageContext = nbgl_pageDrawConfirmation(&pageModalCallback, &info);
    nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
}

/**
 * @brief Draws a page to represent an action, described in a centered info (with given icon),
 * thanks to a button at the bottom of the page. The given callback is called if the
 * button is touched.
 *
 * @param icon icon to set in centered info
 * @param message string to set in centered info
 * @param actionText string to set in button, for action
 * @param callback callback called when action button is touched
 */
void nbgl_useCaseAction(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *actionText,
                        nbgl_callback_t            callback)
{
    nbgl_pageContent_t content = {0};

    reset_callbacks_and_context();
    // memorize callback
    onAction = callback;

    content.tuneId                 = TUNE_TAP_CASUAL;
    content.type                   = INFO_BUTTON;
    content.infoButton.buttonText  = actionText;
    content.infoButton.text        = message;
    content.infoButton.icon        = icon;
    content.infoButton.buttonToken = ACTION_BUTTON_TOKEN;

    pageContext = nbgl_pageDrawGenericContent(&pageCallback, NULL, &content);
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
    nbgl_pageInfoDescription_t info = {.footerText       = rejectText,
                                       .footerToken      = QUIT_TOKEN,
                                       .tapActionText    = NULL,
                                       .isSwipeable      = true,
                                       .tapActionToken   = CONTINUE_TOKEN,
                                       .topRightStyle    = NO_BUTTON_STYLE,
                                       .actionButtonText = NULL,
                                       .tuneId           = TUNE_TAP_CASUAL};

    reset_callbacks_and_context();

    info.centeredInfo.icon    = icon;
    info.centeredInfo.text1   = reviewTitle;
    info.centeredInfo.text2   = reviewSubTitle;
    info.centeredInfo.text3   = "Swipe to review";
    info.centeredInfo.style   = LARGE_CASE_GRAY_INFO;
    info.centeredInfo.offsetY = 0;
    onQuit                    = rejectCallback;
    onContinue                = continueCallback;

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    pageContext = nbgl_pageDrawInfo(&pageCallback, NULL, &info);
    nbgl_refresh();
}

/**
 * @deprecated
 * See #nbgl_useCaseReview
 */
void nbgl_useCaseRegularReview(uint8_t                    initPage,
                               uint8_t                    nbPages,
                               const char                *rejectText,
                               nbgl_layoutTouchCallback_t buttonCallback,
                               nbgl_navCallback_t         navCallback,
                               nbgl_choiceCallback_t      choiceCallback)
{
    reset_callbacks_and_context();

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

    reset_callbacks_and_context();

    // memorize context
    onChoice                              = callback;
    navType                               = GENERIC_NAV;
    pageTitle                             = NULL;
    bundleNavContext.review.operationType = TYPE_OPERATION;

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
    uint8_t nbPages = getNbPagesForGenericContents(&genericContext.genericContents, 0, false);
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

    reset_callbacks_and_context();

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
    uint8_t nbPages = getNbPagesForGenericContents(&genericContext.genericContents, 0, false);
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
    reset_callbacks_and_context();

    useCaseReview(operationType,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  NULL,
                  choiceCallback,
                  false,
                  true);
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
 * @param tipBox parameter to build a tip-box and necessary modal (can be NULL)
 * @param choiceCallback callback called when operation is accepted (param is true) or rejected
 * (param is false)
 */
void nbgl_useCaseReviewBlindSigning(nbgl_operationType_t              operationType,
                                    const nbgl_contentTagValueList_t *tagValueList,
                                    const nbgl_icon_details_t        *icon,
                                    const char                       *reviewTitle,
                                    const char                       *reviewSubTitle,
                                    const char                       *finishTitle,
                                    const nbgl_tipBox_t              *tipBox,
                                    nbgl_choiceCallback_t             choiceCallback)
{
    nbgl_useCaseAdvancedReview(operationType,
                               tagValueList,
                               icon,
                               reviewTitle,
                               reviewSubTitle,
                               finishTitle,
                               tipBox,
                               &blindSigningWarning,
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
    reset_callbacks_and_context();

    // memorize tipBox because it can be in the call stack of the caller
    if (tipBox != NULL) {
        memcpy(&activeTipBox, tipBox, sizeof(activeTipBox));
    }
    // if no warning at all, it's a simple review
    if ((warning == NULL)
        || ((warning->predefinedSet == 0) && (warning->introDetails == NULL)
            && (warning->reviewDetails == NULL) && (warning->prelude == NULL))) {
        useCaseReview(operationType,
                      tagValueList,
                      icon,
                      reviewTitle,
                      reviewSubTitle,
                      finishTitle,
                      tipBox,
                      choiceCallback,
                      false,
                      true);
        return;
    }
    if (warning->predefinedSet == (1 << W3C_NO_THREAT_WARN)) {
        operationType |= NO_THREAT_OPERATION;
    }
    else if (warning->predefinedSet != 0) {
        operationType |= RISKY_OPERATION;
    }
    sharedContext.usage              = SHARE_CTX_REVIEW_WITH_WARNING;
    reviewWithWarnCtx.isStreaming    = false;
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
        useCaseReview(operationType,
                      tagValueList,
                      icon,
                      reviewTitle,
                      reviewSubTitle,
                      finishTitle,
                      tipBox,
                      choiceCallback,
                      false,
                      true);
    }
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
    reset_callbacks_and_context();

    useCaseReview(operationType,
                  tagValueList,
                  icon,
                  reviewTitle,
                  reviewSubTitle,
                  finishTitle,
                  NULL,
                  choiceCallback,
                  true,
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
    reset_callbacks_and_context();

    // memorize context
    onQuit                                = rejectCallback;
    navType                               = GENERIC_NAV;
    pageTitle                             = NULL;
    bundleNavContext.review.operationType = TYPE_OPERATION;

    memcpy(&genericContext.genericContents, contents, sizeof(nbgl_genericContents_t));

    // compute number of pages & fill navigation structure
    uint8_t nbPages = getNbPagesForGenericContents(&genericContext.genericContents, 0, false);
    prepareNavInfo(true, nbPages, rejectText);
    navInfo.quitToken = QUIT_TOKEN;

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
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
    reset_callbacks_and_context();

    useCaseReviewStreamingStart(
        operationType, icon, reviewTitle, reviewSubTitle, choiceCallback, true);
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
    reset_callbacks_and_context();

    // if no warning at all, it's a simple review
    if ((warning == NULL)
        || ((warning->predefinedSet == 0) && (warning->introDetails == NULL)
            && (warning->reviewDetails == NULL) && (warning->prelude == NULL))) {
        useCaseReviewStreamingStart(
            operationType, icon, reviewTitle, reviewSubTitle, choiceCallback, true);
        return;
    }
    if (warning->predefinedSet == (1 << W3C_NO_THREAT_WARN)) {
        operationType |= NO_THREAT_OPERATION;
    }
    else if (warning->predefinedSet != 0) {
        operationType |= RISKY_OPERATION;
    }

    sharedContext.usage              = SHARE_CTX_REVIEW_WITH_WARNING;
    reviewWithWarnCtx.isStreaming    = true;
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
    // display the initial warning only of a risk/threat or blind signing
    else if (HAS_INITIAL_WARNING(warning)) {
        displayInitialWarning();
    }
    else {
        useCaseReviewStreamingStart(
            operationType, icon, reviewTitle, reviewSubTitle, choiceCallback, true);
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
    bundleNavContext.reviewStreaming.stepPageNb = getNbPagesForGenericContents(
        &genericContext.genericContents,
        0,
        (bundleNavContext.reviewStreaming.operationType & SKIPPABLE_OPERATION));
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
    prepareReviewLastPage(bundleNavContext.reviewStreaming.operationType,
                          &STARTING_CONTENT.content.infoLongPress,
                          bundleNavContext.reviewStreaming.icon,
                          finishTitle);

    // compute number of pages & fill navigation structure
    bundleNavContext.reviewStreaming.stepPageNb = getNbPagesForGenericContents(
        &genericContext.genericContents,
        0,
        (bundleNavContext.reviewStreaming.operationType & SKIPPABLE_OPERATION));
    prepareNavInfo(true, 1, getRejectReviewText(bundleNavContext.reviewStreaming.operationType));

    displayGenericContextPage(0, true);
}

/**
 * @deprecated
 * See #nbgl_useCaseAddressReview
 */
void nbgl_useCaseAddressConfirmationExt(const char                       *address,
                                        nbgl_choiceCallback_t             callback,
                                        const nbgl_contentTagValueList_t *tagValueList)
{
    reset_callbacks_and_context();
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
    uint8_t nbPages = getNbPagesForGenericContents(&genericContext.genericContents, 0, false);

    prepareNavInfo(true, nbPages, "Cancel");

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
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
    reset_callbacks_and_context();

    // release a potential modal
    if (addressConfirmationContext.modalLayout) {
        nbgl_layoutRelease(addressConfirmationContext.modalLayout);
    }
    memset(&addressConfirmationContext, 0, sizeof(addressConfirmationContext));

    // save context
    onChoice                              = choiceCallback;
    navType                               = GENERIC_NAV;
    pageTitle                             = NULL;
    bundleNavContext.review.operationType = TYPE_OPERATION;

    genericContext.genericContents.contentsList = localContentsList;
    memset(localContentsList, 0, 3 * sizeof(nbgl_content_t));

    // First a centered info
    STARTING_CONTENT.type = EXTENDED_CENTER;
    prepareReviewFirstPage(
        &STARTING_CONTENT.content.extendedCenter.contentCenter, icon, reviewTitle, reviewSubTitle);
    STARTING_CONTENT.content.extendedCenter.contentCenter.subText = "Swipe to continue";

    // Then the address confirmation pages
    prepareAddressConfirmationPages(
        address, additionalTagValueList, &localContentsList[1], &localContentsList[2]);

    // fill navigation structure, common to all pages
    genericContext.genericContents.nbContents
        = (localContentsList[2].type == TAG_VALUE_CONFIRM) ? 3 : 2;
    uint8_t nbPages = getNbPagesForGenericContents(&genericContext.genericContents, 0, false);

    prepareNavInfo(true, nbPages, "Cancel");

#ifdef HAVE_PIEZO_SOUND
    // Play notification sound
    os_io_seph_cmd_piezo_play_tune(TUNE_LOOK_AT_ME);
#endif  // HAVE_PIEZO_SOUND

    displayGenericContextPage(0, true);
}

/**
 * @brief draw a spinner page with the given parameters. The spinner will "turn" automatically every
 * 800 ms
 * @note If called several time in a raw, it can be used to "turn" the corner, for example when
 * used in a long mono-process, preventing the automatic update.
 *
 * @param text text to use under spinner
 */
void nbgl_useCaseSpinner(const char *text)
{
    // if the previous Use Case was not Spinner, fresh start
    if (genericContext.type != USE_CASE_SPINNER) {
        memset(&genericContext, 0, sizeof(genericContext));
        genericContext.type                        = USE_CASE_SPINNER;
        nbgl_layoutDescription_t layoutDescription = {0};

        layoutDescription.withLeftBorder = true;

        genericContext.backgroundLayout = nbgl_layoutGet(&layoutDescription);

        nbgl_layoutAddSpinner(
            genericContext.backgroundLayout, text, NULL, genericContext.spinnerPosition);

        nbgl_layoutDraw(genericContext.backgroundLayout);
        nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
    }
    else {
        // otherwise increment spinner
        genericContext.spinnerPosition++;
        // there are only NB_SPINNER_POSITIONSpositions
        if (genericContext.spinnerPosition == NB_SPINNER_POSITIONS) {
            genericContext.spinnerPosition = 0;
        }
        int ret = nbgl_layoutUpdateSpinner(
            genericContext.backgroundLayout, text, NULL, genericContext.spinnerPosition);
        if (ret == 1) {
            nbgl_refreshSpecial(BLACK_AND_WHITE_FAST_REFRESH);
        }
        else if (ret == 2) {
            nbgl_refreshSpecial(FULL_COLOR_PARTIAL_REFRESH);
        }
    }
}

#ifdef NBGL_KEYPAD
/**
 * @brief draws a standard keypad modal page. It contains
 *        - a navigation bar at the top
 *        - a title for the pin code
 *        - a digit entry (visible or hidden)
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
 * @param backCallback callback called title is pressed
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
    nbgl_layoutHeader_t      headerDesc        = {.type               = HEADER_BACK_AND_TEXT,
                                                  .separationLine     = true,
                                                  .backAndText.token  = BACK_TOKEN,
                                                  .backAndText.tuneId = TUNE_TAP_CASUAL,
                                                  .backAndText.text   = NULL};
    int                      status            = -1;

    if ((minDigits > KEYPAD_MAX_DIGITS) || (maxDigits > KEYPAD_MAX_DIGITS)) {
        return;
    }

    reset_callbacks_and_context();
    // reset the keypad context
    memset(&keypadContext, 0, sizeof(KeypadContext_t));

    // memorize context
    onQuit = backCallback;

    // get a layout
    layoutDescription.onActionCallback = keypadGeneric_cb;
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
#endif  // NBGL_KEYPAD

#endif  // HAVE_SE_TOUCH
#endif  // NBGL_USE_CASE
