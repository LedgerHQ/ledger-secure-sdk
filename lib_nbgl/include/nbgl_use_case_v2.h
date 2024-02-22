
/*
 * Structure for more generic navigation
 * the app specify a list of contents which can be of mixed types for example:
 * - a CENTERED_INFO
 * - then a TAG_VALUE_LIST
 * - then a INFO_LONG_PRESS
 */

typedef void (*nbgl_contentCallback_t)(uint8_t contentIndex, nbgl_content_t *content);

typedef struct {
    const nbgl_content_t      *contentsList; ///< array of nbgl_content_t (nbContents items). If NULL, callback is used instead
    nbgl_contentCallback_t     contentGetterCallback;  ///< function to call to retrieve a given content
    uint8_t                    nbContents;  ///< number of contents
} nbgl_genericContents_t;


/*
 * Generic Nav, should be used only in non-nominal cases (for example sub-settings levels)
 */

// API to polish, but main idea is to specify an array of contents and a navigation configuration
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

// TODO add something to be able to restart the useCase at a specific page (for example after a switch confirmation page)
// Probably an initPage params, taking an arbitrary value that would have been given in the switch callback.
void nbgl_useCaseHomeAndSettings(const char                   *appName,
                                 const nbgl_icon_details_t    *appIcon,
                                 const char                   *tagline,
                                 const nbgl_contentInfoList_t *infosList,
                                 const nbgl_genericContents_t *settingContents,
                                 nbgl_callback_t               quitCallback)

// TODO add support for plugin and HomeExt use cases.

/*
 * TX review, exist in three variants "transaction", "message" and "operation"
 * Then all wording are handled by the SDK and not exposed to the user.
 * This bundles nbgl_useCaseReviewStart / nbgl_useCaseStaticReview and nbgl_useCaseConfirm
 */

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

// TODO add an API supporting streaming
// TODO add an API supporting Skip
// TODO add an API supporting ReviewLight???

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

// TODO add an API supporting additional tagValueList


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
