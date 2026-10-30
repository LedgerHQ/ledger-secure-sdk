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

#define BUTTON_TOKEN 0x42
#define CROSS_TOKEN  0x43

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
    content = (nbgl_layoutKeyboardContent_t) {
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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_active_button_triggers_callback);
    RUN_TEST(test_button_created_inactive_is_disabled);
    RUN_TEST(test_button_deactivated_is_disabled);
    RUN_TEST(test_late_release_after_deactivation_is_ignored);
    RUN_TEST(test_button_reactivated_triggers_callback);
    RUN_TEST(test_button_created_inactive_then_activated_triggers_callback);
    return UNITY_END();
}
