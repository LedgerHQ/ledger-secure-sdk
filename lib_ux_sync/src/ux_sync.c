#ifdef HAVE_NBGL

#include "ux_sync.h"

static ux_sync_ret_t g_ret;
static bool          g_ended;
static const char   *g_finish_title;

static void choice_callback(bool confirm)
{
    if (confirm) {
        g_ret = UX_SYNC_RET_APPROVED;
    }
    else {
        g_ret = UX_SYNC_RET_REJECTED;
    }

    g_ended = true;
}

static void skip_callback(void)
{
    nbgl_useCaseReviewStreamingFinish(g_finish_title, choice_callback);
}

static void quit_callback(void)
{
    g_ret   = UX_SYNC_RET_QUITTED;
    g_ended = true;
}

static void rejected_callback(void)
{
    g_ret   = UX_SYNC_RET_REJECTED;
    g_ended = true;
}

static void ux_sync_init(void)
{
    g_ended = false;
    g_ret   = UX_SYNC_RET_ERROR;
}

static ux_sync_ret_t ux_sync_wait(bool exitOnApdu)
{
    bool apduReceived;

    while (!g_ended) {
        apduReceived = io_recv_and_process_event();
        if (exitOnApdu && apduReceived) {
            return UX_SYNC_RET_APDU_RECEIVED;
        }
    }

    return g_ret;
}

/**
 * @brief Sets the return code of synchronous UX calls. Can be used by content action callbacks
 * defined by application code.
 *
 * @param ret return code to set.
 */
void ux_sync_setReturnCode(ux_sync_ret_t ret)
{
    g_ret = ret;
}

/**
 * @brief Sets the ended flag of synchronous UX calls. Can be used by content action callbacks
 * defined by application code to end the UX flow.
 *
 * @param ended flag to set.
 */
void ux_sync_setEnded(bool ended)
{
    g_ended = ended;
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
 *
 * @return ret code:
 *         - UX_SYNC_RET_QUITTED
 *         - UX_SYNC_RET_APDU_RECEIVED
 */
ux_sync_ret_t ux_sync_homeAndSettings(const char                   *appName,
                                      const nbgl_icon_details_t    *appIcon,
                                      const char                   *tagline,
                                      const uint8_t                 initSettingPage,
                                      const nbgl_genericContents_t *settingContents,
                                      const nbgl_contentInfoList_t *infosList,
                                      const nbgl_homeAction_t      *action)
{
    ux_sync_init();
    nbgl_useCaseHomeAndSettings(appName,
                                appIcon,
                                tagline,
                                initSettingPage,
                                settingContents,
                                infosList,
                                action,
                                quit_callback);
    return ux_sync_wait(true);
}

/**
 * @brief Draws a flow of pages of a review. A back key is available on top-left of the screen,
 * except in first page It is possible to go to next page thanks to "tap to continue".
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param tagValueList list of tag/value pairs
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param finishTitle string used in the last review page
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_review(nbgl_operationType_t              operationType,
                             const nbgl_contentTagValueList_t *tagValueList,
                             const nbgl_icon_details_t        *icon,
                             const char                       *reviewTitle,
                             const char                       *reviewSubTitle,
                             const char                       *finishTitle)
{
    ux_sync_init();
    nbgl_useCaseReview(operationType,
                       tagValueList,
                       icon,
                       reviewTitle,
                       reviewSubTitle,
                       finishTitle,
                       choice_callback);
    return ux_sync_wait(false);
}

/**
 * @brief Draws a flow of pages of a light review. A back key is available on top-left of the
 * screen,except in first page It is possible to go to next page thanks to "tap to continue".
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a short press one
 *
 * @param operationType type of operation (Operation, Transaction, Message)
 * @param tagValueList list of tag/value pairs
 * @param icon icon used on first and last review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 * @param finishTitle string used in the last review page
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_reviewLight(nbgl_operationType_t              operationType,
                                  const nbgl_contentTagValueList_t *tagValueList,
                                  const nbgl_icon_details_t        *icon,
                                  const char                       *reviewTitle,
                                  const char                       *reviewSubTitle,
                                  const char                       *finishTitle)
{
    ux_sync_init();
    nbgl_useCaseReviewLight(operationType,
                            tagValueList,
                            icon,
                            reviewTitle,
                            reviewSubTitle,
                            finishTitle,
                            choice_callback);
    return ux_sync_wait(false);
}
/**
 * @brief Draws a flow of pages of an extended address verification page.
 * A back key is available on top-left of the screen,
 * except in first page It is possible to go to next page thanks to "tap to continue".
 * @note  All tag/value pairs are provided in the API and the number of pages is automatically
 * computed, the last page being a long press one
 *
 * @param address address to confirm (NULL terminated string)
 * @param additionalTagValueList list of tag/value pairs (can be NULL) (must fit in a single page,
 * and be persistent because no copy)
 * @param callback callback called when button or footer is touched (if true, button, if false
 * footer)
 * @param icon icon used on the first review page
 * @param reviewTitle string used in the first review page
 * @param reviewSubTitle string to set under reviewTitle (can be NULL)
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_addressReview(const char                       *address,
                                    const nbgl_contentTagValueList_t *additionalTagValueList,
                                    const nbgl_icon_details_t        *icon,
                                    const char                       *reviewTitle,
                                    const char                       *reviewSubTitle)
{
    ux_sync_init();
    nbgl_useCaseAddressReview(
        address, additionalTagValueList, icon, reviewTitle, reviewSubTitle, choice_callback);
    return ux_sync_wait(false);
}

/**
 * @brief Draws a transient (3s) status page for the reviewStatusType
 *
 * @param reviewStatusType type of status to display
 *
 * @return ret code:
 *         - UX_SYNC_RET_QUITTED
 */
ux_sync_ret_t ux_sync_reviewStatus(nbgl_reviewStatusType_t reviewStatusType)
{
    ux_sync_init();
    nbgl_useCaseReviewStatus(reviewStatusType, quit_callback);
    return ux_sync_wait(false);
}

/**
 * @brief Draws a transient (3s) status page, either of success or failure, with the given message
 *
 * @param message string to set in middle of page (Upper case for success)
 * @param isSuccess if true, message is drawn in a Ledger style (with corners)
 *
 * @return ret code:
 *         - UX_SYNC_RET_QUITTED
 */
ux_sync_ret_t ux_sync_status(const char *message, bool isSuccess)
{
    ux_sync_init();
    nbgl_useCaseStatus(message, isSuccess, quit_callback);
    return ux_sync_wait(false);
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
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_choice(const nbgl_icon_details_t *icon,
                             const char                *message,
                             const char                *subMessage,
                             const char                *confirmText,
                             const char                *cancelText)
{
    ux_sync_init();
    nbgl_useCaseChoice(icon, message, subMessage, confirmText, cancelText, choice_callback);
    return ux_sync_wait(false);
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
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_reviewStreamingStart(nbgl_operationType_t       operationType,
                                           const nbgl_icon_details_t *icon,
                                           const char                *reviewTitle,
                                           const char                *reviewSubTitle)

{
    ux_sync_init();
    nbgl_useCaseReviewStreamingStart(
        operationType, icon, reviewTitle, reviewSubTitle, choice_callback);
    return ux_sync_wait(false);
}

/**
 * @brief Continue drawing the flow of pages of a review.
 * @note  This should be called after a call to nbgl_useCaseReviewStreamingStart and can be followed
 *        by others calls to nbgl_useCaseReviewStreamingContinue and finally to
 *        nbgl_useCaseReviewStreamingFinish.
 *
 * @param tagValueList list of tag/value pairs
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_reviewStreamingContinue(const nbgl_contentTagValueList_t *tagValueList)

{
    return ux_sync_reviewStreamingContinueExt(tagValueList, NULL);
}

/**
 * @brief Continue drawing the flow of pages of a review.
 * @note  This should be called after a call to nbgl_useCaseReviewStreamingStart with @ref
 * SKIPPABLE_OPERATION and can be followed by others calls to nbgl_useCaseReviewStreamingContinue
 * and finally to nbgl_useCaseReviewStreamingFinish. If "Skip" is touched, a choice will be offered
 * to go directly to last page, using the given finish title.
 *
 * @param tagValueList list of tag/value pairs
 * @param finishTitle string used in the last review page if "skip" is used
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_reviewStreamingContinueExt(const nbgl_contentTagValueList_t *tagValueList,
                                                 const char                       *finishTitle)

{
    ux_sync_init();
    g_finish_title = finishTitle;
    nbgl_useCaseReviewStreamingContinueExt(tagValueList, choice_callback, skip_callback);
    return ux_sync_wait(false);
}

/**
 * @brief finish drawing the flow of pages of a review.
 * @note  This should be called after a call to nbgl_useCaseReviewStreamingContinue.
 *
 * @param finishTitle string used in the last review page
 *
 * @return ret code:
 *         - UX_SYNC_RET_APPROVED
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_reviewStreamingFinish(const char *finishTitle)

{
    ux_sync_init();
    nbgl_useCaseReviewStreamingFinish(finishTitle, choice_callback);
    return ux_sync_wait(false);
}

/**
 * @brief Draws a flow of pages of a review with automatic pagination depending on content
 *        to be displayed that is passed through contents.
 *
 * @param contents contents to be displayed
 * @param rejectText text to use in footer
 *
 * @return ret code:
 *         - UX_SYNC_RET_REJECTED
 */
ux_sync_ret_t ux_sync_genericReview(const nbgl_genericContents_t *contents, const char *rejectText)

{
    ux_sync_init();
    nbgl_useCaseGenericReview(contents, rejectText, rejected_callback);
    return ux_sync_wait(false);
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
 *
 * @return ret code:
 *         - UX_SYNC_RET_QUITTED
 *         - UX_SYNC_RET_APDU_RECEIVED
 */
ux_sync_ret_t ux_sync_genericConfiguration(const char                   *title,
                                           uint8_t                       initPage,
                                           const nbgl_genericContents_t *contents)

{
    ux_sync_init();
    nbgl_useCaseGenericConfiguration(title, initPage, contents, quit_callback);
    return ux_sync_wait(true);
}

#endif
