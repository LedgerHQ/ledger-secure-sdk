#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "nbgl_use_case.h"
#include "ux_sync.h"

static nbgl_choiceCallback_t g_choiceCallback;
static nbgl_callback_t       g_rejectCallback;

void *pic(void *addr)
{
    return addr;
}

bool io_recv_and_process_event(void)
{
    bool ret = (bool) mock_type(bool);
    if (!ret && g_choiceCallback) {
        g_choiceCallback(true);
    }
    return ret;
}

void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback)
{
    check_expected(message);
    check_expected(isSuccess);
    g_rejectCallback = quitCallback;
}

void nbgl_useCaseReviewStatus(nbgl_reviewStatusType_t reviewStatusType,
                              nbgl_callback_t         quitCallback)
{
    check_expected(reviewStatusType);
    g_rejectCallback = quitCallback;
}

void nbgl_useCaseGenericConfiguration(const char                   *title,
                                      uint8_t                       initPage,
                                      const nbgl_genericContents_t *contents,
                                      nbgl_callback_t               quitCallback)
{
    check_expected(title);
    check_expected(initPage);
    check_expected(contents);
    check_expected(quitCallback);
}

void nbgl_useCaseChoice(const nbgl_icon_details_t *icon,
                        const char                *message,
                        const char                *subMessage,
                        const char                *confirmText,
                        const char                *cancelText,
                        nbgl_choiceCallback_t      callback)
{
    check_expected(icon);
    check_expected(message);
    check_expected(subMessage);
    check_expected(confirmText);
    check_expected(cancelText);
    g_choiceCallback = callback;
}

void nbgl_useCaseHomeAndSettings(const char                   *appName,
                                 const nbgl_icon_details_t    *appIcon,
                                 const char                   *tagline,
                                 const uint8_t                 initSettingPage,
                                 const nbgl_genericContents_t *settingContents,
                                 const nbgl_contentInfoList_t *infosList,
                                 const nbgl_homeAction_t      *action,
                                 nbgl_callback_t               quitCallback)
{
    check_expected(appName);
    check_expected(appIcon);
    check_expected(tagline);
    check_expected(initSettingPage);
    check_expected(settingContents);
    check_expected(infosList);
    check_expected(action);
}

void nbgl_useCaseReview(nbgl_operationType_t              operationType,
                        const nbgl_contentTagValueList_t *tagValueList,
                        const nbgl_icon_details_t        *icon,
                        const char                       *reviewTitle,
                        const char                       *reviewSubTitle,
                        const char                       *finishTitle,
                        nbgl_choiceCallback_t             choiceCallback)
{
    check_expected(operationType);
    check_expected(tagValueList);
    check_expected(icon);
    check_expected(reviewTitle);
    check_expected(reviewSubTitle);
    check_expected(finishTitle);
    g_choiceCallback = choiceCallback;
}

void nbgl_useCaseReviewLight(nbgl_operationType_t              operationType,
                             const nbgl_contentTagValueList_t *tagValueList,
                             const nbgl_icon_details_t        *icon,
                             const char                       *reviewTitle,
                             const char                       *reviewSubTitle,
                             const char                       *finishTitle,
                             nbgl_choiceCallback_t             choiceCallback)
{
    check_expected(operationType);
    check_expected(tagValueList);
    check_expected(icon);
    check_expected(reviewTitle);
    check_expected(reviewSubTitle);
    check_expected(finishTitle);
    g_choiceCallback = choiceCallback;
}

void nbgl_useCaseGenericReview(const nbgl_genericContents_t *contents,
                               const char                   *rejectText,
                               nbgl_callback_t               rejectCallback)
{
    check_expected(contents);
    check_expected(rejectText);
    g_rejectCallback = rejectCallback;
}

void nbgl_useCaseReviewStreamingStart(nbgl_operationType_t       operationType,
                                      const nbgl_icon_details_t *icon,
                                      const char                *reviewTitle,
                                      const char                *reviewSubTitle,
                                      nbgl_choiceCallback_t      choiceCallback)
{
    check_expected(operationType);
    check_expected(icon);
    check_expected(reviewTitle);
    check_expected(reviewSubTitle);
    g_choiceCallback = choiceCallback;
}

void nbgl_useCaseReviewStreamingContinueExt(const nbgl_contentTagValueList_t *tagValueList,
                                            nbgl_choiceCallback_t             choiceCallback,
                                            nbgl_callback_t                   skipCallback)
{
    check_expected(tagValueList);
    g_choiceCallback = choiceCallback;
    g_rejectCallback = skipCallback;
}

void nbgl_useCaseReviewStreamingContinue(const nbgl_contentTagValueList_t *tagValueList,
                                         nbgl_choiceCallback_t             choiceCallback)
{
    check_expected(tagValueList);
    g_choiceCallback = choiceCallback;
}

void nbgl_useCaseReviewStreamingFinish(const char           *finishTitle,
                                       nbgl_choiceCallback_t choiceCallback)
{
    check_expected(finishTitle);
    g_choiceCallback = choiceCallback;
}

void nbgl_useCaseAddressReview(const char                       *address,
                               const nbgl_contentTagValueList_t *additionalTagValueList,
                               const nbgl_icon_details_t        *icon,
                               const char                       *reviewTitle,
                               const char                       *reviewSubTitle,
                               nbgl_choiceCallback_t             choiceCallback)
{
    check_expected(address);
    check_expected(additionalTagValueList);
    check_expected(icon);
    check_expected(reviewTitle);
    check_expected(reviewSubTitle);
    g_choiceCallback = choiceCallback;
}
