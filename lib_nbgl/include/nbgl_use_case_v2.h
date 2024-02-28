
/*
 * Structure for more generic navigation
 * the app specify a list of contents which can be of mixed types for example:
 * - a CENTERED_INFO
 * - then a TAG_VALUE_LIST
 * - then a INFO_LONG_PRESS
 */

/**
 * @brief prototype of content navigation callback function
 * @param contentIndex content index (0->(nbContents-1)) that is needed by the lib
 * @param content content to fill
 */
typedef void (*nbgl_contentCallback_t)(uint8_t contentIndex, nbgl_content_t *content);

typedef struct {
    bool callback_call_needed;  ///< indicates whether contents should be retrieved using
                                ///< contentsList or contentGetterCallback
    union {
        const nbgl_content_t *contentsList;  ///< array of nbgl_content_t (nbContents items).
        nbgl_contentCallback_t
            contentGetterCallback;  ///< function to call to retrieve a given content
    };
    uint8_t nbContents;  ///< number of contents
} nbgl_genericContents_t;

/**
 * @brief prototype of content navigation callback function
 * @param contentsIndex contents index (0->(nbContents-1)) that is needed by the lib
 * @param contents content to fill
 * @return true if the page content is valid, false if no more page
 */
typedef bool (*nbgl_genericContentsCallback_t)(uint8_t contentsIndex, nbgl_genericContents_t *contents);


/*
 * Generic Nav, should be used only in non-nominal cases (for example sub-settings levels)
 */

// TODO: API to polish, but main idea is to specify an array of contents and a navigation configuration
// like touchable, progressIndicator and all...
// this should be used in apps:
// - that were previously using nbgl_useCaseSettings for that
// - that have custom content to display that doesn't fit classical cases?
void nbgl_useCaseGenericNav(const char                   *title,
                            const nbgl_genericContents_t *contents,
                            uint8_t                       initPage,
                            bool                          touchable,
                            bool                          progressIndicator,
                            nbgl_callback_t               quitCallback);


/*
 * Home and settings
 */

void nbgl_useCaseGenericSettings(const char                   *appName,
                                 uint8_t                       initPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 nbgl_callback_t               quitCallback);

void nbgl_useCaseHomeAndSettings(const char                   *appName,
                                 const nbgl_icon_details_t    *appIcon,
                                 const char                   *tagline,
                                 const uint8_t                *initSettingPage, // Set to NULL if settings shouldn't be displayed right away
                                 const nbgl_contentInfoList_t *infosList,
                                 const nbgl_genericContents_t *settingContents,
                                 nbgl_callback_t               quitCallback);

void nbgl_useCaseHomeExtAndSettings(const char                   *appName,
                                    const nbgl_icon_details_t    *appIcon,
                                    const char                   *tagline,
                                    const uint8_t                *initSettingPage, // Set to NULL if settings shouldn't be displayed right away
                                    const nbgl_contentInfoList_t *infosList,
                                    const nbgl_genericContents_t *settingContents,
                                    nbgl_callback_t               quitCallback,
                                    const char                   *actionText, // Set to NULL if no additional action
                                    nbgl_callback_t               actionCallback); // Set to NULL if no additional action

// Note no specific support for plugin right now, this is so specific it should probably be handled in app side using nbgl_useCaseGenericNav?

/*
 * TX review, exist in three variants "transaction", "message" and "operation"
 * Then all wording are handled by the SDK and not exposed to the user.
 * This bundles nbgl_useCaseReviewStart / nbgl_useCaseStaticReview and nbgl_useCaseConfirm
 */

// Note, maybe at some point an additional API could be proposed to take a nbgl_genericContents_t instead of a tagValueList
// Could be an *Ext version..

void nbgl_useCaseTransactionReview(
    const nbgl_layoutTagValueList_t *tagValueList,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle, /* Most often this is empty, but sometimes indicates a path / index */
    const char *finish_page_text, /* unused on Nano */
    nbgl_choiceCallback_t choice_callback);

void nbgl_useCaseMessageReview(
    const nbgl_layoutTagValueList_t *tagValueList,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle, /* Most often this is empty, but sometimes indicates a path / index */
    const char *finish_page_text, /* unused on Nano */
    nbgl_choiceCallback_t choice_callback);

void nbgl_useCaseOperationReview(
    const nbgl_layoutTagValueList_t *tagValueList,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle, /* Most often this is empty, but sometimes indicates a path / index */
    const char *finish_page_text, /* unused on Nano */
    nbgl_choiceCallback_t choice_callback);


/* Streaming API, the idea is to:
1/ first call nbgl_useCaseTransactionReviewStreamingInit
2/ once choice_callback is called either stop ir confirm==false, or request more
   data then call nbgl_useCaseTransactionReviewStreamingContinue
3/ Repeat 2/ as long as there is additional data
4/ Once there is no more data call nbgl_useCaseTransactionReviewStreamingFinish
5/ At this point, if choice_callback is called with True, then the review is finished

Note that app side, it's probably safer not to use the same function for the choice_callback
for nbgl_useCaseTransactionReviewStreamingFinish as the behavior when receiving confirm==true
should not be the same
*/
void nbgl_useCaseTransactionReviewStreamingInit(
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle, /* Most often this is empty, but sometimes indicates a path / index */
    nbgl_choiceCallback_t choice_callback);

void nbgl_useCaseTransactionReviewStreamingContinue(
    const nbgl_layoutTagValueList_t *tagValueList,
    nbgl_choiceCallback_t choice_callback);

void nbgl_useCaseTransactionReviewStreamingFinish(
    const nbgl_icon_details_t *icon,
    const char *finish_page_text, /* unused on Nano */
    nbgl_choiceCallback_t choice_callback);



// TODO add an API supporting Skip??? Or not and let the app do it by itself with nbgl_useCaseGenericNav
// TODO add an API supporting ReviewLight??? Or not and let the app do it by itself with nbgl_useCaseGenericNav

/*
 * Address verification
 * This bundles nbgl_useCaseReviewStart / nbgl_useCaseAddressConfirmation
 */

void nbgl_useCaseAddressReview(
    const char *address,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle,
    nbgl_choiceCallback_t choice_callback);

void nbgl_useCaseAddressReviewExt(
    const char *address,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle,
    nbgl_choiceCallback_t choice_callback,
    const nbgl_layoutTagValueList_t *tagValueList);

// Note, maybe at some point an additional API could be proposed to take a nbgl_genericContents_t instead of a tagValueList


/*
 * Status
 */

// Generic, should not be used
void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback);


// For TX, we have the same three variants "transaction", "message" and "operation"
static inline void nbgl_useCaseTransactionSigned(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Transaction signed", true, quitCallback);
}

static inline void nbgl_useCaseTransactionrejected(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Transaction rejected", false, quitCallback);
}

static inline void nbgl_useCaseMessageSigned(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Message signed", true, quitCallback);
}

static inline void nbgl_useCaseMessageRejected(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Message rejected", false, quitCallback);
}

static inline void nbgl_useCaseOperationSigned(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Operation signed", true, quitCallback);
}

static inline void nbgl_useCaseOperationRejected(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Operation rejected", false, quitCallback);
}

static inline void nbgl_useCaseAddressVerified(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Address verified", true, quitCallback);
}

static inline void nbgl_useCaseAddressRejected(nbgl_callback_t quitCallback) {
    nbgl_useCaseStatus("Address verification\ncancelled", false, quitCallback);
}


/*
 * Misc
 */

void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *rejectString,
                        nbgl_choiceCallback_t      callback);

// Same as nbgl_useCaseChoice but as a modal and without icon choice
void nbgl_useCaseConfirm(const char     *message,
                         const char     *subMessage,
                         const char     *confirmText,
                         const char     *rejectText,
                         nbgl_callback_t callback);

void nbgl_useCaseSpinner(const char *text);

void nbgl_useCaseViewDetails(const char *tag, const char *value, bool wrapping);
