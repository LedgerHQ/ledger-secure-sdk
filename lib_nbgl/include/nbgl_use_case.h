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
#ifdef TARGET_STAX
#define NB_MAX_LINES_IN_DETAILS 12
#else  // TARGET_STAX
#define NB_MAX_LINES_IN_DETAILS 11
#endif  // TARGET_STAX

/**
 *  @brief maximum number of lines for value field in review pages
 */
#define NB_MAX_LINES_IN_REVIEW 9

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
 * @param content pin value
 * @param length pin length
 */
typedef void (*nbgl_pinValidCallback_t)(const uint8_t *content, uint8_t page);

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

typedef struct {
    const char                *text;  ///< text to use in action button in Home page
    const nbgl_icon_details_t *icon;  ///< icon to use in action button in Home page
    nbgl_callback_t callback;  ///< function to call when action button is touched in Home page
} nbgl_homeAction_t;

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

void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *rejectString,
                        nbgl_choiceCallback_t      callback);

void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback);

void nbgl_useCaseSpinner(const char *text);

#ifdef HAVE_SE_TOUCH
// utils
uint8_t nbgl_useCaseGetNbTagValuesInPage(uint8_t                           nbPairs,
                                         const nbgl_contentTagValueList_t *tagValueList,
                                         uint8_t                           startIndex,
                                         bool                             *requireSpecificDisplay);
uint8_t nbgl_useCaseGetNbPagesForTagValueList(const nbgl_contentTagValueList_t *tagValueList);

// use case drawing
void nbgl_useCaseHome(const char                *appName,
                      const nbgl_icon_details_t *appIcon,
                      const char                *tagline,
                      bool                       withSettings,
                      nbgl_callback_t            topRightCallback,
                      nbgl_callback_t            quitCallback);
void nbgl_useCaseHomeExt(const char                *appName,
                         const nbgl_icon_details_t *appIcon,
                         const char                *tagline,
                         bool                       withSettings,
                         const char                *actionButtonText,
                         nbgl_callback_t            actionCallback,
                         nbgl_callback_t            topRightCallback,
                         nbgl_callback_t            quitCallback);
void nbgl_useCasePlugInHome(const char                *plugInName,
                            const char                *appName,
                            const nbgl_icon_details_t *appIcon,
                            const char                *tagline,
                            const char                *subTagline,
                            bool                       withSettings,
                            nbgl_callback_t            topRightCallback,
                            nbgl_callback_t            quitCallback);
void nbgl_useCaseSettings(const char                *settingsTitle,
                          uint8_t                    initPage,
                          uint8_t                    nbPages,
                          bool                       touchableTitle,
                          nbgl_callback_t            quitCallback,
                          nbgl_navCallback_t         navCallback,
                          nbgl_layoutTouchCallback_t controlsCallback);
void nbgl_useCaseGenericSettings(const char                   *appName,
                                 uint8_t                       initPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 nbgl_callback_t               quitCallback);
void nbgl_useCaseConfirm(const char     *message,
                         const char     *subMessage,
                         const char     *confirmText,
                         const char     *rejectText,
                         nbgl_callback_t callback);
void nbgl_useCaseReviewStart(const nbgl_icon_details_t *icon,
                             const char                *reviewTitle,
                             const char                *reviewSubTitle,
                             const char                *rejectText,
                             nbgl_callback_t            continueCallback,
                             nbgl_callback_t            rejectCallback);
void nbgl_useCaseRegularReview(uint8_t                    initPage,
                               uint8_t                    nbPages,
                               const char                *rejectText,
                               nbgl_layoutTouchCallback_t buttonCallback,
                               nbgl_navCallback_t         navCallback,
                               nbgl_choiceCallback_t      choiceCallback);
void nbgl_useCaseForwardOnlyReview(const char                *rejectText,
                                   nbgl_layoutTouchCallback_t buttonCallback,
                                   nbgl_navCallback_t         navCallback,
                                   nbgl_choiceCallback_t      choiceCallback);
void nbgl_useCaseForwardOnlyReviewNoSkip(const char                *rejectText,
                                         nbgl_layoutTouchCallback_t buttonCallback,
                                         nbgl_navCallback_t         navCallback,
                                         nbgl_choiceCallback_t      choiceCallback);
void nbgl_useCaseStaticReview(const nbgl_contentTagValueList_t *tagValueList,
                              const nbgl_pageInfoLongPress_t   *infoLongPress,
                              const char                       *rejectText,
                              nbgl_choiceCallback_t             callback);
void nbgl_useCaseStaticReviewLight(const nbgl_contentTagValueList_t *tagValueList,
                                   const nbgl_pageInfoLongPress_t   *infoLongPress,
                                   const char                       *rejectText,
                                   nbgl_choiceCallback_t             callback);
void nbgl_useCaseViewDetails(const char *tag, const char *value, bool wrapping);
void nbgl_useCaseAddressConfirmation(const char *address, nbgl_choiceCallback_t callback);
void nbgl_useCaseAddressConfirmationExt(const char                       *address,
                                        nbgl_choiceCallback_t             callback,
                                        const nbgl_contentTagValueList_t *tagValueList);
#ifdef NBGL_KEYPAD
void nbgl_useCaseKeypadDigits(const char                *title,
                              uint8_t                    minDigits,
                              uint8_t                    maxDigits,
                              uint8_t                    backToken,
                              bool                       shuffled,
                              tune_index_e               tuneId,
                              nbgl_pinValidCallback_t    validatePinCallback,
                              nbgl_layoutTouchCallback_t actionCallback);
void nbgl_useCaseKeypadPIN(const char                *title,
                           uint8_t                    minDigits,
                           uint8_t                    maxDigits,
                           uint8_t                    backToken,
                           bool                       shuffled,
                           tune_index_e               tuneId,
                           nbgl_pinValidCallback_t    validatePinCallback,
                           nbgl_layoutTouchCallback_t actionCallback);
/**
 * @deprecated
 * See #nbgl_useCaseKeypadPIN
 */
DEPRECATED static inline void nbgl_useCaseKeypad(const char                *title,
                                                 uint8_t                    minDigits,
                                                 uint8_t                    maxDigits,
                                                 uint8_t                    backToken,
                                                 bool                       shuffled,
                                                 tune_index_e               tuneId,
                                                 nbgl_pinValidCallback_t    validatePinCallback,
                                                 nbgl_layoutTouchCallback_t actionCallback)
{
    nbgl_useCaseKeypadPIN(title,
                          minDigits,
                          maxDigits,
                          backToken,
                          shuffled,
                          tuneId,
                          validatePinCallback,
                          actionCallback);
}
#endif  // NBGL_KEYPAD
#endif  // HAVE_SE_TOUCH

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_USE_CASE_H */
