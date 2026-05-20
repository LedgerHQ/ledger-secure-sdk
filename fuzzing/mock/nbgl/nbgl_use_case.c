/* NBGL use-case mocks.
 * fuzz_mock_nbgl_reject controls approve/reject callback paths.
 */

#include "nbgl_use_case.h"
#include <stdint.h>

uint8_t fuzz_mock_nbgl_reject;

static inline bool _nbgl_approve(void)
{
    return fuzz_mock_nbgl_reject == 0;
}

/* review / approval */

void nbgl_useCaseReview(nbgl_operationType_t              operationType __attribute__((unused)),
                        const nbgl_contentTagValueList_t *tagValueList __attribute__((unused)),
                        const nbgl_icon_details_t        *icon __attribute__((unused)),
                        const char                       *reviewTitle __attribute__((unused)),
                        const char                       *reviewSubTitle __attribute__((unused)),
                        const char                       *finishTitle __attribute__((unused)),
                        nbgl_choiceCallback_t             choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseReviewBlindSigning(nbgl_operationType_t operationType __attribute__((unused)),
                                    const nbgl_contentTagValueList_t *tagValueList
                                    __attribute__((unused)),
                                    const nbgl_icon_details_t *icon __attribute__((unused)),
                                    const char                *reviewTitle __attribute__((unused)),
                                    const char           *reviewSubTitle __attribute__((unused)),
                                    const char           *finishTitle __attribute__((unused)),
                                    const nbgl_tipBox_t  *tipBox __attribute__((unused)),
                                    nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseAdvancedReview(nbgl_operationType_t operationType __attribute__((unused)),
                                const nbgl_contentTagValueList_t *tagValueList
                                __attribute__((unused)),
                                const nbgl_icon_details_t *icon __attribute__((unused)),
                                const char                *reviewTitle __attribute__((unused)),
                                const char                *reviewSubTitle __attribute__((unused)),
                                const char                *finishTitle __attribute__((unused)),
                                const nbgl_tipBox_t       *tipBox __attribute__((unused)),
                                const nbgl_warning_t      *warning __attribute__((unused)),
                                nbgl_choiceCallback_t      choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseReviewLight(nbgl_operationType_t operationType __attribute__((unused)),
                             const nbgl_contentTagValueList_t *tagValueList __attribute__((unused)),
                             const nbgl_icon_details_t        *icon __attribute__((unused)),
                             const char                       *reviewTitle __attribute__((unused)),
                             const char           *reviewSubTitle __attribute__((unused)),
                             const char           *finishTitle __attribute__((unused)),
                             nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseAddressReview(const char                       *address __attribute__((unused)),
                               const nbgl_contentTagValueList_t *additionalTagValueList
                               __attribute__((unused)),
                               const nbgl_icon_details_t *icon __attribute__((unused)),
                               const char                *reviewTitle __attribute__((unused)),
                               const char                *reviewSubTitle __attribute__((unused)),
                               nbgl_choiceCallback_t      choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseChoice(const nbgl_icon_details_t *icon __attribute__((unused)),
                        const char                *message __attribute__((unused)),
                        const char                *subMessage __attribute__((unused)),
                        const char                *confirmText __attribute__((unused)),
                        const char                *rejectString __attribute__((unused)),
                        nbgl_choiceCallback_t      callback)
{
    if (callback) {
        callback(_nbgl_approve());
    }
}

void nbgl_useCaseChoiceWithDetails(const nbgl_icon_details_t *icon __attribute__((unused)),
                                   const char                *message __attribute__((unused)),
                                   const char                *subMessage __attribute__((unused)),
                                   const char                *confirmText __attribute__((unused)),
                                   const char                *cancelText __attribute__((unused)),
                                   nbgl_warningDetails_t     *details __attribute__((unused)),
                                   nbgl_choiceCallback_t      callback)
{
    if (callback) {
        callback(_nbgl_approve());
    }
}

/* streaming review */

void nbgl_useCaseReviewStreamingStart(nbgl_operationType_t operationType __attribute__((unused)),
                                      const nbgl_icon_details_t *icon __attribute__((unused)),
                                      const char           *reviewTitle __attribute__((unused)),
                                      const char           *reviewSubTitle __attribute__((unused)),
                                      nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseReviewStreamingBlindSigningStart(nbgl_operationType_t operationType
                                                  __attribute__((unused)),
                                                  const nbgl_icon_details_t *icon
                                                  __attribute__((unused)),
                                                  const char *reviewTitle __attribute__((unused)),
                                                  const char *reviewSubTitle
                                                  __attribute__((unused)),
                                                  nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseAdvancedReviewStreamingStart(nbgl_operationType_t operationType
                                              __attribute__((unused)),
                                              const nbgl_icon_details_t *icon
                                              __attribute__((unused)),
                                              const char *reviewTitle __attribute__((unused)),
                                              const char *reviewSubTitle __attribute__((unused)),
                                              const nbgl_warning_t *warning __attribute__((unused)),
                                              nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseReviewStreamingContinueExt(const nbgl_contentTagValueList_t *tagValueList
                                            __attribute__((unused)),
                                            nbgl_choiceCallback_t choiceCallback,
                                            nbgl_callback_t skipCallback __attribute__((unused)))
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseReviewStreamingContinue(const nbgl_contentTagValueList_t *tagValueList
                                         __attribute__((unused)),
                                         nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseReviewStreamingFinish(const char           *finishTitle __attribute__((unused)),
                                       nbgl_choiceCallback_t choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

/* status / navigation */

void nbgl_useCaseReviewStatus(nbgl_reviewStatusType_t reviewStatusType __attribute__((unused)),
                              nbgl_callback_t         quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseHomeAndSettings(const char                *appName __attribute__((unused)),
                                 const nbgl_icon_details_t *appIcon __attribute__((unused)),
                                 const char                *tagline __attribute__((unused)),
                                 const uint8_t              initSettingPage __attribute__((unused)),
                                 const nbgl_genericContents_t *settingContents
                                 __attribute__((unused)),
                                 const nbgl_contentInfoList_t *infosList __attribute__((unused)),
                                 const nbgl_homeAction_t      *action __attribute__((unused)),
                                 nbgl_callback_t               quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseGenericReview(const nbgl_genericContents_t *contents __attribute__((unused)),
                               const char                   *rejectText __attribute__((unused)),
                               nbgl_callback_t               rejectCallback __attribute__((unused)))
{
}

void nbgl_useCaseGenericConfiguration(const char *title __attribute__((unused)),
                                      uint8_t     initPage __attribute__((unused)),
                                      const nbgl_genericContents_t *contents
                                      __attribute__((unused)),
                                      nbgl_callback_t quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseGenericSettings(const char                   *appName __attribute__((unused)),
                                 uint8_t                       initPage __attribute__((unused)),
                                 const nbgl_genericContents_t *settingContents
                                 __attribute__((unused)),
                                 const nbgl_contentInfoList_t *infosList __attribute__((unused)),
                                 nbgl_callback_t               quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseStatus(const char     *message __attribute__((unused)),
                        bool            isSuccess __attribute__((unused)),
                        nbgl_callback_t quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseSpinner(const char *text __attribute__((unused))) {}

/* actions / confirmations */

void nbgl_useCaseConfirm(const char     *message __attribute__((unused)),
                         const char     *subMessage __attribute__((unused)),
                         const char     *confirmText __attribute__((unused)),
                         const char     *rejectText __attribute__((unused)),
                         nbgl_callback_t callback)
{
    if (callback) {
        callback();
    }
}

void nbgl_useCaseAction(const nbgl_icon_details_t *icon __attribute__((unused)),
                        const char                *message __attribute__((unused)),
                        const char                *actionText __attribute__((unused)),
                        nbgl_callback_t            callback)
{
    if (callback) {
        callback();
    }
}

void nbgl_useCaseNavigableContent(const char                *title __attribute__((unused)),
                                  uint8_t                    initPage __attribute__((unused)),
                                  uint8_t                    nbPages __attribute__((unused)),
                                  nbgl_callback_t            quitCallback __attribute__((unused)),
                                  nbgl_navCallback_t         navCallback __attribute__((unused)),
                                  nbgl_layoutTouchCallback_t controlsCallback
                                  __attribute__((unused)))
{
}

/* HAVE_SE_TOUCH use cases */

#ifdef HAVE_SE_TOUCH

void nbgl_useCaseReviewStart(const nbgl_icon_details_t *icon __attribute__((unused)),
                             const char                *reviewTitle __attribute__((unused)),
                             const char                *reviewSubTitle __attribute__((unused)),
                             const char                *rejectText __attribute__((unused)),
                             nbgl_callback_t            continueCallback,
                             nbgl_callback_t            rejectCallback __attribute__((unused)))
{
    if (continueCallback) {
        continueCallback();
    }
}

void nbgl_useCaseRegularReview(uint8_t                    initPage __attribute__((unused)),
                               uint8_t                    nbPages __attribute__((unused)),
                               const char                *rejectText __attribute__((unused)),
                               nbgl_layoutTouchCallback_t buttonCallback __attribute__((unused)),
                               nbgl_navCallback_t         navCallback __attribute__((unused)),
                               nbgl_choiceCallback_t      choiceCallback)
{
    if (choiceCallback) {
        choiceCallback(_nbgl_approve());
    }
}

void nbgl_useCaseStaticReview(const nbgl_contentTagValueList_t *tagValueList
                              __attribute__((unused)),
                              const nbgl_pageInfoLongPress_t *infoLongPress __attribute__((unused)),
                              const char                     *rejectText __attribute__((unused)),
                              nbgl_choiceCallback_t           callback)
{
    if (callback) {
        callback(_nbgl_approve());
    }
}

void nbgl_useCaseStaticReviewLight(const nbgl_contentTagValueList_t *tagValueList
                                   __attribute__((unused)),
                                   const nbgl_pageInfoLongPress_t *infoLongPress
                                   __attribute__((unused)),
                                   const char           *rejectText __attribute__((unused)),
                                   nbgl_choiceCallback_t callback)
{
    if (callback) {
        callback(_nbgl_approve());
    }
}

void nbgl_useCaseAddressConfirmationExt(const char           *address __attribute__((unused)),
                                        nbgl_choiceCallback_t callback,
                                        const nbgl_contentTagValueList_t *tagValueList
                                        __attribute__((unused)))
{
    if (callback) {
        callback(_nbgl_approve());
    }
}

void nbgl_useCaseHome(const char                *appName __attribute__((unused)),
                      const nbgl_icon_details_t *appIcon __attribute__((unused)),
                      const char                *tagline __attribute__((unused)),
                      bool                       withSettings __attribute__((unused)),
                      nbgl_callback_t            topRightCallback __attribute__((unused)),
                      nbgl_callback_t            quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseHomeExt(const char                *appName __attribute__((unused)),
                         const nbgl_icon_details_t *appIcon __attribute__((unused)),
                         const char                *tagline __attribute__((unused)),
                         bool                       withSettings __attribute__((unused)),
                         const char                *actionButtonText __attribute__((unused)),
                         nbgl_callback_t            actionCallback __attribute__((unused)),
                         nbgl_callback_t            topRightCallback __attribute__((unused)),
                         nbgl_callback_t            quitCallback __attribute__((unused)))
{
}

void nbgl_useCaseSettings(const char                *settingsTitle __attribute__((unused)),
                          uint8_t                    initPage __attribute__((unused)),
                          uint8_t                    nbPages __attribute__((unused)),
                          bool                       touchableTitle __attribute__((unused)),
                          nbgl_callback_t            quitCallback __attribute__((unused)),
                          nbgl_navCallback_t         navCallback __attribute__((unused)),
                          nbgl_layoutTouchCallback_t controlsCallback __attribute__((unused)))
{
}

#endif /* HAVE_SE_TOUCH */

/* utility functions */

uint8_t nbgl_useCaseGetNbTagValuesInPage(uint8_t                           nbPairs,
                                         const nbgl_contentTagValueList_t *tagValueList
                                         __attribute__((unused)),
                                         uint8_t startIndex __attribute__((unused)),
                                         bool   *requireSpecificDisplay)
{
    if (requireSpecificDisplay) {
        *requireSpecificDisplay = false;
    }
    return nbPairs;
}

uint8_t nbgl_useCaseGetNbTagValuesInPageExt(uint8_t                           nbPairs,
                                            const nbgl_contentTagValueList_t *tagValueList
                                            __attribute__((unused)),
                                            uint8_t startIndex __attribute__((unused)),
                                            bool    isSkippable __attribute__((unused)),
                                            bool   *requireSpecificDisplay)
{
    if (requireSpecificDisplay) {
        *requireSpecificDisplay = false;
    }
    return nbPairs;
}

uint8_t nbgl_useCaseGetNbInfosInPage(uint8_t                       nbInfos,
                                     const nbgl_contentInfoList_t *infosList
                                     __attribute__((unused)),
                                     uint8_t startIndex __attribute__((unused)),
                                     bool    withNav __attribute__((unused)))
{
    return nbInfos;
}

uint8_t nbgl_useCaseGetNbSwitchesInPage(uint8_t                           nbSwitches,
                                        const nbgl_contentSwitchesList_t *switchesList
                                        __attribute__((unused)),
                                        uint8_t startIndex __attribute__((unused)),
                                        bool    withNav __attribute__((unused)))
{
    return nbSwitches;
}

uint8_t nbgl_useCaseGetNbBarsInPage(uint8_t                       nbBars,
                                    const nbgl_contentBarsList_t *barsList __attribute__((unused)),
                                    uint8_t startIndex __attribute__((unused)),
                                    bool    withNav __attribute__((unused)))
{
    return nbBars;
}

uint8_t nbgl_useCaseGetNbChoicesInPage(uint8_t                          nbChoices,
                                       const nbgl_contentRadioChoice_t *choicesList
                                       __attribute__((unused)),
                                       uint8_t startIndex __attribute__((unused)),
                                       bool    withNav __attribute__((unused)))
{
    return nbChoices;
}

uint8_t nbgl_useCaseGetNbPagesForTagValueList(const nbgl_contentTagValueList_t *tagValueList
                                              __attribute__((unused)))
{
    return 1;
}

#ifdef NBGL_KEYPAD
void nbgl_useCaseKeypad(const char             *title __attribute__((unused)),
                        uint8_t                 minDigits __attribute__((unused)),
                        uint8_t                 maxDigits __attribute__((unused)),
                        bool                    shuffled __attribute__((unused)),
                        bool                    hidden __attribute__((unused)),
                        nbgl_pinValidCallback_t validatePinCallback __attribute__((unused)),
                        nbgl_callback_t         backCallback __attribute__((unused)))
{
}
#endif /* NBGL_KEYPAD */
