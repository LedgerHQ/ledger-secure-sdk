#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "nbgl_layout.h"
#include "nbgl_layout_internal.h"
#include "os_io_seph_cmd.h"
#include "glyphs.h"

// Regression tests for V-138: a keyboard confirmation button made inactive (grayed-out) must not
// be touchable anymore, nor trigger the callback, and must trigger it again once reactivated.
// Regression tests for V-141: suggestion buttons not used (not displayed) must not be touchable,
// nor trigger the callback, at creation as well as after updates or swipes.

#define BUTTON_TOKEN        0x42
#define CROSS_TOKEN         0x43
#define FIRST_SUGGESTION_TK 0x50

// ============================================================================
// Glyphs
// ============================================================================

const nbgl_icon_details_t C_Close_40px;
const nbgl_icon_details_t C_left_half_64px;

// ============================================================================
// NBGL object pool stubs
// ============================================================================

// big enough for any NBGL object
typedef union {
    nbgl_obj_t            obj;
    nbgl_container_t      container;
    nbgl_button_t         button;
    nbgl_text_area_t      textArea;
    nbgl_image_t          image;
    nbgl_line_t           line;
    nbgl_keyboard_t       keyboard;
    nbgl_page_indicator_t pageIndicator;
} pool_obj_t;

#define OBJ_POOL_LEN      32
#define CHILDREN_POOL_LEN 64

static pool_obj_t  obj_pool[OBJ_POOL_LEN];
static uint8_t     nb_used_objs;
static nbgl_obj_t *children_pool[CHILDREN_POOL_LEN];
static uint8_t     nb_used_children;

nbgl_obj_t *nbgl_objPoolGet(nbgl_obj_type_t type, uint8_t layer)
{
    (void) layer;
    TEST_ASSERT_LESS_THAN(OBJ_POOL_LEN, nb_used_objs);
    nbgl_obj_t *obj = &obj_pool[nb_used_objs++].obj;
    obj->type       = type;
    return obj;
}

int nbgl_objPoolGetArray(nbgl_obj_type_t type, uint8_t nbObjs, uint8_t layer, nbgl_obj_t **objArray)
{
    for (uint8_t i = 0; i < nbObjs; i++) {
        objArray[i] = nbgl_objPoolGet(type, layer);
    }
    return 0;
}

nbgl_obj_t **nbgl_containerPoolGet(uint8_t nbObjs, uint8_t layer)
{
    (void) layer;
    TEST_ASSERT_LESS_OR_EQUAL(CHILDREN_POOL_LEN, nb_used_children + nbObjs);
    nbgl_obj_t **children = &children_pool[nb_used_children];
    nb_used_children += nbObjs;
    return children;
}

void nbgl_objDraw(nbgl_obj_t *obj)
{
    (void) obj;
}

void nbgl_refreshSpecial(nbgl_refresh_mode_t mode)
{
    (void) mode;
}

int os_io_seph_cmd_piezo_play_tune(tune_index_e tune_index)
{
    (void) tune_index;
    return 0;
}

// ============================================================================
// Font stubs
// ============================================================================

uint8_t nbgl_getFontHeight(nbgl_font_id_e fontId)
{
    (void) fontId;
    return 32;
}

uint16_t nbgl_getSingleLineTextWidth(nbgl_font_id_e fontId, const char *text)
{
    (void) fontId;
    return (uint16_t) (strlen(text) * 16);
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
    return 32;
}

// ============================================================================
// NBGL layout stubs (same behavior as nbgl_layout.c)
// ============================================================================

// same as in nbgl_layout.c
#define NB_MAX_CONTAINER_CHILDREN 20

static nbgl_layoutInternal_t layout_ctx;
static nbgl_container_t      main_container;
static nbgl_obj_t           *main_children[NB_MAX_CONTAINER_CHILDREN];

layoutObj_t *layoutAddCallbackObj(nbgl_layoutInternal_t *layout,
                                  nbgl_obj_t            *obj,
                                  uint8_t                token,
                                  tune_index_e           tuneId)
{
    TEST_ASSERT_LESS_THAN(LAYOUT_OBJ_POOL_LEN - 1, layout->nbUsedCallbackObjs);
    layoutObj_t *layoutObj = &layout->callbackObjPool[layout->nbUsedCallbackObjs];
    layout->nbUsedCallbackObjs++;
    layoutObj->obj    = obj;
    layoutObj->token  = token;
    layoutObj->tuneId = tuneId;
    return layoutObj;
}

void layoutUpdateCallbackObjToken(nbgl_layoutInternal_t *layout, nbgl_obj_t *obj, uint8_t token)
{
    for (size_t i = 0; i < layout->nbUsedCallbackObjs; i++) {
        if (layout->callbackObjPool[i].obj == obj) {
            layout->callbackObjPool[i].token = token;
            return;
        }
    }
}

void layoutAddObject(nbgl_layoutInternal_t *layout, nbgl_obj_t *obj)
{
    TEST_ASSERT_LESS_THAN(NB_MAX_CONTAINER_CHILDREN, layout->container->nbChildren);
    layout->container->children[layout->container->nbChildren++] = obj;
}

// ============================================================================
// Helpers
// ============================================================================

static int nb_actions;
static int last_token;

static void action_callback(int token, uint8_t index)
{
    (void) index;
    nb_actions++;
    last_token = token;
}

static nbgl_button_t *get_button(void)
{
    nbgl_button_t *button = (nbgl_button_t *) main_container.children[1];
    TEST_ASSERT_NOT_NULL(button);
    TEST_ASSERT_EQUAL(BUTTON, button->obj.type);
    return button;
}

static layoutObj_t *get_button_callback_obj(void)
{
    nbgl_obj_t *button = (nbgl_obj_t *) get_button();
    for (uint8_t i = 0; i < layout_ctx.nbUsedCallbackObjs; i++) {
        if (layout_ctx.callbackObjPool[i].obj == button) {
            return &layout_ctx.callbackObjPool[i];
        }
    }
    TEST_FAIL_MESSAGE("confirmation button has no callback object");
    return NULL;
}

// Layout dispatch of a touch event on the button, as touchCallback() in nbgl_layout.c
static void dispatch_to_layout(void)
{
    layoutObj_t *layoutObj = get_button_callback_obj();
    if ((layout_ctx.callback != NULL) && (layoutObj->token != NBGL_INVALID_TOKEN)) {
        layout_ctx.callback(layoutObj->token, layoutObj->index);
    }
}

// Full touch on the button: the OS only reports TOUCHED events for objects with this event in
// their touchMask, then the layout dispatches it
static void touch_button(void)
{
    if (get_button()->obj.touchMask & (1 << TOUCHED)) {
        dispatch_to_layout();
    }
}

static nbgl_layoutKeyboardContent_t content;

static void add_keyboard_content(bool active)
{
    content = (nbgl_layoutKeyboardContent_t){
        .type                      = KEYBOARD_WITH_BUTTON,
        .title                     = "Enter name",
        .text                      = "abc",
        .textToken                 = CROSS_TOKEN,
        .confirmationButton.text   = "Confirm",
        .confirmationButton.token  = BUTTON_TOKEN,
        .confirmationButton.active = active,
        .tuneId                    = NBGL_NO_TUNE,
    };
    TEST_ASSERT_GREATER_OR_EQUAL(
        0, nbgl_layoutAddKeyboardContent((nbgl_layout_t *) &layout_ctx, &content));
}

static void update_keyboard_content(bool active)
{
    content.confirmationButton.active = active;
    TEST_ASSERT_GREATER_OR_EQUAL(
        0, nbgl_layoutUpdateKeyboardContent((nbgl_layout_t *) &layout_ctx, &content));
}

static void assert_button_disabled(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, get_button()->obj.touchMask);
    TEST_ASSERT_EQUAL_UINT8(NBGL_INVALID_TOKEN, get_button_callback_obj()->token);
}

static void assert_button_enabled(void)
{
    TEST_ASSERT_EQUAL_UINT16(1 << TOUCHED, get_button()->obj.touchMask);
    TEST_ASSERT_EQUAL_UINT8(BUTTON_TOKEN, get_button_callback_obj()->token);
}

void setUp(void)
{
    memset(obj_pool, 0, sizeof(obj_pool));
    memset(children_pool, 0, sizeof(children_pool));
    nb_used_objs     = 0;
    nb_used_children = 0;

    memset(&main_container, 0, sizeof(main_container));
    memset(main_children, 0, sizeof(main_children));
    main_container.obj.type = CONTAINER;
    main_container.children = main_children;

    memset(&layout_ctx, 0, sizeof(layout_ctx));
    layout_ctx.container = &main_container;
    layout_ctx.callback  = action_callback;

    nb_actions = 0;
    last_token = -1;
}

void tearDown(void) {}

// ============================================================================
// Tests
// ============================================================================

void test_active_button_triggers_callback(void)
{
    add_keyboard_content(true);
    assert_button_enabled();

    touch_button();
    TEST_ASSERT_EQUAL_INT(1, nb_actions);
    TEST_ASSERT_EQUAL_INT(BUTTON_TOKEN, last_token);
}

void test_button_created_inactive_is_disabled(void)
{
    add_keyboard_content(false);
    assert_button_disabled();

    touch_button();
    TEST_ASSERT_EQUAL_INT(0, nb_actions);
}

void test_button_deactivated_is_disabled(void)
{
    add_keyboard_content(true);
    touch_button();
    TEST_ASSERT_EQUAL_INT(1, nb_actions);

    update_keyboard_content(false);
    assert_button_disabled();

    touch_button();
    TEST_ASSERT_EQUAL_INT(1, nb_actions);
}

// A touch started while the button was active and released after its deactivation must not
// trigger the callback, even if the OS still reports it
void test_late_release_after_deactivation_is_ignored(void)
{
    add_keyboard_content(true);

    update_keyboard_content(false);
    dispatch_to_layout();
    TEST_ASSERT_EQUAL_INT(0, nb_actions);
}

void test_button_reactivated_triggers_callback(void)
{
    add_keyboard_content(true);
    update_keyboard_content(false);
    update_keyboard_content(true);
    assert_button_enabled();

    touch_button();
    TEST_ASSERT_EQUAL_INT(1, nb_actions);
    TEST_ASSERT_EQUAL_INT(BUTTON_TOKEN, last_token);
}

void test_button_created_inactive_then_activated_triggers_callback(void)
{
    add_keyboard_content(false);
    update_keyboard_content(true);
    assert_button_enabled();

    touch_button();
    TEST_ASSERT_EQUAL_INT(1, nb_actions);
    TEST_ASSERT_EQUAL_INT(BUTTON_TOKEN, last_token);
}

// ============================================================================
// Suggestion buttons (V-141)
// ============================================================================

static const char *const all_suggestions[NB_MAX_SUGGESTION_BUTTONS]
    = {"abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract"};
static const char *suggestions[NB_MAX_SUGGESTION_BUTTONS];

static nbgl_container_t *get_suggestions_container(void)
{
    nbgl_container_t *container = (nbgl_container_t *) main_container.children[1];
    TEST_ASSERT_NOT_NULL(container);
    TEST_ASSERT_EQUAL(CONTAINER, container->obj.type);
    return container;
}

// returns the i-th suggestion button of the suggestions container (in children order)
static nbgl_button_t *get_suggestion_button(uint8_t index)
{
    nbgl_container_t *container = get_suggestions_container();
    uint8_t           found     = 0;
    for (uint8_t i = 0; i < container->nbChildren; i++) {
        nbgl_obj_t *child = container->children[i];
        if ((child != NULL) && (child->type == BUTTON)) {
            if (found == index) {
                return (nbgl_button_t *) child;
            }
            found++;
        }
    }
    TEST_FAIL_MESSAGE("suggestion button not found");
    return NULL;
}

static layoutObj_t *get_callback_obj(nbgl_obj_t *obj)
{
    for (uint8_t i = 0; i < layout_ctx.nbUsedCallbackObjs; i++) {
        if (layout_ctx.callbackObjPool[i].obj == obj) {
            return &layout_ctx.callbackObjPool[i];
        }
    }
    TEST_FAIL_MESSAGE("object has no callback object");
    return NULL;
}

// touch on a suggestion button, as done by the OS then touchCallback() in nbgl_layout.c
static void touch_suggestion(uint8_t index)
{
    nbgl_button_t *button = get_suggestion_button(index);
    if (button->obj.touchMask & (1 << TOUCHED)) {
        layoutObj_t *layoutObj = get_callback_obj((nbgl_obj_t *) button);
        if (layoutObj->token != NBGL_INVALID_TOKEN) {
            layout_ctx.callback(layoutObj->token, layoutObj->index);
        }
    }
}

static void set_suggestions(uint8_t nbUsed)
{
    for (uint8_t i = 0; i < NB_MAX_SUGGESTION_BUTTONS; i++) {
        suggestions[i] = (i < nbUsed) ? all_suggestions[i] : NULL;
    }
    content.suggestionButtons.nbUsedButtons = nbUsed;
}

static int add_suggestions_keyboard(uint8_t nbUsed)
{
    content = (nbgl_layoutKeyboardContent_t){
        .type                               = KEYBOARD_WITH_SUGGESTIONS,
        .title                              = "Enter word",
        .text                               = "",
        .textToken                          = CROSS_TOKEN,
        .suggestionButtons.buttons          = suggestions,
        .suggestionButtons.firstButtonToken = FIRST_SUGGESTION_TK,
        .tuneId                             = NBGL_NO_TUNE,
    };
    set_suggestions(nbUsed);
    return nbgl_layoutAddKeyboardContent((nbgl_layout_t *) &layout_ctx, &content);
}

static int update_suggestions_keyboard(uint8_t nbUsed)
{
    set_suggestions(nbUsed);
    return nbgl_layoutUpdateKeyboardContent((nbgl_layout_t *) &layout_ctx, &content);
}

static void swipe(nbgl_touchType_t eventType)
{
    // text entry and suggestions containers, as set by the full layout
    main_container.nbChildren = 2;
    keyboardSwipeCallback(&layout_ctx, (nbgl_obj_t *) &main_container, eventType);
}

// displayed suggestions are the ones from [firstIndex, firstIndex + nbDisplayed[
static void assert_suggestions(uint8_t firstIndex, uint8_t nbDisplayed)
{
    for (uint8_t i = 0; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
        nbgl_button_t *button    = get_suggestion_button(i);
        layoutObj_t   *layoutObj = get_callback_obj((nbgl_obj_t *) button);
        if (i < nbDisplayed) {
            TEST_ASSERT_EQUAL_PTR(all_suggestions[firstIndex + i], button->text);
            TEST_ASSERT_EQUAL_UINT16(1 << TOUCHED, button->obj.touchMask);
            TEST_ASSERT_EQUAL_UINT8(FIRST_SUGGESTION_TK + firstIndex + i, layoutObj->token);
        }
        else {
            TEST_ASSERT_NULL(button->text);
            TEST_ASSERT_EQUAL_UINT16(0, button->obj.touchMask);
            TEST_ASSERT_EQUAL_UINT8(NBGL_INVALID_TOKEN, layoutObj->token);
        }
    }
}

// touch all suggestion slots, and check that only the displayed ones trigger the callback
static void assert_only_displayed_suggestions_trigger(uint8_t firstIndex, uint8_t nbDisplayed)
{
    for (uint8_t i = 0; i < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; i++) {
        nb_actions = 0;
        last_token = -1;
        touch_suggestion(i);
        if (i < nbDisplayed) {
            TEST_ASSERT_EQUAL_INT(1, nb_actions);
            TEST_ASSERT_EQUAL_INT(FIRST_SUGGESTION_TK + firstIndex + i, last_token);
        }
        else {
            TEST_ASSERT_EQUAL_INT(0, nb_actions);
        }
    }
}

void test_suggestions_created_empty_are_disabled(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(0));
    assert_suggestions(0, 0);
    assert_only_displayed_suggestions_trigger(0, 0);
}

void test_suggestions_created_partially_used(void)
{
    for (uint8_t nbUsed = 1; nbUsed < NB_MAX_VISIBLE_SUGGESTION_BUTTONS; nbUsed++) {
        setUp();
        TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(nbUsed));
        assert_suggestions(0, nbUsed);
        assert_only_displayed_suggestions_trigger(0, nbUsed);
    }
}

void test_suggestions_created_full(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(NB_MAX_VISIBLE_SUGGESTION_BUTTONS));
    assert_suggestions(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);
    assert_only_displayed_suggestions_trigger(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);
}

void test_suggestions_updated(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(0));

    // 0 -> full
    TEST_ASSERT_GREATER_OR_EQUAL(0, update_suggestions_keyboard(NB_MAX_VISIBLE_SUGGESTION_BUTTONS));
    assert_suggestions(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);
    assert_only_displayed_suggestions_trigger(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);

    // full -> 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, update_suggestions_keyboard(0));
    assert_suggestions(0, 0);
    assert_only_displayed_suggestions_trigger(0, 0);

    // 0 -> 1
    TEST_ASSERT_GREATER_OR_EQUAL(0, update_suggestions_keyboard(1));
    assert_suggestions(0, 1);
    assert_only_displayed_suggestions_trigger(0, 1);
}

// with an odd number of suggestions, the last page is partially used
void test_suggestions_swiped(void)
{
    const uint8_t nbUsed = NB_MAX_VISIBLE_SUGGESTION_BUTTONS + 1;
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(nbUsed));
    assert_suggestions(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);

    swipe(SWIPED_LEFT);
    assert_suggestions(NB_MAX_VISIBLE_SUGGESTION_BUTTONS, 1);
    assert_only_displayed_suggestions_trigger(NB_MAX_VISIBLE_SUGGESTION_BUTTONS, 1);

    swipe(SWIPED_RIGHT);
    assert_suggestions(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);
    assert_only_displayed_suggestions_trigger(0, NB_MAX_VISIBLE_SUGGESTION_BUTTONS);
}

// suggestions of a previous keyboard must never be displayed in a new one
void test_suggestions_not_kept_from_previous_keyboard(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(NB_MAX_SUGGESTION_BUTTONS));
    setUp();
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(0));
    assert_suggestions(0, 0);
    swipe(SWIPED_LEFT);
    assert_suggestions(0, 0);
}

void test_too_many_suggestions_are_rejected(void)
{
    content = (nbgl_layoutKeyboardContent_t){
        .type                               = KEYBOARD_WITH_SUGGESTIONS,
        .title                              = "Enter word",
        .text                               = "",
        .textToken                          = CROSS_TOKEN,
        .suggestionButtons.buttons          = suggestions,
        .suggestionButtons.firstButtonToken = FIRST_SUGGESTION_TK,
        .suggestionButtons.nbUsedButtons    = NB_MAX_SUGGESTION_BUTTONS + 1,
        .tuneId                             = NBGL_NO_TUNE,
    };
    TEST_ASSERT_LESS_THAN(0,
                          nbgl_layoutAddKeyboardContent((nbgl_layout_t *) &layout_ctx, &content));

    setUp();
    TEST_ASSERT_GREATER_OR_EQUAL(0, add_suggestions_keyboard(1));
    content.suggestionButtons.nbUsedButtons = NB_MAX_SUGGESTION_BUTTONS + 1;
    TEST_ASSERT_LESS_THAN(
        0, nbgl_layoutUpdateKeyboardContent((nbgl_layout_t *) &layout_ctx, &content));
    // previous state is kept
    assert_suggestions(0, 1);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_active_button_triggers_callback);
    RUN_TEST(test_button_created_inactive_is_disabled);
    RUN_TEST(test_button_deactivated_is_disabled);
    RUN_TEST(test_late_release_after_deactivation_is_ignored);
    RUN_TEST(test_button_reactivated_triggers_callback);
    RUN_TEST(test_button_created_inactive_then_activated_triggers_callback);
    RUN_TEST(test_suggestions_created_empty_are_disabled);
    RUN_TEST(test_suggestions_created_partially_used);
    RUN_TEST(test_suggestions_created_full);
    RUN_TEST(test_suggestions_updated);
    RUN_TEST(test_suggestions_swiped);
    RUN_TEST(test_suggestions_not_kept_from_previous_keyboard);
    RUN_TEST(test_too_many_suggestions_are_rejected);
    return UNITY_END();
}
