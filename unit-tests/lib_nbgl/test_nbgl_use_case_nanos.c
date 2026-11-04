#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "nbgl_use_case.h"
#include "nbgl_step.h"
#include "nbgl_step_internal.h"
#include "glyphs.h"

// Regression tests for V-129: the Nano use cases count their pages in 8-bit fields. A page
// total that does not fit must be refused, and never let the approval page be reached before
// every review element has been displayed.
// Regression tests for V-127: the keyboard must never write beyond entryBuffer[entryMaxLen],
// even when the suggestions callback leaves letter keys enabled.

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

// Max number of lines and style given to the text step of each pair (by index), or
// NOT_DRAWN_AS_TEXT if the pair was not drawn with stepDrawText()
#define NOT_DRAWN_AS_TEXT 0xFF
static uint8_t                         pair_max_lines[MAX_ELEMS];
static nbgl_contentCenteredInfoStyle_t pair_style[MAX_ELEMS];

static int displayed_pair(void);

nbgl_step_t stepDrawText(nbgl_stepPosition_t               pos,
                         nbgl_stepButtonCallback_t         onActionCallback,
                         nbgl_screenTickerConfiguration_t *ticker,
                         const char                       *text,
                         const char                       *subText,
                         nbgl_contentCenteredInfoStyle_t   style,
                         bool                              modal,
                         uint8_t                           nbMaxLinesForSubText)
{
    (void) pos;
    (void) ticker;
    (void) modal;
    record_step(onActionCallback, text, subText, NULL);
    int idx = displayed_pair();
    if (idx >= 0) {
        pair_max_lines[idx] = nbMaxLinesForSubText;
        pair_style[idx]     = style;
    }
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

// number of screen redraws (done when leaving a modal step)
static int nb_redraws;

void nbgl_screenRedraw(void)
{
    nb_redraws++;
}

// Keyboard layout: memorize the key callback and the last key mask
static int                layout_dummy;
static int                nb_keyboards;
static keyboardCallback_t kbd_callback;
static uint32_t           kbd_mask;

nbgl_layout_t *nbgl_layoutGet(const nbgl_layoutDescription_t *description)
{
    (void) description;
    return (nbgl_layout_t *) &layout_dummy;
}

int nbgl_layoutAddCenteredInfo(nbgl_layout_t *layout, const nbgl_layoutCenteredInfo_t *info)
{
    (void) layout;
    (void) info;
    return 0;
}

int nbgl_layoutAddNavigation(nbgl_layout_t *layout, nbgl_layoutNavigation_t *info)
{
    (void) layout;
    (void) info;
    return 0;
}

int nbgl_layoutAddKeyboard(nbgl_layout_t *layout, const nbgl_layoutKbd_t *kbdInfo)
{
    (void) layout;
    nb_keyboards++;
    kbd_callback = kbdInfo->callback;
    kbd_mask     = kbdInfo->keyMask;
    return 1;
}

int nbgl_layoutUpdateKeyboard(nbgl_layout_t *layout, uint8_t index, uint32_t keyMask)
{
    (void) layout;
    (void) index;
    kbd_mask = keyMask;
    return 0;
}

int nbgl_layoutAddEnteredText(nbgl_layout_t *layout, const char *text, bool lettersOnly)
{
    (void) layout;
    (void) text;
    (void) lettersOnly;
    return 2;
}

int nbgl_layoutUpdateEnteredText(nbgl_layout_t *layout, uint8_t index, const char *text)
{
    (void) layout;
    (void) index;
    (void) text;
    return 0;
}

// Keypad layout: memorize the key callback and the last displayed digits
static int                nb_keypads;
static keyboardCallback_t keypad_callback;
static const char        *keypad_text;
static uint8_t            keypad_nb_active;

// PIN received by the validation callback
static int     nb_pins;
static uint8_t pin[KEYPAD_MAX_DIGITS + 1];
static uint8_t pin_len;

int nbgl_layoutAddKeypad(nbgl_layout_t     *layout,
                         keyboardCallback_t callback,
                         const char        *text,
                         bool               shuffled)
{
    (void) layout;
    (void) text;
    (void) shuffled;
    nb_keypads++;
    keypad_callback = callback;
    return 1;
}

int nbgl_layoutUpdateKeypad(nbgl_layout_t *layout,
                            uint8_t        index,
                            bool           enableValidate,
                            bool           enableBackspace)
{
    (void) layout;
    (void) index;
    (void) enableValidate;
    (void) enableBackspace;
    return 0;
}

int nbgl_layoutAddKeypadContent(nbgl_layout_t *layout,
                                bool           hidden,
                                uint8_t        nbDigits,
                                const char    *text)
{
    (void) layout;
    (void) hidden;
    (void) nbDigits;
    keypad_text = text;
    return 2;
}

int nbgl_layoutUpdateKeypadContent(nbgl_layout_t *layout,
                                   bool           hidden,
                                   uint8_t        nbActiveDigits,
                                   const char    *text)
{
    (void) layout;
    (void) hidden;
    keypad_nb_active = nbActiveDigits;
    keypad_text      = text;
    return 0;
}

int nbgl_layoutDraw(nbgl_layout_t *layout)
{
    (void) layout;
    return 0;
}

int nbgl_layoutRelease(nbgl_layout_t *layout)
{
    (void) layout;
    return 0;
}

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
        switches[i].text  = tag_names[i];
        bar_texts[i]      = tag_names[i];
        pair_max_lines[i] = NOT_DRAWN_AS_TEXT;
        pair_style[i]     = REGULAR_INFO;
    }
    nb_draws         = 0;
    last_callback    = NULL;
    last_text        = NULL;
    last_subtext     = NULL;
    last_icon        = NULL;
    nb_choices       = 0;
    last_choice      = false;
    nb_quits         = 0;
    nb_redraws       = 0;
    nb_keyboards     = 0;
    kbd_callback     = NULL;
    kbd_mask         = 0;
    nb_keypads       = 0;
    keypad_callback  = NULL;
    keypad_text      = NULL;
    keypad_nb_active = 0;
    nb_pins          = 0;
    pin_len          = 0;
    memset(pin, 0, sizeof(pin));
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

// index of the first pair to get with the callback (startIndex of the list)
static uint8_t callback_base;
// index (relative to callback_base) of the pair not provided by the callback, -1 if none
static int missing_pair;
// number of times the missing pair is still provided before the callback returns NULL for it
static int nb_calls_before_missing;

static nbgl_contentTagValue_t *get_pair(uint8_t pairIndex)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(callback_base, pairIndex);
    if ((pairIndex - callback_base) == missing_pair) {
        if (nb_calls_before_missing == 0) {
            return NULL;
        }
        nb_calls_before_missing--;
    }
    return &pairs[pairIndex - callback_base];
}

static nbgl_contentTagValueList_t make_callback_list(uint8_t nbPairs, uint8_t startIndex)
{
    nbgl_contentTagValueList_t callbackList = make_tag_value_list(nbPairs);
    callbackList.pairs                      = NULL;
    callbackList.callback                   = get_pair;
    callbackList.startIndex                 = startIndex;
    callback_base                           = startIndex;
    missing_pair                            = -1;
    nb_calls_before_missing                 = 0;
    return callbackList;
}

static void test_review_with_callback_starts_at_start_index(void)
{
    list = make_callback_list(5, 50);
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    walk_to_approve_checking_pairs(5);
}

static void test_review_with_callback_up_to_uint8_limit_displays_all(void)
{
    // the last pair is retrieved with callback(UINT8_MAX)
    list = make_callback_list(5, UINT8_MAX + 1 - 5);
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    walk_to_approve_checking_pairs(5);
}

static void test_review_with_callback_index_overflow_is_rejected(void)
{
    // the last pair would be retrieved with callback(UINT8_MAX + 1), wrapping to 0
    list = make_callback_list(5, UINT8_MAX + 2 - 5);
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    assert_rejected_without_display();
}

static void test_address_review_with_callback_index_overflow_is_rejected(void)
{
    list = make_callback_list(5, UINT8_MAX + 2 - 5);
    nbgl_useCaseAddressReview("0xADDRESS", &list, &C_icon_warning, "Verify", NULL, on_choice);
    assert_rejected_without_display();
}

static void test_streaming_review_with_callback_index_overflow_is_rejected(void)
{
    list = make_callback_list(5, UINT8_MAX + 2 - 5);
    nbgl_useCaseReviewStreamingContinue(&list, on_choice);
    assert_rejected_without_display();
}

static void test_generic_review_with_callback_index_overflow_is_rejected(void)
{
    nbgl_content_t content                       = {0};
    content.type                                 = TAG_VALUE_CONFIRM;
    content.content.tagValueConfirm.tagValueList = make_callback_list(5, UINT8_MAX + 2 - 5);
    nbgl_genericContents_t contents              = {0};
    contents.contentsList                        = &content;
    contents.nbContents                          = 1;

    nbgl_useCaseGenericReview(&contents, "Reject", on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_quits);
}

static void test_review_with_missing_callback_pair_is_rejected(void)
{
    list         = make_callback_list(5, 50);
    missing_pair = 2;
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    assert_rejected_without_display();
}

static void test_review_without_callback_is_rejected(void)
{
    list          = make_callback_list(5, 50);
    list.callback = NULL;
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    assert_rejected_without_display();
}

static void test_address_review_with_missing_callback_pair_is_rejected(void)
{
    list         = make_callback_list(5, 50);
    missing_pair = 2;
    nbgl_useCaseAddressReview("0xADDRESS", &list, &C_icon_warning, "Verify", NULL, on_choice);
    assert_rejected_without_display();
}

static void test_streaming_review_with_missing_callback_pair_is_rejected(void)
{
    list         = make_callback_list(5, 50);
    missing_pair = 2;
    nbgl_useCaseReviewStreamingContinue(&list, on_choice);
    assert_rejected_without_display();
}

static void test_generic_review_with_missing_callback_pair_is_rejected(void)
{
    nbgl_content_t content                       = {0};
    content.type                                 = TAG_VALUE_CONFIRM;
    content.content.tagValueConfirm.tagValueList = make_callback_list(5, 50);
    nbgl_genericContents_t contents              = {0};
    contents.contentsList                        = &content;
    contents.nbContents                          = 1;
    missing_pair                                 = 2;

    nbgl_useCaseGenericReview(&contents, "Reject", on_quit);
    TEST_ASSERT_EQUAL_INT(0, nb_draws);
    TEST_ASSERT_EQUAL_INT(1, nb_quits);
}

static void test_review_without_list_is_rejected(void)
{
    nbgl_useCaseReview(TYPE_TRANSACTION, NULL, &C_icon_warning, "Review", NULL, NULL, on_choice);
    assert_rejected_without_display();
}

static void test_streaming_review_without_list_is_rejected(void)
{
    nbgl_useCaseReviewStreamingContinue(NULL, on_choice);
    assert_rejected_without_display();
}

static void test_review_with_callback_pair_missing_after_start_shows_empty_pair(void)
{
    bool empty_seen = false;

    // the pair is only provided when the review is started
    list                    = make_callback_list(5, 50);
    missing_pair            = 2;
    nb_calls_before_missing = 1;
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    for (int presses = 0; !approve_displayed(); presses++) {
        TEST_ASSERT_LESS_THAN_MESSAGE(MAX_ELEMS + 8, presses, "approval page never reached");
        press(BUTTON_RIGHT_PRESSED);
        if ((last_text != NULL) && (last_text[0] == '\0')) {
            empty_seen = true;
        }
    }
    TEST_ASSERT_TRUE(empty_seen);
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

// Starts an address review with the given additional list, and checks that the address is
// displayed before the approval page, which can be approved
static void check_address_review_is_approved(const nbgl_contentTagValueList_t *additionalList)
{
    bool address_seen = false;

    nbgl_useCaseAddressReview(
        "0xADDRESS", additionalList, &C_icon_warning, "Verify", NULL, on_choice);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    for (int presses = 0; !approve_displayed(); presses++) {
        TEST_ASSERT_LESS_THAN_MESSAGE(8, presses, "approval page never reached");
        TEST_ASSERT_LESS_THAN(0, displayed_pair());
        if ((last_subtext != NULL) && (strcmp(last_subtext, "0xADDRESS") == 0)) {
            address_seen = true;
        }
        press(BUTTON_RIGHT_PRESSED);
    }
    TEST_ASSERT_TRUE(address_seen);
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_TRUE(last_choice);
}

static void test_address_review_with_empty_list_is_displayed(void)
{
    // a zeroed list (no pairs, no callback), as given by the Rust SDK without extra fields
    memset(&list, 0, sizeof(list));
    check_address_review_is_approved(&list);
}

static void test_address_review_without_list_is_displayed(void)
{
    check_address_review_is_approved(NULL);
}

static void test_address_review_without_callback_is_rejected(void)
{
    list          = make_callback_list(5, 50);
    list.callback = NULL;
    nbgl_useCaseAddressReview("0xADDRESS", &list, &C_icon_warning, "Verify", NULL, on_choice);
    assert_rejected_without_display();
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
// nbMaxLinesForValue of the pairs: forwarded to the text step of the pair
// ============================================================================

static const nbgl_contentValueExt_t alias_extension
    = {.aliasType = ENS_ALIAS, .fullValue = "full value", .title = "title"};

// pair 0: no max, pair 1: regular value with a max, pair 2: alias with a max, pair 3: centered
// info with a max (ignored)
static void set_pairs_with_max_lines(void)
{
    pairs[1].nbMaxLinesForValue = 2;
    pairs[2].nbMaxLinesForValue = 1;
    pairs[2].aliasValue         = 1;
    pairs[2].extension          = &alias_extension;
    pairs[3].nbMaxLinesForValue = 3;
    pairs[3].centeredInfo       = 1;
}

// aliasStyle: style used by the use case for an alias value (generic review doesn't display
// aliases as buttons)
static void check_pairs_with_max_lines_drawn(nbgl_contentCenteredInfoStyle_t aliasStyle)
{
    TEST_ASSERT_EQUAL_UINT8(0, pair_max_lines[0]);
    TEST_ASSERT_EQUAL_INT(BOLD_TEXT1_INFO, pair_style[0]);
    TEST_ASSERT_EQUAL_UINT8(2, pair_max_lines[1]);
    TEST_ASSERT_EQUAL_INT(BOLD_TEXT1_INFO, pair_style[1]);
    TEST_ASSERT_EQUAL_UINT8(1, pair_max_lines[2]);
    TEST_ASSERT_EQUAL_INT(aliasStyle, pair_style[2]);
    // a centered info is not drawn as a text step, so its max is not used
    TEST_ASSERT_EQUAL_UINT8(NOT_DRAWN_AS_TEXT, pair_max_lines[3]);
}

static void test_review_pair_max_lines_is_forwarded(void)
{
    set_pairs_with_max_lines();
    review(4, NULL);
    walk_to_approve_checking_pairs(4);
    check_pairs_with_max_lines_drawn(BUTTON_INFO);
}

static void test_review_with_callback_pair_max_lines_is_forwarded(void)
{
    set_pairs_with_max_lines();
    list = make_callback_list(4, 0);
    nbgl_useCaseReview(TYPE_TRANSACTION, &list, &C_icon_warning, "Review", NULL, NULL, on_choice);
    walk_to_approve_checking_pairs(4);
    check_pairs_with_max_lines_drawn(BUTTON_INFO);
}

static void test_streaming_pair_max_lines_is_forwarded(void)
{
    set_pairs_with_max_lines();
    list = make_tag_value_list(4);
    nbgl_useCaseReviewStreamingContinue(&list, on_choice);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(i, displayed_pair());
        press(BUTTON_RIGHT_PRESSED);
    }
    check_pairs_with_max_lines_drawn(BUTTON_INFO);
}

static void test_generic_review_pair_max_lines_is_forwarded(void)
{
    set_pairs_with_max_lines();
    // pairs 0 and 1 in a TAG_VALUE_LIST, pairs 2 and 3 in a TAG_VALUE_CONFIRM
    nbgl_content_t content[2]                             = {0};
    content[0].type                                       = TAG_VALUE_LIST;
    content[0].content.tagValueList                       = make_tag_value_list(2);
    content[1].type                                       = TAG_VALUE_CONFIRM;
    content[1].content.tagValueConfirm.tagValueList       = make_tag_value_list(2);
    content[1].content.tagValueConfirm.tagValueList.pairs = &pairs[2];
    content[1].content.tagValueConfirm.confirmationText   = "Confirm";
    nbgl_genericContents_t contents                       = {0};
    contents.contentsList                                 = content;
    contents.nbContents                                   = 2;

    nbgl_useCaseGenericReview(&contents, "Reject", on_quit);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(i, displayed_pair());
        press(BUTTON_RIGHT_PRESSED);
    }
    check_pairs_with_max_lines_drawn(BOLD_TEXT1_INFO);
}

static void test_alias_list_pair_max_lines_is_forwarded(void)
{
    static nbgl_contentTagValueList_t aliasList;
    static nbgl_contentValueExt_t     listExtension;

    // the extension of pair 0 is a list of pairs 10 (no max) and 11 (with a max)
    aliasList                    = make_tag_value_list(2);
    aliasList.pairs              = &pairs[10];
    pairs[11].nbMaxLinesForValue = 2;
    memset(&listExtension, 0, sizeof(listExtension));
    listExtension.aliasType    = TAG_VALUE_LIST_ALIAS;
    listExtension.tagValuelist = &aliasList;
    pairs[0].aliasValue        = 1;
    pairs[0].extension         = &listExtension;

    review(1, NULL);
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_INT(0, displayed_pair());
    // display the pairs of the extension
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(10, displayed_pair());
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_INT(11, displayed_pair());
    TEST_ASSERT_EQUAL_UINT8(0, pair_max_lines[10]);
    TEST_ASSERT_EQUAL_UINT8(2, pair_max_lines[11]);
}

// pairs of the alias list using a callback (pairs 10 and 11), retrieved from index 40
#define ALIAS_CALLBACK_START_INDEX 40

static nbgl_contentTagValue_t *get_alias_pair(uint8_t pairIndex)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(ALIAS_CALLBACK_START_INDEX, pairIndex);
    TEST_ASSERT_LESS_THAN_UINT8(ALIAS_CALLBACK_START_INDEX + 2, pairIndex);
    return &pairs[10 + pairIndex - ALIAS_CALLBACK_START_INDEX];
}

static void test_alias_list_with_callback_is_displayed(void)
{
    static nbgl_contentTagValueList_t aliasList;
    static nbgl_contentValueExt_t     listExtension;

    // the extension of pair 0 is a list using a callback, giving pairs 10 (no max) and 11 (with
    // a max), from its startIndex
    aliasList                    = make_tag_value_list(2);
    aliasList.pairs              = NULL;
    aliasList.callback           = get_alias_pair;
    aliasList.startIndex         = ALIAS_CALLBACK_START_INDEX;
    pairs[11].nbMaxLinesForValue = 2;
    memset(&listExtension, 0, sizeof(listExtension));
    listExtension.aliasType    = TAG_VALUE_LIST_ALIAS;
    listExtension.tagValuelist = &aliasList;
    pairs[0].aliasValue        = 1;
    pairs[0].extension         = &listExtension;

    review(1, NULL);
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_INT(0, displayed_pair());
    // display the pairs of the extension
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(10, displayed_pair());
    TEST_ASSERT_EQUAL_PTR(pairs[10].value, last_subtext);
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_INT(11, displayed_pair());
    TEST_ASSERT_EQUAL_PTR(pairs[11].value, last_subtext);
    TEST_ASSERT_EQUAL_UINT8(0, pair_max_lines[10]);
    TEST_ASSERT_EQUAL_UINT8(2, pair_max_lines[11]);
    // then the back page
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_PTR(&C_icon_back_x, last_icon);
}

// ============================================================================
// Aliases in streaming review: their full value must be reachable (V-281)
// ============================================================================

#define MAX_EXTENSION_PAGES 8

// Opens the full value of the displayed alias pair (both buttons), records the sub-texts (or
// texts if no sub-text) of the extension pages until the back page, then leaves the modal and
// restores the review step (nbgl_screenRedraw() is a stub here). Returns the number of pages
// before the back page.
static int open_alias_and_collect(const char *collected[MAX_EXTENSION_PAGES])
{
    // review step, redrawn when leaving the modal
    nbgl_stepButtonCallback_t  reviewCallback = last_callback;
    const char                *reviewText     = last_text;
    const char                *reviewSubText  = last_subtext;
    const nbgl_icon_details_t *reviewIcon     = last_icon;
    int                        pairIdx        = displayed_pair();
    int                        nbPages        = 0;
    int                        draws          = nb_draws;

    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, pairIdx);
    TEST_ASSERT_EQUAL_INT(BUTTON_INFO, pair_style[pairIdx]);
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_GREATER_THAN_MESSAGE(draws, nb_draws, "alias full value cannot be opened");
    while (last_icon != &C_icon_back_x) {
        TEST_ASSERT_LESS_THAN_MESSAGE(MAX_EXTENSION_PAGES, nbPages, "back page never reached");
        collected[nbPages++] = (last_subtext != NULL) ? last_subtext : last_text;
        press(BUTTON_RIGHT_PRESSED);
    }
    // leave the modal
    int redraws = nb_redraws;
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(redraws + 1, nb_redraws);
    record_step(reviewCallback, reviewText, reviewSubText, reviewIcon);
    return nbPages;
}

static void test_streaming_alias_full_values_are_reachable(void)
{
    static nbgl_contentValueExt_t     ens;
    static nbgl_contentValueExt_t     addressBook;
    static nbgl_contentValueExt_t     infoListExt;
    static nbgl_contentValueExt_t     tagValueListExt;
    static nbgl_contentInfoList_t     infoList;
    static nbgl_contentTagValueList_t aliasList;
    static const char *const          infoTypes[]    = {"type0", "type1"};
    static const char *const          infoContents[] = {"content0", "content1"};
    const char                       *collected[MAX_EXTENSION_PAGES];

    // pair 0: regular, 1: ENS, 2: address book (with scope and explanation), 3: info list,
    // 4: tag/value list (pairs 10 and 11)
    memset(&ens, 0, sizeof(ens));
    ens.aliasType = ENS_ALIAS;
    ens.title     = "ENS";
    ens.fullValue = "0xENS";

    memset(&addressBook, 0, sizeof(addressBook));
    addressBook.aliasType    = ADDRESS_BOOK_ALIAS;
    addressBook.title        = "Contact";
    addressBook.fullValue    = "0xCONTACT";
    addressBook.aliasSubName = "scope";
    addressBook.explanation  = "explanation";

    memset(&infoList, 0, sizeof(infoList));
    infoList.nbInfos      = 2;
    infoList.infoTypes    = infoTypes;
    infoList.infoContents = infoContents;
    memset(&infoListExt, 0, sizeof(infoListExt));
    infoListExt.aliasType = INFO_LIST_ALIAS;
    infoListExt.infolist  = &infoList;

    aliasList       = make_tag_value_list(2);
    aliasList.pairs = &pairs[10];
    pairs[10].value = "list value 10";
    pairs[11].value = "list value 11";
    memset(&tagValueListExt, 0, sizeof(tagValueListExt));
    tagValueListExt.aliasType    = TAG_VALUE_LIST_ALIAS;
    tagValueListExt.tagValuelist = &aliasList;

    pairs[1].aliasValue = 1;
    pairs[1].extension  = &ens;
    pairs[2].aliasValue = 1;
    pairs[2].extension  = &addressBook;
    pairs[3].aliasValue = 1;
    pairs[3].extension  = &infoListExt;
    pairs[4].aliasValue = 1;
    pairs[4].extension  = &tagValueListExt;

    list = make_tag_value_list(5);
    nbgl_useCaseReviewStreamingContinue(&list, on_choice);

    // regular pair: both buttons do nothing
    TEST_ASSERT_EQUAL_INT(0, displayed_pair());
    int draws = nb_draws;
    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_INT(draws, nb_draws);
    press(BUTTON_RIGHT_PRESSED);

    TEST_ASSERT_EQUAL_INT(1, displayed_pair());
    TEST_ASSERT_EQUAL_INT(1, open_alias_and_collect(collected));
    TEST_ASSERT_EQUAL_STRING("0xENS", collected[0]);
    // the review is resumed on the same pair, and can be opened again
    TEST_ASSERT_EQUAL_INT(1, open_alias_and_collect(collected));
    TEST_ASSERT_EQUAL_STRING("0xENS", collected[0]);
    press(BUTTON_RIGHT_PRESSED);

    TEST_ASSERT_EQUAL_INT(2, displayed_pair());
    TEST_ASSERT_EQUAL_INT(3, open_alias_and_collect(collected));
    TEST_ASSERT_EQUAL_STRING("scope", collected[0]);
    TEST_ASSERT_EQUAL_STRING("0xCONTACT", collected[1]);
    TEST_ASSERT_EQUAL_STRING("explanation", collected[2]);
    press(BUTTON_RIGHT_PRESSED);

    TEST_ASSERT_EQUAL_INT(3, displayed_pair());
    TEST_ASSERT_EQUAL_INT(2, open_alias_and_collect(collected));
    TEST_ASSERT_EQUAL_STRING("content0", collected[0]);
    TEST_ASSERT_EQUAL_STRING("content1", collected[1]);
    press(BUTTON_RIGHT_PRESSED);

    TEST_ASSERT_EQUAL_INT(4, displayed_pair());
    TEST_ASSERT_EQUAL_INT(2, open_alias_and_collect(collected));
    TEST_ASSERT_EQUAL_STRING("list value 10", collected[0]);
    TEST_ASSERT_EQUAL_STRING("list value 11", collected[1]);

    // going back from the last alias displays the previous one, still openable
    press(BUTTON_LEFT_PRESSED);
    TEST_ASSERT_EQUAL_INT(3, displayed_pair());
    TEST_ASSERT_EQUAL_INT(2, open_alias_and_collect(collected));
    TEST_ASSERT_EQUAL_STRING("content0", collected[0]);
    press(BUTTON_RIGHT_PRESSED);
    TEST_ASSERT_EQUAL_INT(4, displayed_pair());

    // no choice made while reviewing aliases, then right on the last pair requests more data
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    press(BUTTON_RIGHT_PRESSED);
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

// ============================================================================
// Keyboard
// ============================================================================

#define ENTRY_MAX_LEN 8
#define CANARY        0xA5

// entry buffer of entryMaxLen + 1 bytes, immediately followed by a canary
static struct {
    char    buffer[ENTRY_MAX_LEN + 1];
    uint8_t canary[8];
} entry;

static void on_keyboard_action(void) {}

static void on_keyboard_back(void) {}

// suggestions callback which never masks any key nor proposes any word
static void permissive_suggestions(nbgl_layoutKeyboardContent_t *content, uint32_t *mask)
{
    content->suggestionButtons.nbUsedButtons = 0;
    content->suggestionButtons.nbCandidates  = 0;
    *mask                                    = 0;
}

static void start_keyboard(nbgl_layoutKeyboardContentType_t type)
{
    static const char    *buttons[NB_MAX_SUGGESTION_BUTTONS];
    nbgl_keyboardParams_t params = {
        .type        = type,
        .title       = "Enter text",
        .entryBuffer = entry.buffer,
        .entryMaxLen = ENTRY_MAX_LEN,
        .mode        = MODE_LOWER_LETTERS,
        .lettersOnly = true,
    };
    if (type == KEYBOARD_WITH_BUTTON) {
        params.confirmationParams.onButtonCallback = on_keyboard_action;
    }
    else {
        params.suggestionParams.buttons               = buttons;
        params.suggestionParams.updateButtonsCallback = permissive_suggestions;
    }
    memset(&entry, CANARY, sizeof(entry));
    nbgl_useCaseKeyboard(&params, on_keyboard_back);
    TEST_ASSERT_NOT_NULL(kbd_callback);
}

static void assert_entry_full_and_canary_intact(void)
{
    TEST_ASSERT_EQUAL_size_t(ENTRY_MAX_LEN, strlen(entry.buffer));
    TEST_ASSERT_EACH_EQUAL_HEX8(CANARY, entry.canary, sizeof(entry.canary));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, kbd_mask);
}

static void test_keyboard_with_button_stops_at_max_len(void)
{
    start_keyboard(KEYBOARD_WITH_BUTTON);
    for (int i = 0; i < 2 * ENTRY_MAX_LEN; i++) {
        kbd_callback('a');
    }
    assert_entry_full_and_canary_intact();
}

static void test_keyboard_with_suggestions_stops_at_max_len(void)
{
    start_keyboard(KEYBOARD_WITH_SUGGESTIONS);
    for (int i = 0; i < 2 * ENTRY_MAX_LEN; i++) {
        kbd_callback('a');
    }
    assert_entry_full_and_canary_intact();
    // removing a character re-enables the letters
    kbd_callback(BACKSPACE_KEY);
    TEST_ASSERT_EQUAL_size_t(ENTRY_MAX_LEN - 1, strlen(entry.buffer));
    TEST_ASSERT_EQUAL_HEX32(0, kbd_mask);
}

static void test_keyboard_without_buffer_is_rejected(void)
{
    nbgl_keyboardParams_t params = {
        .type                                = KEYBOARD_WITH_BUTTON,
        .entryBuffer                         = NULL,
        .entryMaxLen                         = ENTRY_MAX_LEN,
        .confirmationParams.onButtonCallback = on_keyboard_action,
    };
    nbgl_useCaseKeyboard(&params, on_keyboard_back);
    TEST_ASSERT_EQUAL_INT(0, nb_keyboards);
}

static void test_keyboard_with_zero_max_len_is_rejected(void)
{
    nbgl_keyboardParams_t params = {
        .type                                = KEYBOARD_WITH_BUTTON,
        .entryBuffer                         = entry.buffer,
        .entryMaxLen                         = 0,
        .confirmationParams.onButtonCallback = on_keyboard_action,
    };
    memset(&entry, CANARY, sizeof(entry));
    nbgl_useCaseKeyboard(&params, on_keyboard_back);
    TEST_ASSERT_EQUAL_INT(0, nb_keyboards);
    TEST_ASSERT_EQUAL_HEX8(CANARY, (uint8_t) entry.buffer[0]);
}

// ============================================================================
// Keypad
// ============================================================================

static int nb_keypad_backs;

static void on_pin_validate(const uint8_t *content, uint8_t length)
{
    nb_pins++;
    pin_len = length;
    memcpy(pin, content, (length < sizeof(pin)) ? length : sizeof(pin));
}

static void on_keypad_back(void)
{
    nb_keypad_backs++;
}

static void fill_keypad(bool hidden)
{
    nb_keypad_backs = 0;
    nbgl_useCaseKeypad("PIN", 4, KEYPAD_MAX_DIGITS, false, hidden, on_pin_validate, on_keypad_back);
    TEST_ASSERT_EQUAL_INT(1, nb_keypads);
    TEST_ASSERT_NOT_NULL(keypad_callback);
    for (int i = 0; i < 2 * KEYPAD_MAX_DIGITS; i++) {
        keypad_callback('0' + (i % 10));
    }
}

static void test_keypad_max_digits_stops_at_max(void)
{
    fill_keypad(true);
    TEST_ASSERT_EQUAL_UINT8(KEYPAD_MAX_DIGITS, keypad_nb_active);
    keypad_callback(VALIDATE_KEY);
    TEST_ASSERT_EQUAL_INT(1, nb_pins);
    TEST_ASSERT_EQUAL_UINT8(KEYPAD_MAX_DIGITS, pin_len);
    TEST_ASSERT_EQUAL_MEMORY("012345678901", pin, KEYPAD_MAX_DIGITS);
}

static void test_keypad_visible_digits_are_nul_terminated_at_max(void)
{
    fill_keypad(false);
    TEST_ASSERT_NOT_NULL(keypad_text);
    TEST_ASSERT_EQUAL_STRING("012345678901", keypad_text);
}

static void test_keypad_backspace_after_max_empties_pin(void)
{
    fill_keypad(false);
    for (int i = 0; i < KEYPAD_MAX_DIGITS; i++) {
        keypad_callback(BACKSPACE_KEY);
    }
    TEST_ASSERT_EQUAL_UINT8(0, keypad_nb_active);
    TEST_ASSERT_EQUAL_STRING("", keypad_text);
    TEST_ASSERT_EQUAL_INT(0, nb_keypad_backs);
    // an extra backspace on an empty PIN calls the back callback
    keypad_callback(BACKSPACE_KEY);
    TEST_ASSERT_EQUAL_INT(1, nb_keypad_backs);
}

static void test_keypad_too_many_max_digits_is_rejected(void)
{
    nbgl_useCaseKeypad(
        "PIN", 4, KEYPAD_MAX_DIGITS + 1, false, true, on_pin_validate, on_keypad_back);
    TEST_ASSERT_EQUAL_INT(0, nb_keypads);
}

static void test_keypad_too_many_min_digits_is_rejected(void)
{
    nbgl_useCaseKeypad("PIN",
                       KEYPAD_MAX_DIGITS + 1,
                       KEYPAD_MAX_DIGITS,
                       false,
                       true,
                       on_pin_validate,
                       on_keypad_back);
    TEST_ASSERT_EQUAL_INT(0, nb_keypads);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_review_overflow_is_rejected);
    RUN_TEST(test_review_overflow_with_subtitle_is_rejected);
    RUN_TEST(test_review_first_overflowing_count_is_rejected);
    RUN_TEST(test_review_max_pairs_displays_all_before_approve);
    RUN_TEST(test_review_max_pairs_with_subtitle_displays_all_before_approve);
    RUN_TEST(test_review_with_callback_starts_at_start_index);
    RUN_TEST(test_review_with_callback_up_to_uint8_limit_displays_all);
    RUN_TEST(test_review_with_callback_index_overflow_is_rejected);
    RUN_TEST(test_address_review_with_callback_index_overflow_is_rejected);
    RUN_TEST(test_streaming_review_with_callback_index_overflow_is_rejected);
    RUN_TEST(test_generic_review_with_callback_index_overflow_is_rejected);
    RUN_TEST(test_review_with_missing_callback_pair_is_rejected);
    RUN_TEST(test_review_without_callback_is_rejected);
    RUN_TEST(test_address_review_with_missing_callback_pair_is_rejected);
    RUN_TEST(test_streaming_review_with_missing_callback_pair_is_rejected);
    RUN_TEST(test_generic_review_with_missing_callback_pair_is_rejected);
    RUN_TEST(test_review_with_callback_pair_missing_after_start_shows_empty_pair);
    RUN_TEST(test_review_without_list_is_rejected);
    RUN_TEST(test_streaming_review_without_list_is_rejected);
    RUN_TEST(test_address_review_overflow_is_rejected);
    RUN_TEST(test_address_review_first_overflowing_counts_are_rejected);
    RUN_TEST(test_address_review_max_pairs_displays_address_and_pairs_before_approve);
    RUN_TEST(test_address_review_with_empty_list_is_displayed);
    RUN_TEST(test_address_review_without_list_is_displayed);
    RUN_TEST(test_address_review_without_callback_is_rejected);
    RUN_TEST(test_streaming_continue_overflow_is_rejected);
    RUN_TEST(test_streaming_continue_max_pairs_displays_all_then_continues);
    RUN_TEST(test_review_pair_max_lines_is_forwarded);
    RUN_TEST(test_review_with_callback_pair_max_lines_is_forwarded);
    RUN_TEST(test_streaming_pair_max_lines_is_forwarded);
    RUN_TEST(test_generic_review_pair_max_lines_is_forwarded);
    RUN_TEST(test_alias_list_pair_max_lines_is_forwarded);
    RUN_TEST(test_alias_list_with_callback_is_displayed);
    RUN_TEST(test_streaming_alias_full_values_are_reachable);
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
    RUN_TEST(test_keyboard_with_button_stops_at_max_len);
    RUN_TEST(test_keyboard_with_suggestions_stops_at_max_len);
    RUN_TEST(test_keyboard_without_buffer_is_rejected);
    RUN_TEST(test_keyboard_with_zero_max_len_is_rejected);
    RUN_TEST(test_keypad_max_digits_stops_at_max);
    RUN_TEST(test_keypad_visible_digits_are_nul_terminated_at_max);
    RUN_TEST(test_keypad_backspace_after_max_empties_pin);
    RUN_TEST(test_keypad_too_many_max_digits_is_rejected);
    RUN_TEST(test_keypad_too_many_min_digits_is_rejected);
    return UNITY_END();
}
