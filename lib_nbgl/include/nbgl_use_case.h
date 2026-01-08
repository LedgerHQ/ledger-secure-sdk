/**
 * @file nbgl_use_case.h
 * @brief API of the Advanced BOLOS Graphical Library, for typical application use-cases
 *
 */

#ifndef NBGL_USE_CASE_H
#define NBGL_USE_CASE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "nbgl_content.h"
#ifdef NBGL_PAGE
#include "nbgl_page.h"
#else  // NBGL_PAGE
#include "nbgl_flow.h"
#endif  // NBGL_PAGE

/*********************
 *      DEFINES
 *********************/
/**
 *  @brief when using controls in page content (@ref nbgl_pageContent_t), this is the first token
 * value usable for these controls
 */
#define FIRST_USER_TOKEN 20

/**
 *  @brief value of page parameter used with navigation callback when "skip" button is touched, to
 * display the long press button to confirm review.
 */
#define LAST_PAGE_FOR_REVIEW 0xFF

/**
 *  @brief maximum number of lines for value field in details pages
 */
#if defined(TARGET_STAX)
#define NB_MAX_LINES_IN_DETAILS 12
#elif defined(TARGET_FLEX)
#define NB_MAX_LINES_IN_DETAILS 11
#elif defined(TARGET_APEX)
#define NB_MAX_LINES_IN_DETAILS 11
#else
// Nano
#define NB_MAX_LINES_IN_DETAILS 3
#endif  // TARGETS

/**
 *  @brief maximum number of lines for value field in review pages
 */
#if defined(TARGET_STAX)
#define NB_MAX_LINES_IN_REVIEW 10
#elif defined(TARGET_FLEX)
#define NB_MAX_LINES_IN_REVIEW 9
#elif defined(TARGET_APEX)
#define NB_MAX_LINES_IN_REVIEW 9
#else
// Nano
#define NB_MAX_LINES_IN_REVIEW 3
#endif  // TARGETS

/**
 *  @brief maximum number of simultaneously displayed pairs in review pages.
 *         Can be useful when using nbgl_useCaseStaticReview() with the
 *         callback mechanism to retrieve the item/value pairs.
 */
#define NB_MAX_DISPLAYED_PAIRS_IN_REVIEW 4

/**
 *  @brief height available for tag/value pairs display
 */
#define TAG_VALUE_AREA_HEIGHT (SCREEN_HEIGHT - SMALL_CENTERING_HEADER - SIMPLE_FOOTER_HEIGHT)

/**
 *  @brief height available for infos pairs display
 */
#define INFOS_AREA_HEIGHT (SCREEN_HEIGHT - TOUCHABLE_HEADER_BAR_HEIGHT)

/**
 *  @brief Default strings used in the Home tagline
 */
#define TAGLINE_PART1 "This app enables signing\ntransactions on the"
#define TAGLINE_PART2 "network."

/**
 *  @brief Length of buffer used for the default Home tagline
 */
#define APP_DESCRIPTION_MAX_LEN 74

/**
 *  @brief Max supported length of appName used for the default Home tagline
 */
#define MAX_APP_NAME_FOR_SDK_TAGLINE \
    (APP_DESCRIPTION_MAX_LEN - 1 - (sizeof(TAGLINE_PART1) + sizeof(TAGLINE_PART2)))

/**
 *  @brief Value to pass to nbgl_useCaseHomeAndSettings() initSettingPage parameter
 *         to initialize the use case on the Home page and not on a specific setting
 *          page.
 */
#define INIT_HOME_PAGE 0xff

///< Duration of status screens, automatically closing after this timeout (3s)
#define STATUS_SCREEN_DURATION 3000

/**
 * @brief This is the mask to apply on @ref nbgl_operationType_t to get the real type provided by
 * app
 *
 */
#define REAL_TYPE_MASK 0x7

// returns true if the given warning structure requires display of an initial page (internal usage)
#define HAS_INITIAL_WARNING(_warning)                                    \
    ((_warning->predefinedSet                                            \
      & ((1 << W3C_THREAT_DETECTED_WARN) | (1 << W3C_RISK_DETECTED_WARN) \
         | (1 << BLIND_SIGNING_WARN)))                                   \
     || (_warning->introDetails != NULL))

/**********************
 *      MACROS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief prototype of generic callback function
 */
typedef void (*nbgl_callback_t)(void);

/**
 * @brief prototype of navigation callback function
 * @param page page index (0->(nb_pages-1)) on which we go
 * @param content content to fill (only type and union)
 * @return true if the page content is valid, false if no more page
 */
typedef bool (*nbgl_navCallback_t)(uint8_t page, nbgl_pageContent_t *content);

/**
 * @brief prototype of choice callback function
 * @param confirm if true, means that the confirmation button has been pressed
 */
typedef void (*nbgl_choiceCallback_t)(bool confirm);

/**
 * @brief prototype of function to be called when an page of settings is double-pressed
 * @param page page index (0->(nb_pages-1))
 */
typedef void (*nbgl_actionCallback_t)(uint8_t page);

/**
 * @brief prototype of pin validation callback function
 * @param pin pin value
 * @param pinLen pin length
 */
typedef void (*nbgl_pinValidCallback_t)(const uint8_t *pin, uint8_t pinLen);

/**
 * @brief prototype of content navigation callback function
 * @param contentIndex content index (0->(nbContents-1)) that is needed by the lib
 * @param content content to fill
 */
typedef void (*nbgl_contentCallback_t)(uint8_t contentIndex, nbgl_content_t *content);

typedef struct {
    bool callbackCallNeeded;  ///< indicates whether contents should be retrieved using
                              ///< contentsList or contentGetterCallback
    union {
        const nbgl_content_t *contentsList;  ///< array of nbgl_content_t (nbContents items).
        nbgl_contentCallback_t
            contentGetterCallback;  ///< function to call to retrieve a given content
    };
    uint8_t nbContents;  ///< number of contents
} nbgl_genericContents_t;

/**
 * @brief The different types of action button in Home Screen
 *
 */
typedef enum {
    STRONG_HOME_ACTION = 0,  ///< Black button, implicating the main action of the App
    SOFT_HOME_ACTION         ///< White button, more for extended features
} nbgl_homeActionStyle_t;

/**
 * @brief Structure describing the action button in Home Screen
 *
 */
typedef struct {
    const char                *text;  ///< text to use in action button in Home page
    const nbgl_icon_details_t *icon;  ///< icon to use in action button in Home page
    nbgl_callback_t callback;      ///< function to call when action button is touched in Home page
    nbgl_homeActionStyle_t style;  ///< style of action button
} nbgl_homeAction_t;

/**
 * @brief The necessary parameters to build a tip-box in first review page and
 * the modal if this tip box is touched
 *
 */
typedef struct {
    const char                *text;  ///< text of the tip-box
    const nbgl_icon_details_t *icon;  ///< icon of the tip-box
    const char *modalTitle;   ///< title given to modal window displayed when tip-box is touched
    nbgl_contentType_t type;  ///< type of page content in the following union
    union {
        nbgl_contentInfoList_t
            infos;  ///< infos pairs displayed in modal, if type is @ref INFOS_LIST.
    };
} nbgl_tipBox_t;

/**
 * @brief The different types of generic detailed page contents
 *
 */
typedef enum {
    NO_TYPE_WARNING = 0,    ///< Invalid type (to use for bars leading to nothing)
    CENTERED_INFO_WARNING,  ///< Centered info
    QRCODE_WARNING,         ///< QR Code
    BAR_LIST_WARNING        ///< list of touchable bars, to display sub-pages
} nbgl_genericDetailsType_t;

typedef nbgl_genericDetailsType_t nbgl_warningDetailsType_t;

/**
 * @brief The necessary parameters to build a list of touchable bars, to display sub-pages
 *
 */
typedef struct {
    uint8_t            nbBars;    ///< number of touchable bars
    const char *const *texts;     ///< array of texts for each bar (nbBars items, in black/bold)
    const char *const *subTexts;  ///< array of texts for each bar (nbBars items, in black)
    const nbgl_icon_details_t **icons;  ///< array of icons for each bar (nbBars items)
    const struct nbgl_genericDetails_s
        *details;  ///< array of nbBars structures giving what to display when each bar is touched.
} nbgl_genericBarList_t;

typedef nbgl_genericBarList_t nbgl_warningBarList_t;

/**
 * @brief The necessary parameters to build the page(s) displayed when the top-right button is
 * touched in intro page (before review), or in a Choice with details use-case
 *
 */
typedef struct nbgl_genericDetails_s {
    const char *title;  ///< text of the page (used to go back)
    nbgl_genericDetailsType_t
        type;  ///< type of content in the page, determining what to use in the following union
    union {
        nbgl_contentCenter_t
            centeredInfo;  ///< centered info, if type == @ref CENTERED_INFO_WARNING
#ifdef NBGL_QRCODE
        nbgl_layoutQRCode_t qrCode;     ///< QR code, if type == @ref QRCODE_WARNING
#endif                                  // NBGL_QRCODE
        nbgl_genericBarList_t barList;  ///< touchable bars list, if type == @ref BAR_LIST_WARNING
    };
} nbgl_genericDetails_t;

typedef nbgl_genericDetails_t nbgl_warningDetails_t;

/**
 * @brief The different types of pre-defined warnings
 *
 */
typedef enum {
    W3C_ISSUE_WARN = 0,        ///< Web3 Checks issue (not available)
    W3C_RISK_DETECTED_WARN,    ///< Web3 Checks: Risk detected (see reportRisk field)
    W3C_THREAT_DETECTED_WARN,  ///< Web3 Checks: Threat detected (see reportRisk field)
    W3C_NO_THREAT_WARN,        ///< Web3 Checks: No Threat detected
    BLIND_SIGNING_WARN,        ///< Blind signing
    GATED_SIGNING_WARN,        ///< Gated signing
    NB_WARNING_TYPES
} nbgl_warningType_t;

/**
 * @brief The necessary parameters to build the prelude of a review use-case
 * @note On Nano, only title field can be used
 */
typedef struct nbgl_preludeDetails_s {
    const nbgl_icon_details_t *icon;         ///< icon of the centered info
    const char                *title;        ///< title of the centered info
    const char                *description;  ///< sub-text of the centered info
    const char                *buttonText;   ///< text of the black button, used to open details
    const char                *footerText;   ///< text of the footer, used to continue to the review
    const nbgl_genericDetails_t *details;    ///< pointer to structure containing some details
} nbgl_preludeDetails_t;

/**
 * @brief The necessary parameters to build a warning page preceding a review.
 * One can either use `predefinedSet` when the warnings are already supported in @ref
 * nbgl_warningType_t list, or use `introDetails` or `reviewDetails` to configure manually the
 * warning pages
 * @note it's also used to build the modal pages displayed when touching the top-right button in the
 * review's first page
 *
 */
typedef struct {
    uint32_t predefinedSet;    ///< bitfield of pre-defined warnings, to be taken in @ref
                               ///< nbgl_warningType_t set it to 0 if not using pre-defined warnings
    const char *dAppProvider;  ///< name of the dApp provider, used in some strings
    const char *reportUrl;     ///< URL of the report, used in some strings
    const char *reportProvider;   ///< name of the security report provider, used in some strings
    const char *providerMessage;  ///< Dedicated provider message. Default one will be used if NULL
    const nbgl_warningDetails_t
        *introDetails;  ///< details displayed when top-right button is touched in intro page
                        ///< (before review) if using pre-defined configuration, set to NULL,
    const nbgl_warningDetails_t
        *reviewDetails;  ///< details displayed when top-right button is touched in first/last page
                         ///< of review if using pre-defined configuration, set to NULL
    const nbgl_contentCenter_t
        *info;  ///< parameters to build the intro warning page, if not using pre-defined
    const nbgl_icon_details_t
        *introTopRightIcon;  ///< icon to use in the intro warning page, if not using pre-defined
    const nbgl_icon_details_t *reviewTopRightIcon;  ///< icon to use in the first/last page of
                                                    ///< review, if not using pre-defined
    const nbgl_preludeDetails_t
        *prelude;  ///< if not null, means that the review can start by a prelude
} nbgl_warning_t;

/**
 * @brief The different types of operation to review
 *
 */
typedef enum {
    TYPE_TRANSACTION
        = 0,        ///< For operations transferring a coin or taken from an account to another
    TYPE_MESSAGE,   ///< For operations signing a message that will not be broadcast on the
                    ///< blockchain
    TYPE_OPERATION  ///< For other types of operation (generic type)
} nbgl_opType_t;

/**
 * @brief This is to use in @ref nbgl_operationType_t when the operation is skippable.
 * This is used
 *
 */
#define SKIPPABLE_OPERATION (1 << 4)

/**
 * @brief This is to use in @ref nbgl_operationType_t when the operation is "blind"
 * This is used to indicate a warning with a top-right button in review first & last page
 *
 */
#define BLIND_OPERATION (1 << 5)

/**
 * @brief This mask is used to describe the type of operation to review with additional options
 * It is a mask of @ref nbgl_opType_t [| @ref SKIPPABLE_OPERATION] [| @ref BLIND_OPERATION]
 *
 */
typedef uint32_t nbgl_operationType_t;

/**
 * @brief The different types of review status
 *
 */
typedef enum {
    STATUS_TYPE_TRANSACTION_SIGNED = 0,
    STATUS_TYPE_TRANSACTION_REJECTED,
    STATUS_TYPE_MESSAGE_SIGNED,
    STATUS_TYPE_MESSAGE_REJECTED,
    STATUS_TYPE_OPERATION_SIGNED,
    STATUS_TYPE_OPERATION_REJECTED,
    STATUS_TYPE_ADDRESS_VERIFIED,
    STATUS_TYPE_ADDRESS_REJECTED,
} nbgl_reviewStatusType_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void nbgl_useCaseHomeAndSettings(const char                   *appName,
                                 const nbgl_icon_details_t    *appIcon,
                                 const char                   *tagline,
                                 const uint8_t                 initSettingPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 const nbgl_homeAction_t      *action,
                                 nbgl_callback_t               quitCallback);

void nbgl_useCaseReview(nbgl_operationType_t              operationType,
                        const nbgl_contentTagValueList_t *tagValueList,
                        const nbgl_icon_details_t        *icon,
                        const char                       *reviewTitle,
                        const char                       *reviewSubTitle,
                        const char                       *finishTitle,
                        nbgl_choiceCallback_t             choiceCallback);

void nbgl_useCaseReviewBlindSigning(nbgl_operationType_t              operationType,
                                    const nbgl_contentTagValueList_t *tagValueList,
                                    const nbgl_icon_details_t        *icon,
                                    const char                       *reviewTitle,
                                    const char                       *reviewSubTitle,
                                    const char                       *finishTitle,
                                    const nbgl_tipBox_t              *tipBox,
                                    nbgl_choiceCallback_t             choiceCallback);
void nbgl_useCaseAdvancedReview(nbgl_operationType_t              operationType,
                                const nbgl_contentTagValueList_t *tagValueList,
                                const nbgl_icon_details_t        *icon,
                                const char                       *reviewTitle,
                                const char                       *reviewSubTitle,
                                const char                       *finishTitle,
                                const nbgl_tipBox_t              *tipBox,
                                const nbgl_warning_t             *warning,
                                nbgl_choiceCallback_t             choiceCallback);

void nbgl_useCaseReviewLight(nbgl_operationType_t              operationType,
                             const nbgl_contentTagValueList_t *tagValueList,
                             const nbgl_icon_details_t        *icon,
                             const char                       *reviewTitle,
                             const char                       *reviewSubTitle,
                             const char                       *finishTitle,
                             nbgl_choiceCallback_t             choiceCallback);

void nbgl_useCaseAddressReview(const char                       *address,
                               const nbgl_contentTagValueList_t *additionalTagValueList,
                               const nbgl_icon_details_t        *icon,
                               const char                       *reviewTitle,
                               const char                       *reviewSubTitle,
                               nbgl_choiceCallback_t             choiceCallback);

void nbgl_useCaseReviewStatus(nbgl_reviewStatusType_t reviewStatusType,
                              nbgl_callback_t         quitCallback);

void nbgl_useCaseReviewStreamingStart(nbgl_operationType_t       operationType,
                                      const nbgl_icon_details_t *icon,
                                      const char                *reviewTitle,
                                      const char                *reviewSubTitle,
                                      nbgl_choiceCallback_t      choiceCallback);

void nbgl_useCaseReviewStreamingBlindSigningStart(nbgl_operationType_t       operationType,
                                                  const nbgl_icon_details_t *icon,
                                                  const char                *reviewTitle,
                                                  const char                *reviewSubTitle,
                                                  nbgl_choiceCallback_t      choiceCallback);

void nbgl_useCaseAdvancedReviewStreamingStart(nbgl_operationType_t       operationType,
                                              const nbgl_icon_details_t *icon,
                                              const char                *reviewTitle,
                                              const char                *reviewSubTitle,
                                              const nbgl_warning_t      *warning,
                                              nbgl_choiceCallback_t      choiceCallback);

void nbgl_useCaseReviewStreamingContinueExt(const nbgl_contentTagValueList_t *tagValueList,
                                            nbgl_choiceCallback_t             choiceCallback,
                                            nbgl_callback_t                   skipCallback);

void nbgl_useCaseReviewStreamingContinue(const nbgl_contentTagValueList_t *tagValueList,
                                         nbgl_choiceCallback_t             choiceCallback);

void nbgl_useCaseReviewStreamingFinish(const char           *finishTitle,
                                       nbgl_choiceCallback_t choiceCallback);

void nbgl_useCaseGenericReview(const nbgl_genericContents_t *contents,
                               const char                   *rejectText,
                               nbgl_callback_t               rejectCallback);

void nbgl_useCaseGenericConfiguration(const char                   *title,
                                      uint8_t                       initPage,
                                      const nbgl_genericContents_t *contents,
                                      nbgl_callback_t               quitCallback);

void nbgl_useCaseGenericSettings(const char                   *appName,
                                 uint8_t                       initPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 nbgl_callback_t               quitCallback);

void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *rejectString,
                        nbgl_choiceCallback_t      callback);
void nbgl_useCaseChoiceWithDetails(const nbgl_icon_details_t *icon,
                                   const char                *message,
                                   const char                *subMessage,
                                   const char                *confirmText,
                                   const char                *cancelText,
                                   nbgl_warningDetails_t     *details,
                                   nbgl_choiceCallback_t      callback);
void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback);

void nbgl_useClock(const nbgl_icon_details_t *h1,
    const nbgl_icon_details_t *h2,
    const nbgl_icon_details_t *m1,
    const nbgl_icon_details_t *m2,
    bool isSuccess, nbgl_callback_t quitCallback);

void nbgl_useCaseConfirm(const char     *message,
                         const char     *subMessage,
                         const char     *confirmText,
                         const char     *rejectText,
                         nbgl_callback_t callback);

void nbgl_useCaseAction(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *actionText,
                        nbgl_callback_t            callback);

void nbgl_useCaseSpinner(const char *text);

void nbgl_useCaseNavigableContent(const char                *title,
                                  uint8_t                    initPage,
                                  uint8_t                    nbPages,
                                  nbgl_callback_t            quitCallback,
                                  nbgl_navCallback_t         navCallback,
                                  nbgl_layoutTouchCallback_t controlsCallback);

// utils
uint8_t nbgl_useCaseGetNbTagValuesInPage(uint8_t                           nbPairs,
                                         const nbgl_contentTagValueList_t *tagValueList,
                                         uint8_t                           startIndex,
                                         bool                             *requireSpecificDisplay);
uint8_t nbgl_useCaseGetNbTagValuesInPageExt(uint8_t                           nbPairs,
                                            const nbgl_contentTagValueList_t *tagValueList,
                                            uint8_t                           startIndex,
                                            bool                              isSkippable,
                                            bool *requireSpecificDisplay);
uint8_t nbgl_useCaseGetNbInfosInPage(uint8_t                       nbInfos,
                                     const nbgl_contentInfoList_t *infosList,
                                     uint8_t                       startIndex,
                                     bool                          withNav);
uint8_t nbgl_useCaseGetNbSwitchesInPage(uint8_t                           nbSwitches,
                                        const nbgl_contentSwitchesList_t *switchesList,
                                        uint8_t                           startIndex,
                                        bool                              withNav);
uint8_t nbgl_useCaseGetNbBarsInPage(uint8_t                       nbBars,
                                    const nbgl_contentBarsList_t *barsList,
                                    uint8_t                       startIndex,
                                    bool                          withNav);
uint8_t nbgl_useCaseGetNbChoicesInPage(uint8_t                          nbChoices,
                                       const nbgl_contentRadioChoice_t *choicesList,
                                       uint8_t                          startIndex,
                                       bool                             withNav);
uint8_t nbgl_useCaseGetNbPagesForTagValueList(const nbgl_contentTagValueList_t *tagValueList);

#ifdef NBGL_KEYPAD
void nbgl_useCaseKeypad(const char             *title,
                        uint8_t                 minDigits,
                        uint8_t                 maxDigits,
                        bool                    shuffled,
                        bool                    hidden,
                        nbgl_pinValidCallback_t validatePinCallback,
                        nbgl_callback_t         backCallback);
#endif  // NBGL_KEYPAD

#ifdef HAVE_SE_TOUCH
// use case drawing
DEPRECATED void nbgl_useCaseHome(const char                *appName,
                                 const nbgl_icon_details_t *appIcon,
                                 const char                *tagline,
                                 bool                       withSettings,
                                 nbgl_callback_t            topRightCallback,
                                 nbgl_callback_t            quitCallback);
DEPRECATED void nbgl_useCaseHomeExt(const char                *appName,
                                    const nbgl_icon_details_t *appIcon,
                                    const char                *tagline,
                                    bool                       withSettings,
                                    const char                *actionButtonText,
                                    nbgl_callback_t            actionCallback,
                                    nbgl_callback_t            topRightCallback,
                                    nbgl_callback_t            quitCallback);
DEPRECATED void nbgl_useCaseSettings(const char                *settingsTitle,
                                     uint8_t                    initPage,
                                     uint8_t                    nbPages,
                                     bool                       touchableTitle,
                                     nbgl_callback_t            quitCallback,
                                     nbgl_navCallback_t         navCallback,
                                     nbgl_layoutTouchCallback_t controlsCallback);
void            nbgl_useCaseReviewStart(const nbgl_icon_details_t *icon,
                                        const char                *reviewTitle,
                                        const char                *reviewSubTitle,
                                        const char                *rejectText,
                                        nbgl_callback_t            continueCallback,
                                        nbgl_callback_t            rejectCallback);
DEPRECATED void nbgl_useCaseRegularReview(uint8_t                    initPage,
                                          uint8_t                    nbPages,
                                          const char                *rejectText,
                                          nbgl_layoutTouchCallback_t buttonCallback,
                                          nbgl_navCallback_t         navCallback,
                                          nbgl_choiceCallback_t      choiceCallback);
void            nbgl_useCaseStaticReview(const nbgl_contentTagValueList_t *tagValueList,
                                         const nbgl_pageInfoLongPress_t   *infoLongPress,
                                         const char                       *rejectText,
                                         nbgl_choiceCallback_t             callback);
void            nbgl_useCaseStaticReviewLight(const nbgl_contentTagValueList_t *tagValueList,
                                              const nbgl_pageInfoLongPress_t   *infoLongPress,
                                              const char                       *rejectText,
                                              nbgl_choiceCallback_t             callback);

DEPRECATED void nbgl_useCaseAddressConfirmationExt(const char                       *address,
                                                   nbgl_choiceCallback_t             callback,
                                                   const nbgl_contentTagValueList_t *tagValueList);
#define nbgl_useCaseAddressConfirmation(__address, __callback) \
    nbgl_useCaseAddressConfirmationExt(__address, __callback, NULL)
#endif  // HAVE_SE_TOUCH

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_USE_CASE_H */
