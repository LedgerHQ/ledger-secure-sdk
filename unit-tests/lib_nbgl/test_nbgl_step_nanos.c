#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "unity.h"

#include "nbgl_step.h"
#include "nbgl_layout.h"
#include "glyphs.h"

// Regression tests for V-330: re-rendering a modal Nano step (internal page navigation in a
// multi-pages text step, selection change in a menu list) must not leak a screen layer, nor
// exhaust the modal layout slots.

const nbgl_icon_details_t C_icon_down;

// ============================================================================
// Layout mock, mimicking the slots management of nbgl_layout_nanos.c:
// slot 0 is the background, slots 1 and 2 are the only modal slots, a modal
// layout pushes a screen and its release pops it
// ============================================================================

#define NB_SLOTS 3

typedef struct {
    bool                        used;
    bool                        modal;
    nbgl_layoutButtonCallback_t callback;
} mock_layout_t;

static mock_layout_t slots[NB_SLOTS];
static int           screen_stack_size;  // number of pushed (modal) screens
static int           nb_get_failures;

static void reset_mocks(void)
{
    memset(slots, 0, sizeof(slots));
    screen_stack_size = 0;
    nb_get_failures   = 0;
}

static int nb_used_modal_slots(void)
{
    return (slots[1].used ? 1 : 0) + (slots[2].used ? 1 : 0);
}

nbgl_layout_t *nbgl_layoutGet(const nbgl_layoutDescription_t *description)
{
    mock_layout_t *layout = NULL;
    if (description->modal) {
        if (!slots[1].used) {
            layout = &slots[1];
        }
        else if (!slots[2].used) {
            layout = &slots[2];
        }
    }
    else {
        layout = &slots[0];
    }
    if (layout == NULL) {
        nb_get_failures++;
        return NULL;
    }
    layout->used     = true;
    layout->modal    = description->modal;
    layout->callback = description->onActionCallback;
    if (description->modal) {
        screen_stack_size++;
    }
    return (nbgl_layout_t *) layout;
}

int nbgl_layoutRelease(nbgl_layout_t *layoutParam)
{
    mock_layout_t *layout = (mock_layout_t *) layoutParam;
    if (layout == NULL) {
        return -1;
    }
    if (layout->modal && layout->used) {
        screen_stack_size--;
    }
    layout->used = false;
    return 0;
}

int nbgl_layoutAddText(nbgl_layout_t                  *layout,
                       const char                     *text,
                       const char                     *subText,
                       nbgl_contentCenteredInfoStyle_t style)
{
    (void) text;
    (void) subText;
    (void) style;
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

int nbgl_layoutAddNavigation(nbgl_layout_t *layout, nbgl_layoutNavigation_t *info)
{
    (void) info;
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

int nbgl_layoutAddMenuList(nbgl_layout_t *layout, nbgl_layoutMenuList_t *list)
{
    (void) list;
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

int nbgl_layoutAddCenteredInfo(nbgl_layout_t *layout, const nbgl_layoutCenteredInfo_t *info)
{
    (void) info;
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

int nbgl_layoutAddSwitch(nbgl_layout_t *layout, const nbgl_layoutSwitch_t *switchLayout)
{
    (void) switchLayout;
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

int nbgl_layoutDraw(nbgl_layout_t *layout)
{
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

int layoutAddText(nbgl_layout_t                  *layout,
                  const char                     *text,
                  const char                     *subText,
                  nbgl_contentCenteredInfoStyle_t style,
                  uint8_t                         nbMaxLines,
                  bool                            ellipsis)
{
    (void) text;
    (void) subText;
    (void) style;
    (void) nbMaxLines;
    (void) ellipsis;
    TEST_ASSERT_NOT_NULL(layout);
    return 0;
}

uint16_t layoutGetButtonRowLen(const char *text, uint16_t *len, uint16_t *width)
{
    *len   = strlen(text);
    *width = 0;
    return *len;
}

void nbgl_refresh(void) {}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t srcLen = strlen(src);
    if (size > 0) {
        size_t n = (srcLen < size) ? srcLen : size - 1;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return srcLen;
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
    strlcpy(reducedText, origText, reducedTextLen);
}

// ============================================================================
// Font/text mocks: every page of the text contains PAGE_LEN chars
// ============================================================================

#define PAGE_LEN 4
#define NB_PAGES 4

static const char long_text[] = "0123456789ABCDEF";  // NB_PAGES * PAGE_LEN chars

uint8_t nbgl_getTextNbPagesInWidth(nbgl_font_id_e fontId,
                                   const char    *text,
                                   uint8_t        nbLinesPerPage,
                                   uint16_t       maxWidth)
{
    (void) fontId;
    (void) nbLinesPerPage;
    (void) maxWidth;
    return (strlen(text) + PAGE_LEN - 1) / PAGE_LEN;
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
    size_t textLen = strlen(text);
    *len           = (textLen < PAGE_LEN) ? textLen : PAGE_LEN;
    return textLen <= PAGE_LEN;
}

// ============================================================================
// Helpers
// ============================================================================

static int nb_step_actions;

static void stepCallback(nbgl_step_t step, nbgl_buttonEvent_t event)
{
    (void) step;
    (void) event;
    nb_step_actions++;
}

static void press(nbgl_buttonEvent_t event)
{
    // the event goes to the top-most used layout, as done by nbgl_layout_nanos.c
    int i = NB_SLOTS;
    while (i > 0) {
        i--;
        if (slots[i].used) {
            TEST_ASSERT_NOT_NULL(slots[i].callback);
            slots[i].callback((nbgl_layout_t *) &slots[i], event);
            return;
        }
    }
    TEST_FAIL_MESSAGE("no active layout");
}

static uint8_t menuSelected;

static const char *menuListCallback(uint8_t choiceIndex)
{
    (void) choiceIndex;
    return "item";
}

static void menuSelectedCallback(uint8_t choiceIndex)
{
    menuSelected = choiceIndex;
}

void setUp(void)
{
    reset_mocks();
    nb_step_actions = 0;
    menuSelected    = 0xFF;
}

void tearDown(void) {}

// ============================================================================
// Tests
// ============================================================================

void test_modal_text_step_navigation_keeps_one_screen(void)
{
    nbgl_step_t step
        = nbgl_stepDrawText(SINGLE_STEP, stepCallback, NULL, long_text, NULL, REGULAR_INFO, true);
    TEST_ASSERT_NOT_NULL(step);
    TEST_ASSERT_EQUAL_INT(1, screen_stack_size);

    // forward to the last page, then backward to the first one, several times
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int page = 1; page < NB_PAGES; page++) {
            press(BUTTON_RIGHT_PRESSED);
            TEST_ASSERT_EQUAL_INT(1, screen_stack_size);
            TEST_ASSERT_EQUAL_INT(1, nb_used_modal_slots());
        }
        for (int page = NB_PAGES - 1; page > 0; page--) {
            press(BUTTON_LEFT_PRESSED);
            TEST_ASSERT_EQUAL_INT(1, screen_stack_size);
            TEST_ASSERT_EQUAL_INT(1, nb_used_modal_slots());
        }
    }
    TEST_ASSERT_EQUAL_INT(0, nb_get_failures);
    // internal navigation is not reported to the step callback
    TEST_ASSERT_EQUAL_INT(0, nb_step_actions);

    // closing the step restores the background, with no orphan layer
    TEST_ASSERT_EQUAL_INT(0, nbgl_stepRelease(step));
    TEST_ASSERT_EQUAL_INT(0, screen_stack_size);
    TEST_ASSERT_EQUAL_INT(0, nb_used_modal_slots());
}

void test_modal_text_step_second_modal_can_still_be_opened(void)
{
    nbgl_step_t step
        = nbgl_stepDrawText(SINGLE_STEP, stepCallback, NULL, long_text, NULL, REGULAR_INFO, true);
    press(BUTTON_RIGHT_PRESSED);
    press(BUTTON_RIGHT_PRESSED);

    // a modal slot must still be available for another modal step on top
    nbgl_step_t step2
        = nbgl_stepDrawText(SINGLE_STEP, stepCallback, NULL, "x", NULL, REGULAR_INFO, true);
    TEST_ASSERT_NOT_NULL(step2);
    TEST_ASSERT_EQUAL_INT(2, screen_stack_size);
    TEST_ASSERT_EQUAL_INT(0, nb_get_failures);

    nbgl_stepRelease(step2);
    nbgl_stepRelease(step);
    TEST_ASSERT_EQUAL_INT(0, screen_stack_size);
    TEST_ASSERT_EQUAL_INT(0, nb_used_modal_slots());
}

void test_modal_menu_list_navigation_keeps_one_screen(void)
{
    nbgl_layoutMenuList_t list
        = {.callback = menuListCallback, .nbChoices = 4, .selectedChoice = 0};
    nbgl_step_t step = nbgl_stepDrawMenuList(menuSelectedCallback, NULL, &list, true);
    TEST_ASSERT_NOT_NULL(step);
    TEST_ASSERT_EQUAL_INT(1, screen_stack_size);

    for (int cycle = 0; cycle < 3; cycle++) {
        for (int i = 1; i < 4; i++) {
            press(BUTTON_RIGHT_PRESSED);
            TEST_ASSERT_EQUAL_INT(1, screen_stack_size);
        }
        for (int i = 1; i < 4; i++) {
            press(BUTTON_LEFT_PRESSED);
            TEST_ASSERT_EQUAL_INT(1, screen_stack_size);
        }
    }
    TEST_ASSERT_EQUAL_INT(0, nb_get_failures);

    press(BUTTON_BOTH_PRESSED);
    TEST_ASSERT_EQUAL_UINT8(0, menuSelected);

    nbgl_stepRelease(step);
    TEST_ASSERT_EQUAL_INT(0, screen_stack_size);
    TEST_ASSERT_EQUAL_INT(0, nb_used_modal_slots());
}

void test_background_text_step_navigation(void)
{
    nbgl_step_t step
        = nbgl_stepDrawText(SINGLE_STEP, stepCallback, NULL, long_text, NULL, REGULAR_INFO, false);
    TEST_ASSERT_NOT_NULL(step);
    for (int page = 1; page < NB_PAGES; page++) {
        press(BUTTON_RIGHT_PRESSED);
    }
    TEST_ASSERT_EQUAL_INT(0, screen_stack_size);
    TEST_ASSERT_TRUE(slots[0].used);
    TEST_ASSERT_EQUAL_INT(0, nb_get_failures);
    nbgl_stepRelease(step);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_modal_text_step_navigation_keeps_one_screen);
    RUN_TEST(test_modal_text_step_second_modal_can_still_be_opened);
    RUN_TEST(test_modal_menu_list_navigation_keeps_one_screen);
    RUN_TEST(test_background_text_step_navigation);
    return UNITY_END();
}
