#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"

#include "nbgl_use_case.h"
#include "nbgl_page.h"
#include "os_io_seph_cmd.h"
#include "glyphs.h"

// Regression tests for V-149: when the value alias of a pair of a multi-page TAG_VALUE_CONFIRM is
// touched on a page other than the first one, the details shown in the modal must be the ones of
// the touched pair (and not the ones of a pair further in the list, or out of the list).
// Regression tests for V-293: an advanced review with a custom warning only made of the info field
// must display this warning page, and the review must only start once the user accepts it.
// Regression tests for V-295: an advanced review with custom review details must display a
// top-right button with the given icon on first and last pages, opening these review details.
// When a predefined warning is also given, it takes precedence over the custom fields.

// number of pairs of the TAG_VALUE_CONFIRM list
#define NB_PAIRS            12
#define CONFIRMATION_TOKEN  0x31
// height of every text, giving a realistic number of pairs per page
#define TEXT_HEIGHT         32
#define CANCEL_TOKEN        0x72
#define NO_MODAL_TEXT_DRAWN NULL

// ============================================================================
// Glyphs
// ============================================================================

const nbgl_icon_details_t C_Check_Circle_64px;
const nbgl_icon_details_t C_Chevron_40px;
const nbgl_icon_details_t C_Denied_Circle_64px;
const nbgl_icon_details_t C_Important_Circle_64px;
const nbgl_icon_details_t C_Info_40px;
const nbgl_icon_details_t C_Privacy_40px;
const nbgl_icon_details_t C_QRCode_40px;
const nbgl_icon_details_t C_Search_40px;
const nbgl_icon_details_t C_Warning_40px;
const nbgl_icon_details_t C_Warning_64px;

// ============================================================================
// NBGL page stubs: memorize the last drawn generic page
// ============================================================================

static nbgl_layoutTouchCallback_t page_callback;
static nbgl_pageNavigationInfo_t  page_nav;
static nbgl_pageContent_t         page_content;
static int                        page_dummy;

nbgl_page_t *nbgl_pageDrawGenericContent(nbgl_layoutTouchCallback_t       onActionCallback,
                                         const nbgl_pageNavigationInfo_t *nav,
                                         nbgl_pageContent_t              *content)
{
    page_callback = onActionCallback;
    memcpy(&page_nav, nav, sizeof(page_nav));
    memcpy(&page_content, content, sizeof(page_content));
    return (nbgl_page_t *) &page_dummy;
}

int nbgl_pageRelease(nbgl_page_t *page)
{
    (void) page;
    return 0;
}

nbgl_page_t *nbgl_pageDrawGenericContentExt(nbgl_layoutTouchCallback_t       onActionCallback,
                                            const nbgl_pageNavigationInfo_t *nav,
                                            nbgl_pageContent_t              *content,
                                            bool                             modal)
{
    (void) onActionCallback;
    (void) nav;
    (void) content;
    (void) modal;
    TEST_FAIL_MESSAGE("unexpected call to nbgl_pageDrawGenericContentExt");
    return NULL;
}

nbgl_page_t *nbgl_pageDrawConfirmation(nbgl_layoutTouchCallback_t                onActionCallback,
                                       const nbgl_pageConfirmationDescription_t *info)
{
    (void) onActionCallback;
    (void) info;
    TEST_FAIL_MESSAGE("unexpected call to nbgl_pageDrawConfirmation");
    return NULL;
}

nbgl_page_t *nbgl_pageDrawInfo(nbgl_layoutTouchCallback_t              onActionCallback,
                               const nbgl_screenTickerConfiguration_t *ticker,
                               const nbgl_pageInfoDescription_t       *info)
{
    (void) onActionCallback;
    (void) ticker;
    (void) info;
    TEST_FAIL_MESSAGE("unexpected call to nbgl_pageDrawInfo");
    return NULL;
}

// ============================================================================
// NBGL layout stubs: memorize the text content of the alias modal and the content of the warning
// page
// ============================================================================

static int                         layout_dummy;
static const char                 *modal_title;
static const char                 *modal_full_value;
static nbgl_layoutTouchCallback_t  layout_callback;
static const nbgl_contentCenter_t *warning_center;
static uint8_t                     warning_choice_token;
static bool                        warning_choice_drawn;

nbgl_layout_t *nbgl_layoutGet(const nbgl_layoutDescription_t *description)
{
    layout_callback = description->onActionCallback;
    return (nbgl_layout_t *) &layout_dummy;
}

int nbgl_layoutAddContentCenter(nbgl_layout_t *layout, const nbgl_contentCenter_t *info)
{
    (void) layout;
    warning_center = info;
    return 0;
}

int nbgl_layoutAddChoiceButtons(nbgl_layout_t *layout, const nbgl_layoutChoiceButtons_t *info)
{
    (void) layout;
    warning_choice_token = info->token;
    warning_choice_drawn = true;
    return 0;
}

static const char                *modal_header_text;
static uint8_t                    nb_bars;
static const nbgl_icon_details_t *layout_top_right_icon;

int nbgl_layoutAddHeader(nbgl_layout_t *layout, const nbgl_layoutHeader_t *headerDesc)
{
    (void) layout;
    if (headerDesc->type == HEADER_BACK_AND_TEXT) {
        modal_header_text = headerDesc->backAndText.text;
    }
    return 0;
}

int nbgl_layoutAddTouchableBar(nbgl_layout_t *layout, const nbgl_layoutBar_t *barLayout)
{
    (void) layout;
    (void) barLayout;
    nb_bars++;
    return 0;
}

int nbgl_layoutAddSeparationLine(nbgl_layout_t *layout)
{
    (void) layout;
    return 0;
}

int nbgl_layoutAddTopRightButton(nbgl_layout_t             *layout,
                                 const nbgl_icon_details_t *icon,
                                 uint8_t                    token,
                                 tune_index_e               tuneId)
{
    (void) layout;
    (void) token;
    (void) tuneId;
    layout_top_right_icon = icon;
    return 0;
}

int nbgl_layoutAddLeftContent(nbgl_layout_t *layout, const nbgl_layoutLeftContent_t *info)
{
    (void) layout;
    (void) info;
    return 0;
}

int nbgl_layoutAddTextContent(nbgl_layout_t *layout, nbgl_layoutTextContent_t *content)
{
    (void) layout;
    modal_title      = content->title;
    modal_full_value = content->descriptions[content->nbDescriptions - 1];
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

// Layout functions used by the other use cases, not exercised here
#define UNUSED_LAYOUT_FUNC(name, ...)                              \
    int nbgl_layout##name(__VA_ARGS__)                             \
    {                                                              \
        TEST_FAIL_MESSAGE("unexpected call to nbgl_layout" #name); \
        return -1;                                                 \
    }

UNUSED_LAYOUT_FUNC(AddExtendedFooter, nbgl_layout_t *layout, const nbgl_layoutFooter_t *footerDesc)
UNUSED_LAYOUT_FUNC(AddFooter,
                   nbgl_layout_t *layout,
                   const char    *text,
                   uint8_t        token,
                   tune_index_e   tuneId)
UNUSED_LAYOUT_FUNC(AddKeyboard, nbgl_layout_t *layout, const nbgl_layoutKbd_t *kbdInfo)
UNUSED_LAYOUT_FUNC(AddKeyboardContent, nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content)
UNUSED_LAYOUT_FUNC(AddKeypad, nbgl_layout_t *layout, keyboardCallback_t callback, bool shuffled)
UNUSED_LAYOUT_FUNC(AddKeypadContent,
                   nbgl_layout_t *layout,
                   const char    *title,
                   bool           hidden,
                   uint8_t        nbDigits,
                   const char    *text)
UNUSED_LAYOUT_FUNC(AddQRCode, nbgl_layout_t *layout, const nbgl_layoutQRCode_t *info)
UNUSED_LAYOUT_FUNC(AddSpinner,
                   nbgl_layout_t *layout,
                   const char    *text,
                   const char    *subText,
                   uint8_t        initPosition)
UNUSED_LAYOUT_FUNC(UpdateKeyboard,
                   nbgl_layout_t *layout,
                   uint8_t        index,
                   uint32_t       keyMask,
                   bool           updateCasing,
                   keyboardCase_t casing)
UNUSED_LAYOUT_FUNC(UpdateKeyboardContent,
                   nbgl_layout_t                *layout,
                   nbgl_layoutKeyboardContent_t *content)
UNUSED_LAYOUT_FUNC(UpdateKeypad,
                   nbgl_layout_t *layout,
                   uint8_t        index,
                   bool           enableValidate,
                   bool           enableBackspace,
                   bool           enableDigits)
UNUSED_LAYOUT_FUNC(UpdateKeypadContent,
                   nbgl_layout_t *layout,
                   bool           hidden,
                   uint8_t        nbActiveDigits,
                   const char    *text)
UNUSED_LAYOUT_FUNC(UpdateSpinner,
                   nbgl_layout_t *layout,
                   const char    *text,
                   const char    *subText,
                   uint8_t        position)

// ============================================================================
// NBGL fonts/screen and OS stubs: every text fits on a single line
// ============================================================================

uint8_t nbgl_getFontLineHeight(nbgl_font_id_e fontId)
{
    (void) fontId;
    return TEXT_HEIGHT;
}

uint16_t nbgl_getTextHeightInWidth(nbgl_font_id_e fontId,
                                   const char    *text,
                                   uint16_t       maxWidth,
                                   bool           wrapping)
{
    (void) fontId;
    (void) text;
    (void) maxWidth;
    (void) wrapping;
    return TEXT_HEIGHT;
}

uint16_t nbgl_getTextNbLinesInWidth(nbgl_font_id_e fontId,
                                    const char    *text,
                                    uint16_t       maxWidth,
                                    bool           wrapping)
{
    (void) fontId;
    (void) text;
    (void) maxWidth;
    (void) wrapping;
    return 1;
}

bool nbgl_getTextMaxLenInNbLines(nbgl_font_id_e fontId,
                                 const char    *text,
                                 uint16_t       maxWidth,
                                 uint16_t       maxNbLines,
                                 uint16_t      *len,
                                 bool           wrapping)
{
    (void) fontId;
    (void) maxWidth;
    (void) maxNbLines;
    (void) wrapping;
    *len = strlen(text);
    return true;
}

uint8_t nbgl_getTextNbPagesInWidth(nbgl_font_id_e fontId,
                                   const char    *text,
                                   uint8_t        nbLinesPerPage,
                                   uint16_t       maxWidth)
{
    (void) fontId;
    (void) text;
    (void) nbLinesPerPage;
    (void) maxWidth;
    return 1;
}

void nbgl_textReduceOnNbLines(nbgl_font_id_e fontId,
                              const char    *origText,
                              uint16_t       maxWidth,
                              uint8_t        nbLines,
                              char          *reducedText,
                              uint16_t       reducedTextLen)
{
    (void) fontId;
    (void) maxWidth;
    (void) nbLines;
    snprintf(reducedText, reducedTextLen, "%s", origText);
}

void nbgl_refresh(void) {}

void nbgl_refreshSpecial(nbgl_refresh_mode_t mode)
{
    (void) mode;
}

void nbgl_refreshSpecialWithPostRefresh(nbgl_refresh_mode_t mode, nbgl_post_refresh_t post_refresh)
{
    (void) mode;
    (void) post_refresh;
}

void nbgl_screenRedraw(void) {}

int os_io_seph_cmd_piezo_play_tune(tune_index_e tune_index)
{
    (void) tune_index;
    return 0;
}

// ============================================================================
// Test data: a TAG_VALUE_CONFIRM list of NB_PAIRS pairs, all with a value alias, either given as
// an array or through a callback. The array is allocated with its exact size, so that any access
// beyond the list is caught by the address sanitizer.
// ============================================================================

static char                    items[NB_PAIRS][16];
static char                    aliases[NB_PAIRS][16];
static char                    full_values[NB_PAIRS][16];
static nbgl_contentValueExt_t  extensions[NB_PAIRS];
static nbgl_contentTagValue_t *pairs;
static nbgl_content_t          content;
static nbgl_genericContents_t  contents;
// index of the first pair to get with the callback (startIndex of the list)
static uint8_t callbackBase;
// index (relative to callbackBase) of the pair not provided by the callback, -1 if none
static int missingPair;
// number of times the missing pair is still provided before the callback returns NULL for it
static int nbCallsBeforeMissing;

static int nb_rejects;

static void onReject(void)
{
    nb_rejects++;
}

static int  nb_choices;
static bool last_choice;

static void onChoice(bool confirm)
{
    nb_choices++;
    last_choice = confirm;
}

static nbgl_contentTagValue_t *getPair(uint8_t pairIndex)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(callbackBase, pairIndex);
    TEST_ASSERT_LESS_THAN_UINT8(NB_PAIRS, pairIndex - callbackBase);
    if ((pairIndex - callbackBase) == missingPair) {
        if (nbCallsBeforeMissing == 0) {
            return NULL;
        }
        nbCallsBeforeMissing--;
    }
    return &pairs[pairIndex - callbackBase];
}

// Builds the list, each page containing at most maxPairsInPage pairs (forced with forcePageStart)
static void buildList(uint8_t maxPairsInPage, bool withCallback)
{
    memset(pairs, 0, NB_PAIRS * sizeof(nbgl_contentTagValue_t));
    for (uint8_t i = 0; i < NB_PAIRS; i++) {
        pairs[i].item           = items[i];
        pairs[i].value          = aliases[i];
        pairs[i].extension      = &extensions[i];
        pairs[i].aliasValue     = 1;
        pairs[i].forcePageStart = ((i % maxPairsInPage) == 0) ? 1 : 0;
    }

    memset(&content, 0, sizeof(content));
    content.type                                         = TAG_VALUE_CONFIRM;
    content.content.tagValueConfirm.tagValueList.nbPairs = NB_PAIRS;
    if (withCallback) {
        content.content.tagValueConfirm.tagValueList.pairs      = NULL;
        content.content.tagValueConfirm.tagValueList.callback   = getPair;
        content.content.tagValueConfirm.tagValueList.startIndex = callbackBase;
    }
    else {
        content.content.tagValueConfirm.tagValueList.pairs = pairs;
    }
    content.content.tagValueConfirm.confirmationText  = "Confirm";
    content.content.tagValueConfirm.confirmationToken = CONFIRMATION_TOKEN;
    content.content.tagValueConfirm.cancelText        = "Cancel";
    content.content.tagValueConfirm.cancelToken       = CANCEL_TOKEN;

    contents.callbackCallNeeded = false;
    contents.contentsList       = &content;
    contents.nbContents         = 1;
}

void setUp(void)
{
    memset(&page_content, 0, sizeof(page_content));
    memset(&page_nav, 0, sizeof(page_nav));
    page_callback         = NULL;
    modal_title           = NULL;
    modal_full_value      = NULL;
    layout_callback       = NULL;
    warning_center        = NULL;
    warning_choice_token  = 0;
    warning_choice_drawn  = false;
    modal_header_text     = NULL;
    nb_bars               = 0;
    layout_top_right_icon = NULL;
    nb_choices            = 0;
    last_choice           = false;
    nb_rejects            = 0;
    callbackBase          = 0;
    missingPair           = -1;
    nbCallsBeforeMissing  = 0;

    memset(extensions, 0, sizeof(extensions));
    for (uint8_t i = 0; i < NB_PAIRS; i++) {
        snprintf(items[i], sizeof(items[i]), "Item %d", i);
        snprintf(aliases[i], sizeof(aliases[i]), "Alias %d", i);
        snprintf(full_values[i], sizeof(full_values[i]), "Address %d", i);
        extensions[i].fullValue = full_values[i];
        extensions[i].aliasType = ENS_ALIAS;
    }
    pairs = malloc(NB_PAIRS * sizeof(nbgl_contentTagValue_t));
    TEST_ASSERT_NOT_NULL(pairs);
}

void tearDown(void)
{
    free(pairs);
    pairs = NULL;
}

// Returns the tag/value list of the currently drawn page (intermediate pages are drawn as
// TAG_VALUE_LIST, the last one as TAG_VALUE_CONFIRM)
static const nbgl_contentTagValueList_t *currentList(void)
{
    if (page_content.type == TAG_VALUE_CONFIRM) {
        return &page_content.tagValueConfirm.tagValueList;
    }
    TEST_ASSERT_EQUAL(TAG_VALUE_LIST, page_content.type);
    return &page_content.tagValueList;
}

// Touches the alias of the pair at the given index in the current page
static void touchAlias(uint8_t index)
{
    modal_title      = NULL;
    modal_full_value = NULL;
    page_callback(currentList()->token, index);
}

// Goes through all pages of the review and touches every alias, checking that the modal always
// shows the touched pair. On the last page, also touches an alias beyond the list.
static void checkAllAliases(uint8_t maxPairsInPage, bool withCallback)
{
    char    msg[64];
    uint8_t firstPairIdx = 0;
    uint8_t pageIdx      = 0;

    buildList(maxPairsInPage, withCallback);
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_NOT_NULL(page_callback);

    while (true) {
        const nbgl_contentTagValueList_t *list = currentList();

        snprintf(msg,
                 sizeof(msg),
                 "max %d pairs/page, %s, page %d",
                 maxPairsInPage,
                 withCallback ? "callback" : "array",
                 pageIdx);
        TEST_ASSERT_GREATER_THAN_UINT8_MESSAGE(0, list->nbPairs, msg);
        TEST_ASSERT_LESS_OR_EQUAL_UINT8_MESSAGE(maxPairsInPage, list->nbPairs, msg);
        if (withCallback) {
            TEST_ASSERT_NULL_MESSAGE(list->pairs, msg);
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(callbackBase + firstPairIdx, list->startIndex, msg);
        }
        else {
            TEST_ASSERT_EQUAL_PTR_MESSAGE(&pairs[firstPairIdx], list->pairs, msg);
        }

        for (uint8_t i = 0; i < list->nbPairs; i++) {
            touchAlias(i);
            TEST_ASSERT_EQUAL_STRING_MESSAGE(aliases[firstPairIdx + i], modal_title, msg);
            TEST_ASSERT_EQUAL_STRING_MESSAGE(full_values[firstPairIdx + i], modal_full_value, msg);
        }
        firstPairIdx += currentList()->nbPairs;

        if (page_content.type == TAG_VALUE_CONFIRM) {
            break;
        }
        pageIdx++;
        page_callback(page_nav.navWithButtons.navToken, pageIdx);
    }
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(NB_PAIRS, firstPairIdx, msg);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(page_nav.nbPages - 1, pageIdx, msg);

    // an alias beyond the end of the list shall not display anything
    touchAlias(currentList()->nbPairs);
    TEST_ASSERT_EQUAL_PTR_MESSAGE(NO_MODAL_TEXT_DRAWN, modal_title, msg);
    TEST_ASSERT_EQUAL_PTR_MESSAGE(NO_MODAL_TEXT_DRAWN, modal_full_value, msg);
}

// ============================================================================
// Tests
// ============================================================================

void test_alias_shows_touched_pair_on_every_page(void)
{
    // from 1 pair per page up to as many pairs as fit in a page
    for (uint8_t maxPairsInPage = 1; maxPairsInPage <= NB_PAIRS; maxPairsInPage++) {
        checkAllAliases(maxPairsInPage, false);
    }
}

void test_alias_shows_touched_pair_on_every_page_with_callback(void)
{
    // from 1 pair per page up to as many pairs as fit in a page
    for (uint8_t maxPairsInPage = 1; maxPairsInPage <= NB_PAIRS; maxPairsInPage++) {
        checkAllAliases(maxPairsInPage, true);
    }
}

// Starts a review on a single page, in which the second pair is an alias without extension, and
// checks that touching this alias does not display anything, while the other aliases still work
static void checkAliasWithoutExtension(bool withCallback)
{
    buildList(NB_PAIRS, withCallback);
    pairs[1].extension = NULL;
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_GREATER_THAN_UINT8(2, currentList()->nbPairs);

    touchAlias(1);
    TEST_ASSERT_EQUAL_PTR(NO_MODAL_TEXT_DRAWN, modal_title);
    TEST_ASSERT_EQUAL_PTR(NO_MODAL_TEXT_DRAWN, modal_full_value);

    touchAlias(0);
    TEST_ASSERT_EQUAL_STRING(aliases[0], modal_title);
    TEST_ASSERT_EQUAL_STRING(full_values[0], modal_full_value);
    touchAlias(2);
    TEST_ASSERT_EQUAL_STRING(aliases[2], modal_title);
    TEST_ASSERT_EQUAL_STRING(full_values[2], modal_full_value);
}

void test_alias_shows_touched_pair_on_every_page_with_callback_base(void)
{
    // the callback must be called from startIndex, on every page and for every alias
    callbackBase = 50;
    for (uint8_t maxPairsInPage = 1; maxPairsInPage <= NB_PAIRS; maxPairsInPage++) {
        checkAllAliases(maxPairsInPage, true);
    }
}

void test_alias_shows_touched_pair_with_callback_base_at_uint8_limit(void)
{
    // the last pair is retrieved with callback(UINT8_MAX)
    callbackBase = UINT8_MAX + 1 - NB_PAIRS;
    checkAllAliases(NB_PAIRS, true);
}

void test_generic_review_is_rejected_on_callback_index_overflow(void)
{
    // the last pair would be retrieved with callback(UINT8_MAX + 1), wrapping to 0
    callbackBase = UINT8_MAX + 2 - NB_PAIRS;
    buildList(NB_PAIRS, true);
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_EQUAL_INT(1, nb_rejects);
    TEST_ASSERT_NULL(page_callback);
}

void test_review_is_rejected_on_callback_index_overflow(void)
{
    callbackBase = UINT8_MAX + 2 - NB_PAIRS;
    buildList(NB_PAIRS, true);
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &content.content.tagValueConfirm.tagValueList,
                       &C_Warning_64px,
                       "Review",
                       NULL,
                       "Sign",
                       onChoice);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
    TEST_ASSERT_NULL(page_callback);
}

void test_generic_review_is_rejected_on_missing_callback_pair(void)
{
    buildList(NB_PAIRS, true);
    missingPair = 2;
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_EQUAL_INT(1, nb_rejects);
    TEST_ASSERT_NULL(page_callback);
}

void test_review_is_rejected_on_missing_callback_pair(void)
{
    buildList(NB_PAIRS, true);
    missingPair = 2;
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &content.content.tagValueConfirm.tagValueList,
                       &C_Warning_64px,
                       "Review",
                       NULL,
                       "Sign",
                       onChoice);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
    TEST_ASSERT_NULL(page_callback);
}

void test_review_is_rejected_without_callback(void)
{
    buildList(NB_PAIRS, true);
    content.content.tagValueConfirm.tagValueList.callback = NULL;
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &content.content.tagValueConfirm.tagValueList,
                       &C_Warning_64px,
                       "Review",
                       NULL,
                       "Sign",
                       onChoice);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
    TEST_ASSERT_NULL(page_callback);
}

void test_review_is_displayed_when_callback_pair_is_missing_after_start(void)
{
    // the pair is only provided when the review is started
    buildList(1, true);
    missingPair          = 2;
    nbCallsBeforeMissing = 1;
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_EQUAL_INT(0, nb_rejects);
    TEST_ASSERT_NOT_NULL(page_callback);
}

void test_page_shows_empty_pair_when_callback_pair_is_missing_while_drawing(void)
{
    const nbgl_contentTagValueList_t *list;

    buildList(3, true);
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_NOT_NULL(page_callback);

    // the second pair of the first page is no longer provided, the following one still is
    missingPair = 1;
    list        = currentList();
    TEST_ASSERT_EQUAL_UINT8(3, list->nbPairs);
    TEST_ASSERT_NOT_NULL(list->callback);
    TEST_ASSERT_EQUAL_PTR(&pairs[0], list->callback(list->startIndex));
    TEST_ASSERT_NOT_NULL(list->callback(list->startIndex + 1));
    TEST_ASSERT_EQUAL_STRING("", list->callback(list->startIndex + 1)->item);
    TEST_ASSERT_EQUAL_STRING("", list->callback(list->startIndex + 1)->value);
    TEST_ASSERT_EQUAL_PTR(&pairs[2], list->callback(list->startIndex + 2));
}

void test_review_is_rejected_without_list(void)
{
    nbgl_useCaseReview(TYPE_TRANSACTION, NULL, &C_Warning_64px, "Review", NULL, "Sign", onChoice);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
    TEST_ASSERT_NULL(page_callback);
}

void test_review_is_rejected_with_empty_list(void)
{
    // without pairs, the review could not go further than its first page
    nbgl_contentTagValueList_t emptyList = {0};

    for (uint8_t withPairs = 0; withPairs < 2; withPairs++) {
        emptyList.pairs = withPairs ? pairs : NULL;
        nb_choices      = 0;
        page_callback   = NULL;
        nbgl_useCaseReview(
            TYPE_TRANSACTION, &emptyList, &C_Warning_64px, "Review", NULL, "Sign", onChoice);
        TEST_ASSERT_EQUAL_INT(1, nb_choices);
        TEST_ASSERT_FALSE(last_choice);
        TEST_ASSERT_NULL(page_callback);
    }
}

void test_generic_review_is_rejected_with_empty_list(void)
{
    // an intro, an empty tag/value content and an approval page: the empty content would block
    // the review on its intro
    nbgl_content_t         emptyContents[3] = {0};
    nbgl_genericContents_t emptyGeneric     = {0};

    emptyContents[0].type                                = CENTERED_INFO;
    emptyContents[0].content.centeredInfo.text1          = "Intro";
    emptyContents[2].type                                = INFO_LONG_PRESS;
    emptyContents[2].content.infoLongPress.text          = "Sign";
    emptyContents[2].content.infoLongPress.longPressText = "Hold to sign";
    emptyGeneric.contentsList                            = emptyContents;
    emptyGeneric.nbContents                              = 3;

    for (uint8_t confirm = 0; confirm < 2; confirm++) {
        for (uint8_t withPairs = 0; withPairs < 2; withPairs++) {
            memset(&emptyContents[1], 0, sizeof(emptyContents[1]));
            emptyContents[1].type = confirm ? TAG_VALUE_CONFIRM : TAG_VALUE_LIST;
            if (confirm) {
                emptyContents[1].content.tagValueConfirm.tagValueList.pairs
                    = withPairs ? pairs : NULL;
                emptyContents[1].content.tagValueConfirm.confirmationText = "Confirm";
            }
            else {
                emptyContents[1].content.tagValueList.pairs = withPairs ? pairs : NULL;
            }
            nb_rejects    = 0;
            page_callback = NULL;
            nbgl_useCaseGenericReview(&emptyGeneric, "Reject", onReject);
            TEST_ASSERT_EQUAL_INT(1, nb_rejects);
            TEST_ASSERT_NULL(page_callback);
        }
    }
}

void test_streaming_review_is_rejected_without_list(void)
{
    nbgl_useCaseReviewStreamingContinue(NULL, onChoice);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
    TEST_ASSERT_NULL(page_callback);
}

void test_alias_without_extension_shows_nothing(void)
{
    checkAliasWithoutExtension(false);
}

void test_alias_without_extension_shows_nothing_with_callback(void)
{
    checkAliasWithoutExtension(true);
}

void test_review_spans_at_least_three_pages(void)
{
    buildList(NB_PAIRS / 3, false);
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(3, page_nav.nbPages);
}

// ============================================================================
// nbgl_useCaseAddressReview: first page + address page with QR code button
// ============================================================================

// Starts an address review with the given additional list, and checks that it is displayed and
// that the address page, reached from the first page, has the QR code button
static void checkAddressReviewWithQrCode(const nbgl_contentTagValueList_t *additionalList)
{
    nbgl_useCaseAddressReview(
        "0xADDRESS", additionalList, &C_Warning_64px, "Verify address", NULL, onChoice);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
    TEST_ASSERT_NOT_NULL(page_callback);
    TEST_ASSERT_EQUAL(EXTENDED_CENTER, page_content.type);

    page_callback(page_nav.navWithButtons.navToken, 1);
    TEST_ASSERT_EQUAL(TAG_VALUE_CONFIRM, page_content.type);
    TEST_ASSERT_EQUAL_UINT8(1, page_content.tagValueConfirm.tagValueList.nbPairs);
    TEST_ASSERT_EQUAL_STRING("0xADDRESS", page_content.tagValueConfirm.tagValueList.pairs[0].value);
    TEST_ASSERT_EQUAL_PTR(&QRCODE_ICON, page_content.tagValueConfirm.detailsButtonIcon);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);
}

void test_address_review_with_empty_list_is_displayed(void)
{
    // a zeroed list (no pairs, no callback), as given by the Rust SDK without extra fields
    nbgl_contentTagValueList_t emptyList = {0};

    checkAddressReviewWithQrCode(&emptyList);
}

void test_address_review_without_list_is_displayed(void)
{
    checkAddressReviewWithQrCode(NULL);
}

void test_address_review_is_rejected_without_callback(void)
{
    buildList(NB_PAIRS, true);
    content.content.tagValueConfirm.tagValueList.callback = NULL;
    nbgl_useCaseAddressReview("0xADDRESS",
                              &content.content.tagValueConfirm.tagValueList,
                              &C_Warning_64px,
                              "Verify address",
                              NULL,
                              onChoice);
    TEST_ASSERT_EQUAL_INT(1, nb_choices);
    TEST_ASSERT_FALSE(last_choice);
    TEST_ASSERT_NULL(page_callback);
}

// ============================================================================
// V-293: custom warning only made of the info field
// ============================================================================

static const nbgl_contentCenter_t customWarningInfo
    = {.title = "Custom warning", .description = "Dangerous operation"};

// Starts an advanced review (regular or streaming) with the given warning
static void startAdvancedReview(const nbgl_warning_t *warning, bool streaming)
{
    if (streaming) {
        nbgl_useCaseAdvancedReviewStreamingStart(
            TYPE_TRANSACTION, &C_Warning_64px, "Review", NULL, warning, onChoice);
    }
    else {
        buildList(NB_PAIRS, false);
        nbgl_useCaseAdvancedReview(TYPE_TRANSACTION,
                                   &content.content.tagValueConfirm.tagValueList,
                                   &C_Warning_64px,
                                   "Review",
                                   NULL,
                                   "Sign",
                                   NULL,
                                   warning,
                                   onChoice);
    }
}

// Checks that the custom warning page is displayed instead of the review, then accepts or rejects
// it, depending on accept
static void checkInfoOnlyWarning(bool streaming, bool accept)
{
    nbgl_warning_t warning = {0};

    warning.info = &customWarningInfo;
    startAdvancedReview(&warning, streaming);

    // the custom warning page shall be drawn, and the review not started
    TEST_ASSERT_EQUAL_PTR(&customWarningInfo, warning_center);
    TEST_ASSERT_TRUE(warning_choice_drawn);
    TEST_ASSERT_NOT_NULL(layout_callback);
    TEST_ASSERT_NULL(page_callback);
    TEST_ASSERT_EQUAL_INT(0, nb_choices);

    if (accept) {
        // bottom button: the review shall start
        layout_callback(warning_choice_token, 1);
        TEST_ASSERT_NOT_NULL(page_callback);
        TEST_ASSERT_EQUAL_INT(0, nb_choices);
    }
    else {
        // top button: the review shall be rejected without being started
        layout_callback(warning_choice_token, 0);
        TEST_ASSERT_NULL(page_callback);
        TEST_ASSERT_EQUAL_INT(1, nb_choices);
        TEST_ASSERT_FALSE(last_choice);
    }
}

void test_info_only_warning_is_displayed_before_review(void)
{
    checkInfoOnlyWarning(false, true);
}

void test_info_only_warning_can_be_rejected(void)
{
    checkInfoOnlyWarning(false, false);
}

void test_info_only_warning_is_displayed_before_streaming_review(void)
{
    checkInfoOnlyWarning(true, true);
}

void test_info_only_warning_can_be_rejected_in_streaming_review(void)
{
    checkInfoOnlyWarning(true, false);
}

// Without any warning (NULL or empty), the review shall start directly
void test_empty_warning_starts_review_directly(void)
{
    nbgl_warning_t warning = {0};

    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        page_callback = NULL;
        startAdvancedReview(NULL, streaming);
        TEST_ASSERT_NOT_NULL(page_callback);
        TEST_ASSERT_FALSE(warning_choice_drawn);

        page_callback = NULL;
        startAdvancedReview(&warning, streaming);
        TEST_ASSERT_NOT_NULL(page_callback);
        TEST_ASSERT_FALSE(warning_choice_drawn);
    }
}

// ============================================================================
// V-295: custom review details accessible from first/last pages of review
// ============================================================================

static const nbgl_warningDetails_t customIntroDetails = {
    .title        = "Intro details",
    .type         = CENTERED_INFO_WARNING,
    .centeredInfo = {.title = "Intro", .description = "Intro details"}
};
static const nbgl_warningDetails_t customReviewDetails = {
    .title        = "Review details",
    .type         = CENTERED_INFO_WARNING,
    .centeredInfo = {.title = "Review", .description = "Review details"}
};

// Checks that the current page has a top-right button with the given icon, and that touching it
// displays either the custom review details or the predefined security report
static void checkTopRightButton(const nbgl_icon_details_t *expectedIcon,
                                bool                       customDetails,
                                const char                *msg)
{
    TEST_ASSERT_EQUAL_PTR_MESSAGE(expectedIcon, page_content.topRightIcon, msg);
    warning_center    = NULL;
    modal_header_text = NULL;
    nb_bars           = 0;
    page_callback(page_content.topRightToken, 0);
    if (customDetails) {
        TEST_ASSERT_EQUAL_PTR_MESSAGE(&customReviewDetails.centeredInfo, warning_center, msg);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(customReviewDetails.title, modal_header_text, msg);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nb_bars, msg);
    }
    else {
        TEST_ASSERT_NULL_MESSAGE(warning_center, msg);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("Security report", modal_header_text, msg);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nb_bars, msg);
    }
}

// Goes through the review (regular or streaming), checking that the top-right button is
// displayed with the given icon in the first and last pages, and only in them
static void checkTopRightButtons(const nbgl_icon_details_t *expectedIcon,
                                 bool                       customDetails,
                                 bool                       streaming)
{
    const char *msg = streaming ? "streaming" : "regular";

    // first page of review
    TEST_ASSERT_NOT_NULL_MESSAGE(page_callback, msg);
    TEST_ASSERT_EQUAL_MESSAGE(EXTENDED_CENTER, page_content.type, msg);
    checkTopRightButton(expectedIcon, customDetails, msg);

    if (streaming) {
        buildList(NB_PAIRS, false);
        nbgl_useCaseReviewStreamingContinue(&content.content.tagValueConfirm.tagValueList,
                                            onChoice);
        TEST_ASSERT_NULL_MESSAGE(page_content.topRightIcon, msg);
        nbgl_useCaseReviewStreamingFinish("Sign", onChoice);
    }
    else {
        uint8_t pageIdx = 0;
        while (page_content.type != INFO_LONG_PRESS) {
            pageIdx++;
            TEST_ASSERT_LESS_THAN_UINT8_MESSAGE(page_nav.nbPages, pageIdx, msg);
            page_callback(page_nav.navWithButtons.navToken, pageIdx);
            if (page_content.type != INFO_LONG_PRESS) {
                // no button in intermediate pages
                TEST_ASSERT_NULL_MESSAGE(page_content.topRightIcon, msg);
            }
        }
    }
    // last page of review
    TEST_ASSERT_EQUAL_MESSAGE(INFO_LONG_PRESS, page_content.type, msg);
    checkTopRightButton(expectedIcon, customDetails, msg);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nb_choices, msg);
}

// Goes through the review (regular or streaming), checking that the custom review details are
// accessible from the first and last pages, and only from them
static void checkReviewDetails(const nbgl_warning_t *warning, bool streaming)
{
    checkTopRightButtons(
        (warning->reviewTopRightIcon != NULL) ? warning->reviewTopRightIcon : &WARNING_ICON,
        true,
        streaming);
}

// A warning only made of review details (and icon) shall not lead to an ordinary review
void test_review_details_only_warning_is_accessible(void)
{
    nbgl_warning_t warning = {0};

    warning.reviewDetails      = &customReviewDetails;
    warning.reviewTopRightIcon = &C_QRCode_40px;
    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        page_callback = NULL;
        startAdvancedReview(&warning, streaming);
        checkReviewDetails(&warning, streaming);
    }
}

// Without reviewTopRightIcon, a default warning icon shall be used
void test_review_details_without_icon_uses_warning_icon(void)
{
    nbgl_warning_t warning = {0};

    warning.reviewDetails = &customReviewDetails;
    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        page_callback = NULL;
        startAdvancedReview(&warning, streaming);
        checkReviewDetails(&warning, streaming);
    }
}

// With an initial custom warning, the intro details shall not be displayed once the review has
// started, but the review details
void test_review_details_after_initial_warning(void)
{
    nbgl_warning_t warning = {0};

    warning.info               = &customWarningInfo;
    warning.introDetails       = &customIntroDetails;
    warning.reviewDetails      = &customReviewDetails;
    warning.reviewTopRightIcon = &C_QRCode_40px;
    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        page_callback = NULL;
        startAdvancedReview(&warning, streaming);
        // accept the initial warning to start the review
        TEST_ASSERT_TRUE(warning_choice_drawn);
        TEST_ASSERT_NULL(page_callback);
        layout_callback(warning_choice_token, 1);
        checkReviewDetails(&warning, streaming);
    }
}

// Without any warning, no top-right button shall be displayed in first page of review
void test_no_warning_no_review_details_button(void)
{
    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        page_callback = NULL;
        startAdvancedReview(NULL, streaming);
        TEST_ASSERT_NOT_NULL(page_callback);
        TEST_ASSERT_NULL(page_content.topRightIcon);
    }
}

// Starts an advanced review with the given predefined set, with or without the custom fields, and
// checks that the predefined warning is used everywhere, the custom fields being ignored
static void checkPredefinedWarning(uint32_t                   predefinedSet,
                                   const nbgl_icon_details_t *expectedIcon,
                                   bool                       withCustom,
                                   bool                       streaming)
{
    nbgl_warning_t warning = {0};

    warning.predefinedSet = predefinedSet;
    if (withCustom) {
        warning.introDetails       = &customIntroDetails;
        warning.reviewDetails      = &customReviewDetails;
        warning.introTopRightIcon  = &C_Search_40px;
        warning.reviewTopRightIcon = &C_QRCode_40px;
    }
    page_callback         = NULL;
    layout_top_right_icon = NULL;
    warning_choice_drawn  = false;
    startAdvancedReview(&warning, streaming);
    if (HAS_INITIAL_WARNING((&warning))) {
        // the predefined initial warning page shall be drawn, without the custom icon
        TEST_ASSERT_TRUE(warning_choice_drawn);
        TEST_ASSERT_NULL(page_callback);
        TEST_ASSERT_NOT_NULL(layout_top_right_icon);
        TEST_ASSERT_TRUE(layout_top_right_icon != &C_Search_40px);
        TEST_ASSERT_TRUE(warning_center != &customWarningInfo);
        // accept it to start the review
        layout_callback(warning_choice_token, 1);
    }
    else {
        TEST_ASSERT_FALSE(warning_choice_drawn);
    }
    checkTopRightButtons(expectedIcon, false, streaming);
}

// A predefined warning alone shall display the predefined security report
void test_predefined_warning_without_custom_fields(void)
{
    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        checkPredefinedWarning(1 << BLIND_SIGNING_WARN, &WARNING_ICON, false, streaming);
        checkPredefinedWarning(1 << W3C_NO_THREAT_WARN, &PRIVACY_ICON, false, streaming);
    }
}

// When both a predefined warning and custom fields are given, the predefined warning shall take
// precedence: neither the custom icons nor the custom details shall be displayed
void test_predefined_warning_takes_precedence_over_custom_fields(void)
{
    for (uint8_t streaming = 0; streaming < 2; streaming++) {
        checkPredefinedWarning(1 << BLIND_SIGNING_WARN, &WARNING_ICON, true, streaming);
        checkPredefinedWarning(1 << W3C_NO_THREAT_WARN, &PRIVACY_ICON, true, streaming);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alias_shows_touched_pair_on_every_page);
    RUN_TEST(test_alias_shows_touched_pair_on_every_page_with_callback);
    RUN_TEST(test_alias_shows_touched_pair_on_every_page_with_callback_base);
    RUN_TEST(test_alias_shows_touched_pair_with_callback_base_at_uint8_limit);
    RUN_TEST(test_generic_review_is_rejected_on_callback_index_overflow);
    RUN_TEST(test_review_is_rejected_on_callback_index_overflow);
    RUN_TEST(test_generic_review_is_rejected_on_missing_callback_pair);
    RUN_TEST(test_review_is_rejected_on_missing_callback_pair);
    RUN_TEST(test_review_is_rejected_without_callback);
    RUN_TEST(test_review_is_displayed_when_callback_pair_is_missing_after_start);
    RUN_TEST(test_page_shows_empty_pair_when_callback_pair_is_missing_while_drawing);
    RUN_TEST(test_review_is_rejected_without_list);
    RUN_TEST(test_review_is_rejected_with_empty_list);
    RUN_TEST(test_generic_review_is_rejected_with_empty_list);
    RUN_TEST(test_streaming_review_is_rejected_without_list);
    RUN_TEST(test_alias_without_extension_shows_nothing);
    RUN_TEST(test_alias_without_extension_shows_nothing_with_callback);
    RUN_TEST(test_review_spans_at_least_three_pages);
    RUN_TEST(test_address_review_with_empty_list_is_displayed);
    RUN_TEST(test_address_review_without_list_is_displayed);
    RUN_TEST(test_address_review_is_rejected_without_callback);
    RUN_TEST(test_info_only_warning_is_displayed_before_review);
    RUN_TEST(test_info_only_warning_can_be_rejected);
    RUN_TEST(test_info_only_warning_is_displayed_before_streaming_review);
    RUN_TEST(test_info_only_warning_can_be_rejected_in_streaming_review);
    RUN_TEST(test_empty_warning_starts_review_directly);
    RUN_TEST(test_review_details_only_warning_is_accessible);
    RUN_TEST(test_review_details_without_icon_uses_warning_icon);
    RUN_TEST(test_review_details_after_initial_warning);
    RUN_TEST(test_no_warning_no_review_details_button);
    RUN_TEST(test_predefined_warning_without_custom_fields);
    RUN_TEST(test_predefined_warning_takes_precedence_over_custom_fields);
    return UNITY_END();
}
