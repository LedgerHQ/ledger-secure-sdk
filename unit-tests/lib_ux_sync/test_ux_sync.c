#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include "nbgl_screen.h"
#include "nbgl_debug.h"
#include "ux_loc.h"
#include "ux_sync.h"

#define UNUSED(x) (void) x

unsigned long gLogger = 0;

const char                      *g_appName = "appName";
const nbgl_icon_details_t        g_appIcon;
const char                      *g_tagline = "tag line";
nbgl_genericContents_t           g_settingContents;
nbgl_contentInfoList_t           g_infosList;
const nbgl_contentTagValueList_t g_tagValueList;

static void test_home(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_string(nbgl_useCaseHomeAndSettings, appName, g_appName);
    expect_memory(nbgl_useCaseHomeAndSettings, appIcon, &g_appIcon, sizeof(g_appIcon));
    expect_string(nbgl_useCaseHomeAndSettings, tagline, g_tagline);
    expect_value(nbgl_useCaseHomeAndSettings, initSettingPage, 0);
    expect_memory(nbgl_useCaseHomeAndSettings,
                  settingContents,
                  &g_settingContents,
                  sizeof(g_settingContents));
    expect_memory(nbgl_useCaseHomeAndSettings, infosList, &g_infosList, sizeof(g_infosList));
    expect_value(nbgl_useCaseHomeAndSettings, action, NULL);

    will_return(io_recv_and_process_event, true);
    ret = ux_sync_homeAndSettings(
        g_appName, &g_appIcon, g_tagline, 0, &g_settingContents, &g_infosList, NULL);
    assert_int_equal(ret, UX_SYNC_RET_APDU_RECEIVED);
}

static void test_review(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_value(nbgl_useCaseReview, operationType, TYPE_TRANSACTION);
    expect_memory(nbgl_useCaseReview, tagValueList, &g_tagValueList, sizeof(g_tagValueList));
    expect_memory(nbgl_useCaseReview, icon, &g_appIcon, sizeof(g_appIcon));
    expect_string(nbgl_useCaseReview, reviewTitle, "Review Title");
    expect_string(nbgl_useCaseReview, reviewSubTitle, "Review Sub-title");
    expect_string(nbgl_useCaseReview, finishTitle, "Finish Title");

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_review(TYPE_TRANSACTION,
                         &g_tagValueList,
                         &g_appIcon,
                         "Review Title",
                         "Review Sub-title",
                         "Finish Title");
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

static void test_review_light(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_value(nbgl_useCaseReviewLight, operationType, TYPE_TRANSACTION);
    expect_memory(nbgl_useCaseReviewLight, tagValueList, &g_tagValueList, sizeof(g_tagValueList));
    expect_memory(nbgl_useCaseReviewLight, icon, &g_appIcon, sizeof(g_appIcon));
    expect_string(nbgl_useCaseReviewLight, reviewTitle, "Review Title");
    expect_string(nbgl_useCaseReviewLight, reviewSubTitle, "Review Sub-title");
    expect_string(nbgl_useCaseReviewLight, finishTitle, "Finish Title");

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_reviewLight(TYPE_TRANSACTION,
                              &g_tagValueList,
                              &g_appIcon,
                              "Review Title",
                              "Review Sub-title",
                              "Finish Title");
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

static void test_address_review(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_string(nbgl_useCaseAddressReview, address, "My address");
    expect_memory(
        nbgl_useCaseAddressReview, additionalTagValueList, &g_tagValueList, sizeof(g_tagValueList));
    expect_memory(nbgl_useCaseAddressReview, icon, &g_appIcon, sizeof(g_appIcon));
    expect_string(nbgl_useCaseAddressReview, reviewTitle, "Review Title");
    expect_string(nbgl_useCaseAddressReview, reviewSubTitle, "Review Sub-title");

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_addressReview(
        "My address", &g_tagValueList, &g_appIcon, "Review Title", "Review Sub-title");
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

static void test_review_status(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_value(nbgl_useCaseReviewStatus, reviewStatusType, STATUS_TYPE_TRANSACTION_SIGNED);

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_reviewStatus(STATUS_TYPE_TRANSACTION_SIGNED);
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

static void test_status(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_string(nbgl_useCaseStatus, message, "My status");
    expect_value(nbgl_useCaseStatus, isSuccess, true);

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_status("My status", true);
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

static void test_choice(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_memory(nbgl_useCaseChoice, icon, &g_appIcon, sizeof(g_appIcon));
    expect_string(nbgl_useCaseChoice, message, "Message");
    expect_string(nbgl_useCaseChoice, subMessage, "Sub Message");
    expect_string(nbgl_useCaseChoice, confirmText, "Confirm");
    expect_string(nbgl_useCaseChoice, cancelText, "Reject");

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_choice(&g_appIcon, "Message", "Sub Message", "Confirm", "Reject");
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

static void test_streaming(void **state __attribute__((unused)))
{
    ux_sync_ret_t ret;

    expect_value(nbgl_useCaseReviewStreamingStart, operationType, TYPE_TRANSACTION);
    expect_memory(nbgl_useCaseReviewStreamingStart, icon, &g_appIcon, sizeof(g_appIcon));
    expect_string(nbgl_useCaseReviewStreamingStart, reviewTitle, "Review Title");
    expect_string(nbgl_useCaseReviewStreamingStart, reviewSubTitle, "Review Sub-title");

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_reviewStreamingStart(
        TYPE_TRANSACTION, &g_appIcon, "Review Title", "Review Sub-title");
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);

    expect_memory(nbgl_useCaseReviewStreamingContinueExt,
                  tagValueList,
                  &g_tagValueList,
                  sizeof(g_tagValueList));

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_reviewStreamingContinue(&g_tagValueList);
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);

    expect_string(nbgl_useCaseReviewStreamingFinish, finishTitle, "Finish Title");

    will_return(io_recv_and_process_event, false);
    ret = ux_sync_reviewStreamingFinish("Finish Title");
    assert_int_equal(ret, UX_SYNC_RET_APPROVED);
}

int main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_home),
                                       cmocka_unit_test(test_review),
                                       cmocka_unit_test(test_review_light),
                                       cmocka_unit_test(test_address_review),
                                       cmocka_unit_test(test_review_status),
                                       cmocka_unit_test(test_status),
                                       cmocka_unit_test(test_choice),
                                       cmocka_unit_test(test_streaming)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
