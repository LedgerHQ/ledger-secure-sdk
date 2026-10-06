#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "nbgl_use_case.h"
#include "nbgl_step.h"
#include "glyphs.h"

// Regression tests for V-129: the Nano use cases count their pages in 8-bit fields. A page
// total that does not fit must be refused, and never let the approval page be reached before
// every review element has been displayed.

#define MAX_ELEMS 255

// ============================================================================
// Glyphs and NBGL stubs
// ============================================================================

const nbgl_icon_details_t C_icon_back_x;
const nbgl_icon_details_t C_icon_coggle;
const nbgl_icon_details_t C_icon_crossmark;
const nbgl_icon_details_t C_icon_down;
const nbgl_icon_details_t C_icon_processing;
const nbgl_icon_details_t C_icon_validate_14;
const nbgl_icon_details_t C_icon_warning;
const nbgl_icon_details_t C_Information_circle_14px;
const nbgl_icon_details_t C_Quit_14px;

// Last drawn step
static int                        nb_draws;
static nbgl_stepButtonCallback_t  last_callback;
static const char                *last_text;
static const char                *last_subtext;
static const nbgl_icon_details_t *last_icon;
static int                        dummy_step;

static void record_step(nbgl_stepButtonCallback_t  callback,
                        const char                *text,
                        const char                *subText,
                        const nbgl_icon_details_t *icon)
{
    nb_draws++;
    last_callback = callback;
    last_text     = text;
    last_subtext  = subText;
    last_icon     = icon;
}

nbgl_step_t nbgl_stepDrawText(nbgl_stepPosition_t               pos,
                              nbgl_stepButtonCallback_t         onActionCallback,
                              nbgl_screenTickerConfiguration_t *ticker,
                              const char                       *text,
                              const char                       *subText,
                              nbgl_contentCenteredInfoStyle_t   style,
                              bool                              modal)
{
    (void) pos;
    (void) ticker;
    (void) style;
    (void) modal;
    record_step(onActionCallback, text, subText, NULL);
    return &dummy_step;
}

nbgl_step_t nbgl_stepDrawCenteredInfo(nbgl_stepPosition_t               pos,
                                      nbgl_stepButtonCallback_t         onActionCallback,
                                      nbgl_screenTickerConfiguration_t *ticker,
                                      nbgl_layoutCenteredInfo_t        *info,
                                      bool                              modal)
{
    (void) pos;
    (void) ticker;
    (void) modal;
    record_step(onActionCallback, info->text1, info->text2, info->icon);
    return &dummy_step;
}

nbgl_step_t nbgl_stepDrawMenuList(nbgl_stepMenuListCallback_t       onActionCallback,
                                  nbgl_screenTickerConfiguration_t *ticker,
                                  nbgl_layoutMenuList_t            *list,
                                  bool                              modal)
{
    (void) onActionCallback;
    (void) ticker;
    (void) list;
    (void) modal;
    record_step(NULL, NULL, NULL, NULL);
    return &dummy_step;
}

nbgl_step_t nbgl_stepDrawSwitch(nbgl_stepPosition_t               pos,
                                nbgl_stepButtonCallback_t         onActionCallback,
                                nbgl_screenTickerConfiguration_t *ticker,
                                nbgl_layoutSwitch_t              *switchInfo,
                                bool                              modal)
{
    (void) pos;
    (void) ticker;
    (void) modal;
    record_step(onActionCallback, switchInfo->text, switchInfo->subText, NULL);
    return &dummy_step;
}

int nbgl_stepRelease(nbgl_step_t step)
{
    (void) step;
    return 0;
}

void nbgl_refresh(void) {}

void nbgl_screenRedraw(void) {}

// ============================================================================
// Test data and callbacks
// ============================================================================

static char                   tag_names[MAX_ELEMS][8];
static nbgl_contentTagValue_t pairs[MAX_ELEMS];
static nbgl_contentSwitch_t   switches[MAX_ELEMS];
static const char            *bar_texts[MAX_ELEMS];

static int  nb_choices;
static bool last_choice;
static int  nb_quits;

static void on_choice(bool confirm)
{
    nb_choices++;
    last_choice = confirm;
}

static void on_quit(void)
{
    nb_quits++;
}

// tag/value list given to the use cases, which keep a pointer on it
static nbgl_contentTagValueList_t list;

static nbgl_contentTagValueList_t make_tag_value_list(uint8_t nbPairs)
{
    nbgl_contentTagValueList_t tagValueList = {0};
    tagValueList.pairs                      = pairs;
    tagValueList.nbPairs                    = nbPairs;
    return tagValueList;
}

// Index of the pair displayed by the last drawn step, or -1 if it is not a pair page
static int displayed_pair(void)
{
    int idx;
    if ((last_text != NULL) && (sscanf(last_text, "tag%d", &idx) == 1)) {
        return idx;
    }
    return -1;
}

static bool approve_displayed(void)
{
    return last_icon == &C_icon_validate_14;
}

static void press(nbgl_buttonEvent_t event)
{
    TEST_ASSERT_NOT_NULL(last_callback);
    last_callback(&dummy_step, event);
}

// Presses the right button until the approval page is displayed, and checks that every pair
// in [0, nbPairs) was displayed before it
static void walk_to_approve_checking_pairs(uint8_t nbPairs)
{
    bool seen[MAX_ELEMS] = {false};

    for (int presses = 0; !approve_displayed(); presses++) {
        int idx = displayed_pair();
        if (idx >= 0) {
            TEST_ASSERT_LESS_THAN(nbPairs, idx);
            seen[idx] = true;
        }
        TEST_ASSERT_LESS_THAN_MESSAGE(MAX_ELEMS + 8, presses, "approval page never reached");
        int draws = nb_draws;
        press(BUTTON_RIGHT_PRESSED);
        TEST_ASSERT_GREATER_THAN_MESSAGE(draws, nb_draws, "navigation blocked");
    }
    for (int i = 0; i < nbPairs; i++) {
        TEST_ASSERT_TRUE_MESSAGE(seen[i], "a pair was not displayed before approval");
    }
}

// Checks that the use case was refused: no page drawn, and only the expected callback called
static void assert_rejected_without_display(void)
{
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
}

void setUp(void)
{
    for (int i = 0; i < MAX_ELEMS; i++) {
        snprintf(tag_names[i], sizeof(tag_names[i]), "tag%d", i);
        memset(&pairs[i], 0, sizeof(pairs[i]));
        pairs[i].item  = tag_names[i];
        pairs[i].value = "value";
        memset(&switches[i], 0, sizeof(switches[i]));
        switches[i].text = tag_names[i];
        bar_texts[i]     = tag_names[i];
    }
    nb_draws      = 0;
    last_callback = NULL;
    last_text     = NULL;
    last_subtext  = NULL;
    last_icon     = NULL;
    nb_choices    = 0;
    last_choice   = false;
    nb_quits      = 0;
}

void tearDown(void) {}

// ============================================================================
// nbgl_useCaseReview: 1 title page (+ 1 subtitle page) + pairs + approve/reject
// ============================================================================

static void review(uint8_t nbPairs, const char *subTitle)
{
    list = make_tag_value_list(nbPairs);
    nbgl_useCaseReview(
        TYPE_TRANSACTION, &list, &C_icon_warning, "Review", subTitle, NULL, on_choice);
}

static void test_review_overflow_is_rejected(void)
{
    // 255 + 3 used to wrap to 2: approval was the very first page
    review(255, NULL);
    assert_rejected_without_display();
}

static void test_review_overflow_with_subtitle_is_rejected(void)
{
    review(252, "sub");
    assert_rejected_without_display();
}

static void test_review_first_overflowing_count_is_rejected(void)
{
    review(253, NULL);
    assert_rejected_without_display();
}

static void test_review_max_pairs_displays_all_before_approve(void)
{
    review(252, NULL);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    walk_to_approve_checking_pairs(252);
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_TRUE(last_choice);
}

static void test_review_max_pairs_with_subtitle_displays_all_before_approve(void)
{
    review(251, "sub");
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    walk_to_approve_checking_pairs(251);
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_TRUE(last_choice);
}

// ============================================================================
// nbgl_useCaseAddressReview: 1 title page (+ 1 subtitle page) + address + pairs + approve/reject
// ============================================================================

static void address_review(uint8_t nbPairs, const char *subTitle)
{
    list = make_tag_value_list(nbPairs);
    nbgl_useCaseAddressReview("0xADDRESS", &list, &C_icon_warning, "Verify", subTitle, on_choice);
}

static void test_address_review_overflow_is_rejected(void)
{
    // 255 + 4 used to wrap to 3: approval was displayed before the address
    address_review(255, NULL);
    assert_rejected_without_display();
}

static void test_address_review_first_overflowing_counts_are_rejected(void)
{
    address_review(252, NULL);
    assert_rejected_without_display();
    setUp();
    address_review(251, "sub");
    assert_rejected_without_display();
}

static void test_address_review_max_pairs_displays_address_and_pairs_before_approve(void)
{
    bool address_seen = false;

    address_review(251, NULL);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    // the address page comes before any pair
    while (displayed_pair() < 0 && !approve_displayed()) {
        if ((last_subtext != NULL) && (strcmp(last_subtext, "0xADDRESS") == 0)) {
            address_seen = true;
        }
        press(BUTTON_RIGHT_PRESSED);
    }
    TEST_ASSERT_TRUE(address_seen);
    walk_to_approve_checking_pairs(251);
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_TRUE(last_choice);
}

// ============================================================================
// nbgl_useCaseReviewStreamingContinue: pairs + 1 page to request more data
// ============================================================================

static void test_streaming_continue_overflow_is_rejected(void)
{
    // 255 + 1 used to wrap to 0: the right button was ignored, blocking the review
    list = make_tag_value_list(255);
    nbgl_useCaseReviewStreamingContinue(&list, on_choice);
    assert_rejected_without_display();
}

static void test_streaming_continue_max_pairs_displays_all_then_continues(void)
{
    list = make_tag_value_list(254);
    nbgl_useCaseReviewStreamingContinue(&list, on_choice);

    for (int i = 0; i < 254; i++) {
        TEST_ASSERT_EQUAL_INT(0, nb_choices);
        TEST_ASSERT_EQUAL_INT(i, displayed_pair());
        press(BUTTON_RIGHT_PRESSED);
    }
    // pressing right on the last pair requests more data
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_TRUE(last_choice);
}

// ============================================================================
// nbgl_useCaseChoiceWithDetails: intro pages + bar pages + confirm/cancel
// ============================================================================

static void choice_with_bars(uint8_t nbBars)
{
    static nbgl_genericDetails_t details;
    memset(&details, 0, sizeof(details));
    details.type           = BAR_LIST_WARNING;
    details.barList.nbBars = nbBars;
    details.barList.texts  = bar_texts;
    // icon + message + subMessage: 2 intro pages
    nbgl_useCaseChoiceWithDetails(
        &C_icon_warning, "Message", "Sub message", "Confirm", "Cancel", &details, on_choice);
}

static void test_choice_overflow_is_rejected(void)
{
    choice_with_bars(255);
    assert_rejected_without_display();
}

static void test_choice_first_overflowing_count_is_rejected(void)
{
    choice_with_bars(252);
    assert_rejected_without_display();
}

static void test_choice_max_bars_reaches_confirm_and_cancel(void)
{
    choice_with_bars(251);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    walk_to_approve_checking_pairs(251);
    TEST_ASSERT_EQUAL_STRING("Confirm", last_text);
    // the cancel page is the last one, right after confirm
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_PTR(&C_icon_crossmark, last_icon);
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
}

// ============================================================================
// nbgl_useCaseGenericReview: contents pages + 1 quit/reject page
// ============================================================================

static void test_generic_review_tag_value_confirm_overflow_is_rejected(void)
{
    // nbPairs + 1 used to wrap to 0 in getContentNbElement(): the content was skipped
    nbgl_content_t content                       = {0};
    content.type                                 = TAG_VALUE_CONFIRM;
    content.content.tagValueConfirm.tagValueList = make_tag_value_list(255);
    nbgl_genericContents_t contents              = {0};
    contents.contentsList                        = &content;
    contents.nbContents                          = 1;

    nbgl_useCaseGenericReview(&contents, "Reject", on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_quits);
}

static void test_generic_review_contents_overflow_is_rejected(void)
{
    nbgl_content_t content[2]       = {0};
    content[0].type                 = TAG_VALUE_LIST;
    content[0].content.tagValueList = make_tag_value_list(200);
    content[1].type                 = TAG_VALUE_LIST;
    content[1].content.tagValueList = make_tag_value_list(200);
    nbgl_genericContents_t contents = {0};
    contents.contentsList           = content;
    contents.nbContents             = 2;

    nbgl_useCaseGenericReview(&contents, "Reject", on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_quits);
}

static void test_generic_review_max_pairs_is_displayed(void)
{
    // 253 pairs + confirm + quit = 255 pages
    nbgl_content_t content                       = {0};
    content.type                                 = TAG_VALUE_CONFIRM;
    content.content.tagValueConfirm.tagValueList = make_tag_value_list(253);
    nbgl_genericContents_t contents              = {0};
    contents.contentsList                        = &content;
    contents.nbContents                          = 1;

    nbgl_useCaseGenericReview(&contents, "Reject", on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_quits);
    TEST_ASSERT_EQUAL_INT(0, displayed_pair());
    for (int i = 1; i < 253; i++) {
        press(BUTTON_RIGHT_PRESSED);
        TEST_ASSERT_EQUAL_INT(i, displayed_pair());
    }
}

// ============================================================================
// nbgl_useCaseNavigableContent: contents retrieved through navCallback + 1 quit page
// ============================================================================

static bool nav_callback(uint8_t page, nbgl_pageContent_t *content)
{
    (void) page;
    content->type         = TAG_VALUE_LIST;
    content->tagValueList = make_tag_value_list(200);
    return true;
}

static void test_navigable_content_overflow_calls_quit(void)
{
    // the quit callback must be taken from the content context, not from the home one
    nbgl_useCaseNavigableContent("Title", 0, 2, on_quit, nav_callback, NULL);
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_quits);
}

// ============================================================================
// Settings: contents pages + 1 back page
// ============================================================================

static void make_switches_contents(nbgl_content_t         *content,
                                   nbgl_genericContents_t *contents,
                                   uint8_t                 nbContents,
                                   uint8_t                 nbSwitches)
{
    for (uint8_t i = 0; i < nbContents; i++) {
        memset(&content[i], 0, sizeof(content[i]));
        content[i].type                            = SWITCHES_LIST;
        content[i].content.switchesList.switches   = switches;
        content[i].content.switchesList.nbSwitches = nbSwitches;
    }
    memset(contents, 0, sizeof(*contents));
    contents->contentsList = content;
    contents->nbContents   = nbContents;
}

static void test_generic_settings_overflow_calls_quit(void)
{
    nbgl_content_t         content[2];
    nbgl_genericContents_t contents;
    make_switches_contents(content, &contents, 2, 200);

    nbgl_useCaseGenericSettings("App", 0, &contents, NULL, on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_quits);
}

static void test_generic_settings_max_switches_is_displayed(void)
{
    // 254 switches + back = 255 pages
    nbgl_content_t         content[1];
    nbgl_genericContents_t contents;
    make_switches_contents(content, &contents, 1, 254);

    nbgl_useCaseGenericSettings("App", 0, &contents, NULL, on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_quits);
    TEST_ASSERT_EQUAL_STRING("tag0", last_text);
    for (int i = 1; i < 254; i++) {
        press(BUTTON_RIGHT_PRESSED);
        TEST_ASSERT_EQUAL_STRING(tag_names[i], last_text);
    }
}

static void test_home_settings_overflow_goes_back_home(void)
{
    nbgl_content_t         content[2];
    nbgl_genericContents_t contents;
    make_switches_contents(content, &contents, 2, 200);

    nbgl_useCaseHomeAndSettings("App", &C_icon_warning, NULL, 0, &contents, NULL, NULL, on_quit);
    // the home flow is displayed again on its "App settings" entry, without quitting the app
    TEST_ASSERT_EQUAL_INT(0, nb_quits);
    TEST_ASSERT_EQUAL_INT(1, nb_draws);
    TEST_ASSERT_EQUAL_PTR(&C_icon_coggle, last_icon);
    TEST_ASSERT_EQUAL_STRING("App settings", last_text);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_review_overflow_is_rejected);
    RUN_TEST(test_review_overflow_with_subtitle_is_rejected);
    RUN_TEST(test_review_first_overflowing_count_is_rejected);
    RUN_TEST(test_review_max_pairs_displays_all_before_approve);
    RUN_TEST(test_review_max_pairs_with_subtitle_displays_all_before_approve);
    RUN_TEST(test_address_review_overflow_is_rejected);
    RUN_TEST(test_address_review_first_overflowing_counts_are_rejected);
    RUN_TEST(test_address_review_max_pairs_displays_address_and_pairs_before_approve);
    RUN_TEST(test_streaming_continue_overflow_is_rejected);
    RUN_TEST(test_streaming_continue_max_pairs_displays_all_then_continues);
    RUN_TEST(test_choice_overflow_is_rejected);
    RUN_TEST(test_choice_first_overflowing_count_is_rejected);
    RUN_TEST(test_choice_max_bars_reaches_confirm_and_cancel);
    RUN_TEST(test_generic_review_tag_value_confirm_overflow_is_rejected);
    RUN_TEST(test_generic_review_contents_overflow_is_rejected);
    RUN_TEST(test_generic_review_max_pairs_is_displayed);
    RUN_TEST(test_navigable_content_overflow_calls_quit);
    RUN_TEST(test_generic_settings_overflow_calls_quit);
    RUN_TEST(test_generic_settings_max_switches_is_displayed);
    RUN_TEST(test_home_settings_overflow_goes_back_home);
    return UNITY_END();
}
