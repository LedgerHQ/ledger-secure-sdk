#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"

#include "nbgl_use_case.h"
#include "nbgl_page.h"
#include "nbgl_layout_internal.h"
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
// Regression tests for V-127: the keyboard must never write beyond entryBuffer[entryMaxLen],
// even when the suggestions callback leaves letter keys enabled.

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
const nbgl_icon_details_t C_Mini_Push_40px;
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

// modal pages (the details of a TAG_VALUE_LIST_ALIAS alias) are only expected when this is true
static bool                       modal_pages_expected;
static nbgl_layoutTouchCallback_t modal_callback;
static nbgl_pageNavigationInfo_t  modal_nav;
static nbgl_pageContent_t         modal_content;

nbgl_page_t *nbgl_pageDrawGenericContentExt(nbgl_layoutTouchCallback_t       onActionCallback,
                                            const nbgl_pageNavigationInfo_t *nav,
                                            nbgl_pageContent_t              *content,
                                            bool                             modal)
{
    (void) modal;
    if (!modal_pages_expected) {
        TEST_FAIL_MESSAGE("unexpected call to nbgl_pageDrawGenericContentExt");
    }
    modal_callback = onActionCallback;
    memcpy(&modal_nav, nav, sizeof(modal_nav));
    memcpy(&modal_content, content, sizeof(modal_content));
    return (nbgl_page_t *) &page_dummy;
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

// Keyboard layout: memorize the key callback and the last key mask
static int                nb_keyboards;
static keyboardCallback_t kbd_callback;
static uint32_t           kbd_mask;

int nbgl_layoutAddKeyboard(nbgl_layout_t *layout, const nbgl_layoutKbd_t *kbdInfo)
{
    (void) layout;
    nb_keyboards++;
    kbd_callback = kbdInfo->callback;
    kbd_mask     = kbdInfo->keyMask;
    return 1;
}

int nbgl_layoutAddKeyboardContent(nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content)
{
    (void) layout;
    (void) content;
    return 2;
}

int nbgl_layoutUpdateKeyboard(nbgl_layout_t *layout,
                              uint8_t        index,
                              uint32_t       keyMask,
                              bool           updateCasing,
                              keyboardCase_t casing)
{
    (void) layout;
    (void) index;
    (void) updateCasing;
    (void) casing;
    kbd_mask = keyMask;
    return 0;
}

int nbgl_layoutUpdateKeyboardContent(nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content)
{
    (void) layout;
    (void) content;
    return 0;
}

// Keypad layout: memorize the key callback and the last displayed digits
static int                nb_keypads;
static keyboardCallback_t keypad_callback;
static const char        *keypad_text;

int nbgl_layoutAddKeypad(nbgl_layout_t *layout, keyboardCallback_t callback, bool shuffled)
{
    (void) layout;
    (void) shuffled;
    nb_keypads++;
    keypad_callback = callback;
    return 0;
}

int nbgl_layoutAddKeypadContent(nbgl_layout_t *layout,
                                const char    *title,
                                bool           hidden,
                                uint8_t        nbDigits,
                                const char    *text)
{
    (void) layout;
    (void) title;
    (void) hidden;
    (void) nbDigits;
    keypad_text = text;
    return 0;
}

int nbgl_layoutUpdateKeypad(nbgl_layout_t *layout,
                            uint8_t        index,
                            bool           enableValidate,
                            bool           enableBackspace,
                            bool           enableDigits)
{
    (void) layout;
    (void) index;
    (void) enableValidate;
    (void) enableBackspace;
    (void) enableDigits;
    return 0;
}

int nbgl_layoutUpdateKeypadContent(nbgl_layout_t *layout,
                                   bool           hidden,
                                   uint8_t        nbActiveDigits,
                                   const char    *text)
{
    (void) layout;
    (void) hidden;
    (void) nbActiveDigits;
    keypad_text = text;
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
UNUSED_LAYOUT_FUNC(AddQRCode, nbgl_layout_t *layout, const nbgl_layoutQRCode_t *info)
UNUSED_LAYOUT_FUNC(AddSpinner,
                   nbgl_layout_t *layout,
                   const char    *text,
                   const char    *subText,
                   uint8_t        initPosition)
UNUSED_LAYOUT_FUNC(UpdateSpinner,
                   nbgl_layout_t *layout,
                   const char    *text,
                   const char    *subText,
                   uint8_t        position)

// ============================================================================
// NBGL fonts/screen and OS stubs: every text fits on a single line, except LONG_VALUE, and
// WIDER_IN_BOLD in the small bold font
// ============================================================================

// a value too long to fit in a review page
static const char LONG_VALUE[] = "long value";
#define LONG_VALUE_NB_LINES (NB_MAX_LINES_IN_REVIEW + 5)
// a value fitting in a single line in the regular font, but on 2 lines in the small bold font
static const char WIDER_IN_BOLD[] = "wider in bold";
// a value fitting in a single line in the small fonts, but on 3 lines in the large font
static const char TALLER_IN_LARGE[] = "taller in large";
// a value displayed in many pages of the "More" details modal. Any text starting in it is split in
// lines of a number of chars depending on the font, like real fonts (fewer chars in bold)
#define MORE_VALUE_LEN 1500
static char MORE_VALUE[MORE_VALUE_LEN + 1];

static bool isInMoreValue(const char *text)
{
    return (text >= MORE_VALUE) && (text < &MORE_VALUE[MORE_VALUE_LEN]);
}

static uint16_t getNbCharsPerLine(nbgl_font_id_e fontId)
{
    if (fontId == SMALL_BOLD_FONT) {
        return 20;
    }
    if (fontId == SMALL_REGULAR_FONT) {
        return 24;
    }
    return 18;
}

static uint16_t getTextNbLines(nbgl_font_id_e fontId, const char *text)
{
    if (text == NULL) {
        return 1;
    }
    if (isInMoreValue(text)) {
        uint16_t nbChars = getNbCharsPerLine(fontId);
        return (strlen(text) + nbChars - 1) / nbChars;
    }
    if (strcmp(text, LONG_VALUE) == 0) {
        return LONG_VALUE_NB_LINES;
    }
    if ((strcmp(text, WIDER_IN_BOLD) == 0) && (fontId == SMALL_BOLD_FONT)) {
        return 2;
    }
    if ((strcmp(text, TALLER_IN_LARGE) == 0) && (fontId == LARGE_MEDIUM_FONT)) {
        return 3;
    }
    return 1;
}

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
    (void) maxWidth;
    (void) wrapping;
    return getTextNbLines(fontId, text) * TEXT_HEIGHT;
}

uint16_t nbgl_getTextNbLinesInWidth(nbgl_font_id_e fontId,
                                    const char    *text,
                                    uint16_t       maxWidth,
                                    bool           wrapping)
{
    (void) maxWidth;
    (void) wrapping;
    return getTextNbLines(fontId, text);
}

bool nbgl_getTextMaxLenInNbLines(nbgl_font_id_e fontId,
                                 const char    *text,
                                 uint16_t       maxWidth,
                                 uint16_t       maxNbLines,
                                 uint16_t      *len,
                                 bool           wrapping)
{
    (void) maxWidth;
    (void) wrapping;
    *len = strlen(text);
    if (isInMoreValue(text) && (*len > (maxNbLines * getNbCharsPerLine(fontId)))) {
        *len = maxNbLines * getNbCharsPerLine(fontId);
    }
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
    modal_pages_expected  = false;
    modal_callback        = NULL;
    nb_choices            = 0;
    last_choice           = false;
    nb_rejects            = 0;
    callbackBase          = 0;
    missingPair           = -1;
    nbCallsBeforeMissing  = 0;
    nb_keyboards          = 0;
    kbd_callback          = NULL;
    kbd_mask              = 0;
    nb_keypads            = 0;
    keypad_callback       = NULL;
    keypad_text           = NULL;

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

// ============================================================================
// nbMaxLinesForValue of the pairs: a value with a max number of lines is counted with its
// truncated height to split the pages, so it shares its page and never needs a "More" page
// ============================================================================

#define NB_PAIRS_WITH_LONG_VALUE 3

// the max number of lines of a pair is in the bit-field byte of the pair, so the size of the
// public structure is unchanged: 3 pointers + the bit-field byte (padded)
_Static_assert(sizeof(nbgl_contentTagValue_t) == (4 * sizeof(void *)),
               "nbgl_contentTagValue_t size changed");

// Builds a TAG_VALUE_LIST of NB_PAIRS_WITH_LONG_VALUE pairs, the first one with LONG_VALUE
static void buildListWithLongValue(uint8_t pairMaxLines,
                                   uint8_t listMaxLines,
                                   bool    alias,
                                   bool    withCallback)
{
    memset(pairs, 0, NB_PAIRS * sizeof(nbgl_contentTagValue_t));
    for (uint8_t i = 0; i < NB_PAIRS_WITH_LONG_VALUE; i++) {
        pairs[i].item  = items[i];
        pairs[i].value = aliases[i];
    }
    pairs[0].value              = LONG_VALUE;
    pairs[0].nbMaxLinesForValue = pairMaxLines;
    if (alias) {
        pairs[0].aliasValue = 1;
        pairs[0].extension  = &extensions[0];
    }

    memset(&content, 0, sizeof(content));
    content.type                                    = TAG_VALUE_LIST;
    content.content.tagValueList.nbPairs            = NB_PAIRS_WITH_LONG_VALUE;
    content.content.tagValueList.nbMaxLinesForValue = listMaxLines;
    if (withCallback) {
        content.content.tagValueList.callback   = getPair;
        content.content.tagValueList.startIndex = callbackBase;
    }
    else {
        content.content.tagValueList.pairs = pairs;
    }

    contents.callbackCallNeeded = false;
    contents.contentsList       = &content;
    contents.nbContents         = 1;
}

// Returns the number of pages of the given list, split by the public helper used by applications
static uint8_t getNbPagesWithPublicHelper(const nbgl_contentTagValueList_t *tagValueList)
{
    uint8_t nbPages = 0;
    uint8_t i       = 0;
    bool    flag;

    while (i < tagValueList->nbPairs) {
        i += nbgl_useCaseGetNbTagValuesInPage(tagValueList->nbPairs - i, tagValueList, i, &flag);
        nbPages++;
    }
    return nbPages;
}

// Checks that the long value shares its page with the other pairs, without "More" button
static void checkLongValueSharesItsPage(uint8_t pairMaxLines,
                                        uint8_t listMaxLines,
                                        bool    alias,
                                        bool    withCallback)
{
    buildListWithLongValue(pairMaxLines, listMaxLines, alias, withCallback);
    TEST_ASSERT_EQUAL_UINT8(1, getNbPagesWithPublicHelper(&content.content.tagValueList));

    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_EQUAL(TAG_VALUE_LIST, page_content.type);
    TEST_ASSERT_EQUAL_UINT8(NB_PAIRS_WITH_LONG_VALUE, page_content.tagValueList.nbPairs);
}

// Checks that the long value is alone in its page, with a "More" button, as without max
static void checkLongValueUsesDetailsPage(uint8_t listMaxLines, bool withCallback)
{
    buildListWithLongValue(0, listMaxLines, false, withCallback);
    TEST_ASSERT_EQUAL_UINT8(2, getNbPagesWithPublicHelper(&content.content.tagValueList));

    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_EQUAL(TAG_VALUE_DETAILS, page_content.type);
    TEST_ASSERT_EQUAL_UINT8(1, page_content.tagValueDetails.tagValueList.nbPairs);
    TEST_ASSERT_EQUAL_UINT8(NB_MAX_LINES_IN_REVIEW,
                            page_content.tagValueDetails.tagValueList.nbMaxLinesForValue);
}

void test_pair_max_lines_shares_page(void)
{
    for (uint8_t withCallback = 0; withCallback < 2; withCallback++) {
        for (uint8_t maxLines = 1; maxLines <= 3; maxLines++) {
            checkLongValueSharesItsPage(maxLines, 0, false, withCallback);
            checkLongValueSharesItsPage(maxLines, 0, true, withCallback);
            // the smallest of the list and pair max is used
            checkLongValueSharesItsPage(maxLines, NB_MAX_LINES_IN_REVIEW, false, withCallback);
        }
    }
}

void test_no_pair_max_lines_uses_details_page(void)
{
    for (uint8_t withCallback = 0; withCallback < 2; withCallback++) {
        checkLongValueUsesDetailsPage(0, withCallback);
        // a list max smaller than NB_MAX_LINES_IN_REVIEW is overridden by the review, so it
        // changes nothing
        checkLongValueUsesDetailsPage(2, withCallback);
    }
}

// ============================================================================
// Details of a TAG_VALUE_LIST_ALIAS alias: the pairs of each modal page must fit in the page, with
// the font used to draw them (small bold for values), in which their values take 2 lines
// ============================================================================

#define NB_ALIAS_LIST_PAIRS 8

static nbgl_contentTagValue_t     aliasListPairs[NB_ALIAS_LIST_PAIRS];
static nbgl_contentTagValueList_t aliasList;

// Builds a list of NB_ALIAS_LIST_PAIRS alias pairs with WIDER_IN_BOLD values, to be changed by
// the caller before calling openAliasListModal()
static void buildAliasList(void)
{
    memset(aliasListPairs, 0, sizeof(aliasListPairs));
    for (uint8_t i = 0; i < NB_ALIAS_LIST_PAIRS; i++) {
        aliasListPairs[i].item       = items[i];
        aliasListPairs[i].value      = WIDER_IN_BOLD;
        aliasListPairs[i].aliasValue = 1;
    }
    memset(&aliasList, 0, sizeof(aliasList));
    aliasList.pairs   = aliasListPairs;
    aliasList.nbPairs = NB_ALIAS_LIST_PAIRS;
}

// Starts a review whose first pair has the alias list as alias, and touches this alias
static void openAliasListModal(void)
{
    buildList(NB_PAIRS, false);
    extensions[0].aliasType    = TAG_VALUE_LIST_ALIAS;
    extensions[0].tagValuelist = &aliasList;
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    modal_pages_expected = true;
    touchAlias(0);
    TEST_ASSERT_NOT_NULL(modal_callback);
}

// Goes through all modal pages, checking that each page contains at least one pair and that all
// pairs are displayed once. If pairHeight is not 0, also checks that the pairs fit in each page.
// If the alias list uses a callback, the pairs of each page must be retrieved with the callback of
// the page, from its startIndex
static void checkAliasListModalPages(uint16_t pairHeight)
{
    uint8_t nbPairsDrawn = 0;
    for (uint8_t page = 0; page < modal_nav.nbPages; page++) {
        uint8_t nbPairs = modal_content.tagValueList.nbPairs;
        TEST_ASSERT_EQUAL(TAG_VALUE_LIST, modal_content.type);
        if (aliasList.pairs != NULL) {
            TEST_ASSERT_EQUAL_PTR(&aliasListPairs[nbPairsDrawn], modal_content.tagValueList.pairs);
        }
        else {
            const nbgl_contentTagValueList_t *drawn = &modal_content.tagValueList;
            TEST_ASSERT_NULL(drawn->pairs);
            TEST_ASSERT_NOT_NULL(drawn->callback);
            TEST_ASSERT_EQUAL_UINT8(aliasList.startIndex + nbPairsDrawn, drawn->startIndex);
            for (uint8_t i = 0; i < nbPairs; i++) {
                TEST_ASSERT_EQUAL_PTR(&aliasListPairs[nbPairsDrawn + i],
                                      drawn->callback(drawn->startIndex + i));
            }
        }
        TEST_ASSERT_GREATER_THAN_UINT8(0, nbPairs);
        if (pairHeight > 0) {
            TEST_ASSERT_LESS_THAN_UINT16(TAG_VALUE_AREA_HEIGHT,
                                         PRE_TAG_VALUE_MARGIN + nbPairs * pairHeight
                                             + (nbPairs - 1) * INTER_TAG_VALUE_MARGIN);
        }
        nbPairsDrawn += nbPairs;
        if ((page + 1) < modal_nav.nbPages) {
            modal_callback(modal_nav.navWithButtons.navToken, page + 1);
        }
    }
    TEST_ASSERT_EQUAL_UINT8(NB_ALIAS_LIST_PAIRS, nbPairsDrawn);
}

void test_alias_list_modal_pages_fit(void)
{
    buildAliasList();
    openAliasListModal();
    // height of a pair as drawn: tag, space, and value on 2 lines
    checkAliasListModalPages(TEXT_HEIGHT + TAG_VALUE_INTERVALE + 2 * TEXT_HEIGHT);
}

void test_alias_list_modal_pair_too_long_has_its_page(void)
{
    // a pair too long to fit in a modal page is displayed alone in its page, it doesn't make the
    // modal pages empty
    buildAliasList();
    aliasListPairs[0].value = LONG_VALUE;
    openAliasListModal();
    TEST_ASSERT_EQUAL_UINT8(1, modal_content.tagValueList.nbPairs);
    // its value is truncated to the max number of lines fitting in the page, with "..."
    uint8_t        nbLines   = modal_content.tagValueList.nbMaxLinesForValue;
    const uint16_t tagHeight = PRE_TAG_VALUE_MARGIN + TEXT_HEIGHT + TAG_VALUE_INTERVALE;
    TEST_ASSERT_GREATER_THAN_UINT8(0, nbLines);
    TEST_ASSERT_LESS_THAN_UINT8(LONG_VALUE_NB_LINES, nbLines);
    TEST_ASSERT_TRUE(modal_content.tagValueList.hideEndOfLastLine);
    TEST_ASSERT_LESS_THAN_UINT16(TAG_VALUE_AREA_HEIGHT, tagHeight + nbLines * TEXT_HEIGHT);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(TAG_VALUE_AREA_HEIGHT,
                                        tagHeight + (nbLines + 1) * TEXT_HEIGHT);
    checkAliasListModalPages(0);
}

// first index given to the callback of the alias list (its startIndex)
#define ALIAS_LIST_START_INDEX 20

static nbgl_contentTagValue_t *getAliasListPair(uint8_t pairIndex)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(ALIAS_LIST_START_INDEX, pairIndex);
    TEST_ASSERT_LESS_THAN_UINT8(ALIAS_LIST_START_INDEX + NB_ALIAS_LIST_PAIRS, pairIndex);
    return &aliasListPairs[pairIndex - ALIAS_LIST_START_INDEX];
}

// Changes the alias list built by buildAliasList() to a list using a callback
static void useAliasListCallback(void)
{
    aliasList.pairs      = NULL;
    aliasList.callback   = getAliasListPair;
    aliasList.startIndex = ALIAS_LIST_START_INDEX;
}

void test_alias_list_modal_with_callback(void)
{
    // the pairs of an alias list using a callback are split in pages as the static ones, and each
    // modal page gets them with the callback, from startIndex. The callback of the page gives the
    // pairs of the application as is (same pointers), so their own nbMaxLinesForValue reaches
    // the layout
    buildAliasList();
    useAliasListCallback();
    openAliasListModal();
    TEST_ASSERT_GREATER_THAN_UINT8(1, modal_nav.nbPages);
    // height of a pair as drawn: tag, space, and value on 2 lines
    checkAliasListModalPages(TEXT_HEIGHT + TAG_VALUE_INTERVALE + 2 * TEXT_HEIGHT);
}

void test_alias_list_modal_with_callback_pair_too_long(void)
{
    // pairs too long to fit in a page of an alias list using a callback are alone in their page
    // and truncated, as the static ones
    buildAliasList();
    aliasListPairs[0].value = LONG_VALUE;
    aliasListPairs[1].value = LONG_VALUE;
    useAliasListCallback();
    openAliasListModal();
    TEST_ASSERT_GREATER_THAN_UINT8(2, modal_nav.nbPages);
    for (uint8_t page = 0; page < 2; page++) {
        if (page > 0) {
            modal_callback(modal_nav.navWithButtons.navToken, page);
        }
        TEST_ASSERT_EQUAL_UINT8(1, modal_content.tagValueList.nbPairs);
        TEST_ASSERT_GREATER_THAN_UINT8(0, modal_content.tagValueList.nbMaxLinesForValue);
        TEST_ASSERT_LESS_THAN_UINT8(LONG_VALUE_NB_LINES,
                                    modal_content.tagValueList.nbMaxLinesForValue);
        TEST_ASSERT_TRUE(modal_content.tagValueList.hideEndOfLastLine);
    }
    // open the modal again to check all pages from the first one: touching the alias calls
    // displayTagValueListModal(), which resets detailsContext
    openAliasListModal();
    checkAliasListModalPages(0);
}

// ============================================================================
// Review pages are split with the font used to draw their values
// ============================================================================

// Builds a list of NB_PAIRS pairs with the given value, and smallCaseForValue set to true
static void buildSmallCaseList(nbgl_contentType_t type, const char *value)
{
    nbgl_contentTagValueList_t *list;

    memset(pairs, 0, NB_PAIRS * sizeof(nbgl_contentTagValue_t));
    for (uint8_t i = 0; i < NB_PAIRS; i++) {
        pairs[i].item  = items[i];
        pairs[i].value = value;
    }
    memset(&content, 0, sizeof(content));
    content.type = type;
    if (type == TAG_VALUE_CONFIRM) {
        list = &content.content.tagValueConfirm.tagValueList;
        content.content.tagValueConfirm.confirmationText  = "Confirm";
        content.content.tagValueConfirm.confirmationToken = CONFIRMATION_TOKEN;
    }
    else {
        list = &content.content.tagValueList;
    }
    list->pairs             = pairs;
    list->nbPairs           = NB_PAIRS;
    list->smallCaseForValue = true;

    contents.callbackCallNeeded = false;
    contents.contentsList       = &content;
    contents.nbContents         = 1;
}

// Goes through the pages of the review, checking that they are drawn with the given
// smallCaseForValue, that pairs of the given height fit in each page, and that all pairs are
// displayed
static void checkReviewPagesFit(bool smallCaseDrawn, uint16_t pairHeight)
{
    uint8_t nbPairsDrawn = 0;

    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    for (uint8_t page = 0; nbPairsDrawn < NB_PAIRS; page++) {
        const nbgl_contentTagValueList_t *list;
        TEST_ASSERT_LESS_THAN_UINT8(page_nav.nbPages, page);
        if (page > 0) {
            page_callback(page_nav.navWithButtons.navToken, page);
        }
        list = (page_content.type == TAG_VALUE_CONFIRM) ? &page_content.tagValueConfirm.tagValueList
                                                        : &page_content.tagValueList;
        TEST_ASSERT_EQUAL(smallCaseDrawn, list->smallCaseForValue);
        TEST_ASSERT_GREATER_THAN_UINT8(0, list->nbPairs);
        TEST_ASSERT_LESS_THAN_UINT16(TAG_VALUE_AREA_HEIGHT,
                                     PRE_TAG_VALUE_MARGIN + list->nbPairs * pairHeight
                                         + (list->nbPairs - 1) * INTER_TAG_VALUE_MARGIN);
        nbPairsDrawn += list->nbPairs;
    }
    TEST_ASSERT_EQUAL_UINT8(NB_PAIRS, nbPairsDrawn);
}

void test_review_list_pages_split_with_drawn_font(void)
{
    // a TAG_VALUE_LIST is drawn with the large font, whatever smallCaseForValue
    buildSmallCaseList(TAG_VALUE_LIST, TALLER_IN_LARGE);
    checkReviewPagesFit(false, TEXT_HEIGHT + TAG_VALUE_INTERVALE + 3 * TEXT_HEIGHT);
}

void test_review_confirm_pages_split_with_drawn_font(void)
{
    // a TAG_VALUE_CONFIRM is drawn with its smallCaseForValue, so with the small bold font
    buildSmallCaseList(TAG_VALUE_CONFIRM, WIDER_IN_BOLD);
    checkReviewPagesFit(true, TEXT_HEIGHT + TAG_VALUE_INTERVALE + 2 * TEXT_HEIGHT);
}

// ============================================================================
// "More" details modal: its pages are counted with the font used to display them, so the last page
// displays the end of the value, and going back displays the same pages as going forward
// ============================================================================

void test_more_details_pages_show_whole_value(void)
{
    const char *pagesStart[UINT8_MAX];
    uint8_t     nbPages;

    memset(MORE_VALUE, 'A', MORE_VALUE_LEN);
    MORE_VALUE[MORE_VALUE_LEN] = '\0';
    // a single pair with this value, too long for a review page: it has a "More" button
    memset(pairs, 0, NB_PAIRS * sizeof(nbgl_contentTagValue_t));
    pairs[0].item  = items[0];
    pairs[0].value = MORE_VALUE;
    memset(&content, 0, sizeof(content));
    content.type                         = TAG_VALUE_LIST;
    content.content.tagValueList.pairs   = pairs;
    content.content.tagValueList.nbPairs = 1;
    contents.callbackCallNeeded          = false;
    contents.contentsList                = &content;
    contents.nbContents                  = 1;
    nbgl_useCaseGenericReview(&contents, "Reject", onReject);
    TEST_ASSERT_EQUAL(TAG_VALUE_DETAILS, page_content.type);

    modal_pages_expected = true;
    page_callback(page_content.tagValueDetails.detailsButtonToken, 0);
    TEST_ASSERT_NOT_NULL(modal_callback);
    nbPages = modal_nav.nbPages;
    TEST_ASSERT_GREATER_THAN_UINT8(1, nbPages);

    // forward: every page but the last one is cut, the last one displays the end of the value
    for (uint8_t page = 0; page < nbPages; page++) {
        if (page > 0) {
            modal_callback(modal_nav.navWithButtons.navToken, page);
        }
        pagesStart[page] = modal_content.tagValueList.pairs[0].value;
        TEST_ASSERT_TRUE(isInMoreValue(pagesStart[page]));
        if (page < (nbPages - 1)) {
            TEST_ASSERT_EQUAL_UINT8(NB_MAX_LINES_IN_DETAILS,
                                    modal_content.tagValueList.nbMaxLinesForValue);
        }
        else {
            TEST_ASSERT_EQUAL_UINT8(0, modal_content.tagValueList.nbMaxLinesForValue);
            TEST_ASSERT_LESS_OR_EQUAL_UINT16(
                NB_MAX_LINES_IN_DETAILS * getNbCharsPerLine(SMALL_BOLD_FONT),
                strlen(pagesStart[page]));
        }
    }
    // backward: the same pages
    for (int page = nbPages - 2; page >= 0; page--) {
        modal_callback(modal_nav.navWithButtons.navToken, page);
        TEST_ASSERT_EQUAL_PTR(pagesStart[page], modal_content.tagValueList.pairs[0].value);
    }
}

// ============================================================================
// Max number of lines of a value: the smallest of the list and pair ones, and a value of a pair
// with a max always ends with "..." if it doesn't fit, whichever max cuts it
// ============================================================================

void test_tag_value_max_lines(void)
{
    nbgl_contentTagValue_t pair = {0};
    bool                   forceEllipsis;

    // no max for the pair: the list one is used, with the ellipsis setting of the list
    TEST_ASSERT_EQUAL_UINT8(0, layoutGetTagValueMaxLines(0, &pair, &forceEllipsis));
    TEST_ASSERT_FALSE(forceEllipsis);
    TEST_ASSERT_EQUAL_UINT8(5, layoutGetTagValueMaxLines(5, &pair, &forceEllipsis));
    TEST_ASSERT_FALSE(forceEllipsis);

    pair.nbMaxLinesForValue = 3;
    TEST_ASSERT_EQUAL_UINT8(3, layoutGetTagValueMaxLines(0, &pair, &forceEllipsis));
    TEST_ASSERT_TRUE(forceEllipsis);
    TEST_ASSERT_EQUAL_UINT8(3, layoutGetTagValueMaxLines(5, &pair, &forceEllipsis));
    TEST_ASSERT_TRUE(forceEllipsis);
    // the list max is smaller: it cuts the value, still with an ellipsis
    TEST_ASSERT_EQUAL_UINT8(2, layoutGetTagValueMaxLines(2, &pair, &forceEllipsis));
    TEST_ASSERT_TRUE(forceEllipsis);
    // forceEllipsis is optional
    TEST_ASSERT_EQUAL_UINT8(2, layoutGetTagValueMaxLines(2, &pair, NULL));
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
        .mode        = MODE_LETTERS,
        .lettersOnly = true,
        .casing      = LOWER_CASE,
    };
    if (type == KEYBOARD_WITH_BUTTON) {
        params.confirmationParams.buttonText       = "Confirm";
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

void test_keyboard_with_button_stops_at_max_len(void)
{
    start_keyboard(KEYBOARD_WITH_BUTTON);
    for (int i = 0; i < 2 * ENTRY_MAX_LEN; i++) {
        kbd_callback('a');
    }
    assert_entry_full_and_canary_intact();
}

void test_keyboard_with_suggestions_stops_at_max_len(void)
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

void test_keyboard_without_buffer_is_rejected(void)
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

void test_keyboard_with_zero_max_len_is_rejected(void)
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

static void on_pin_validate(const uint8_t *content, uint8_t length)
{
    (void) content;
    (void) length;
}

static void on_keypad_back(void) {}

void test_keypad_visible_digits_are_nul_terminated_at_max(void)
{
    nbgl_useCaseKeypad("PIN", 4, KEYPAD_MAX_DIGITS, false, false, on_pin_validate, on_keypad_back);
    TEST_ASSERT_EQUAL_INT(1, nb_keypads);
    TEST_ASSERT_NOT_NULL(keypad_callback);
    for (int i = 0; i < 2 * KEYPAD_MAX_DIGITS; i++) {
        keypad_callback('0' + (i % 10));
    }
    TEST_ASSERT_EQUAL_STRING("012345678901", keypad_text);
}

void test_keypad_too_many_max_digits_is_rejected(void)
{
    nbgl_useCaseKeypad(
        "PIN", 4, KEYPAD_MAX_DIGITS + 1, false, true, on_pin_validate, on_keypad_back);
    TEST_ASSERT_EQUAL_INT(0, nb_keypads);
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
    RUN_TEST(test_pair_max_lines_shares_page);
    RUN_TEST(test_no_pair_max_lines_uses_details_page);
    RUN_TEST(test_alias_list_modal_pages_fit);
    RUN_TEST(test_alias_list_modal_pair_too_long_has_its_page);
    RUN_TEST(test_alias_list_modal_with_callback);
    RUN_TEST(test_alias_list_modal_with_callback_pair_too_long);
    RUN_TEST(test_review_list_pages_split_with_drawn_font);
    RUN_TEST(test_review_confirm_pages_split_with_drawn_font);
    RUN_TEST(test_more_details_pages_show_whole_value);
    RUN_TEST(test_tag_value_max_lines);
    RUN_TEST(test_keyboard_with_button_stops_at_max_len);
    RUN_TEST(test_keyboard_with_suggestions_stops_at_max_len);
    RUN_TEST(test_keyboard_without_buffer_is_rejected);
    RUN_TEST(test_keyboard_with_zero_max_len_is_rejected);
    RUN_TEST(test_keypad_visible_digits_are_nul_terminated_at_max);
    RUN_TEST(test_keypad_too_many_max_digits_is_rejected);
    return UNITY_END();
}
