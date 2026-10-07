#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "nbgl_page.h"
#include "nbgl_layout_internal.h"
#include "glyphs.h"

// Regression tests for V-134: on the confirmation page, the confirmation button must report the
// confirmation token and the cancel button must report the cancel token, so that a tap on Cancel
// can never be taken for a confirmation.

#define CONFIRMATION_TOKEN 0x31
#define CANCEL_TOKEN       0x72

// ============================================================================
// Glyphs
// ============================================================================

const nbgl_icon_details_t C_Check_Circle_64px;
const nbgl_icon_details_t C_Chevron_40px;
const nbgl_icon_details_t C_Close_40px;
const nbgl_icon_details_t C_Info_40px;
const nbgl_icon_details_t C_Settings_40px;

// ============================================================================
// NBGL layout stubs
// ============================================================================

static nbgl_layoutInternal_t layout_ctx;
static nbgl_button_t         choice_buttons[2];

nbgl_layout_t *nbgl_layoutGet(const nbgl_layoutDescription_t *description)
{
    memset(&layout_ctx, 0, sizeof(layout_ctx));
    memset(choice_buttons, 0, sizeof(choice_buttons));
    layout_ctx.callback = description->onActionCallback;
    layout_ctx.modal    = description->modal;
    return (nbgl_layout_t *) &layout_ctx;
}

static layoutObj_t *add_callback_obj(nbgl_obj_t *obj, uint8_t token, uint8_t index)
{
    TEST_ASSERT_LESS_THAN(LAYOUT_OBJ_POOL_LEN, layout_ctx.nbUsedCallbackObjs);
    layoutObj_t *layoutObj = &layout_ctx.callbackObjPool[layout_ctx.nbUsedCallbackObjs++];
    layoutObj->obj         = obj;
    layoutObj->token       = token;
    layoutObj->index       = index;
    return layoutObj;
}

// Same callback objects as the real FOOTER_CHOICE_BUTTONS construction: the bottom button is
// registered first with index 1, then the top button with index 0, both with the single token.
int nbgl_layoutAddChoiceButtons(nbgl_layout_t *layout, const nbgl_layoutChoiceButtons_t *info)
{
    (void) layout;
    choice_buttons[0].obj.touchId = CHOICE_2_ID;
    add_callback_obj(&choice_buttons[0].obj, info->token, 1);
    choice_buttons[1].obj.touchId = CHOICE_1_ID;
    add_callback_obj(&choice_buttons[1].obj, info->token, 0);
    return 0;
}

int nbgl_layoutAddHeader(nbgl_layout_t *layout, const nbgl_layoutHeader_t *headerDesc)
{
    (void) layout;
    (void) headerDesc;
    return 0;
}

int nbgl_layoutAddCenteredInfo(nbgl_layout_t *layout, const nbgl_layoutCenteredInfo_t *info)
{
    (void) layout;
    (void) info;
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

// Layout functions used by the other pages, not exercised here
#define UNUSED_LAYOUT_FUNC(name, ...)                              \
    int nbgl_layout##name(__VA_ARGS__)                             \
    {                                                              \
        TEST_FAIL_MESSAGE("unexpected call to nbgl_layout" #name); \
        return -1;                                                 \
    }

UNUSED_LAYOUT_FUNC(AddBottomButton,
                   nbgl_layout_t             *layout,
                   const nbgl_icon_details_t *icon,
                   uint8_t                    token,
                   bool                       separationLine,
                   tune_index_e               tuneId)
UNUSED_LAYOUT_FUNC(AddButton, nbgl_layout_t *layout, const nbgl_layoutButton_t *buttonInfo)
UNUSED_LAYOUT_FUNC(AddContentCenter, nbgl_layout_t *layout, const nbgl_contentCenter_t *info)
UNUSED_LAYOUT_FUNC(AddExtendedFooter, nbgl_layout_t *layout, const nbgl_layoutFooter_t *footerDesc)
UNUSED_LAYOUT_FUNC(AddFooter,
                   nbgl_layout_t *layout,
                   const char    *text,
                   uint8_t        token,
                   tune_index_e   tuneId)
UNUSED_LAYOUT_FUNC(AddHorizontalButtons,
                   nbgl_layout_t                        *layout,
                   const nbgl_layoutHorizontalButtons_t *info)
UNUSED_LAYOUT_FUNC(AddLongPressButton,
                   nbgl_layout_t *layout,
                   const char    *text,
                   uint8_t        token,
                   tune_index_e   tuneId)
UNUSED_LAYOUT_FUNC(AddProgressIndicator,
                   nbgl_layout_t *layout,
                   uint8_t        activePage,
                   uint8_t        nbPages,
                   bool           withBack,
                   uint8_t        backToken,
                   tune_index_e   tuneId)
UNUSED_LAYOUT_FUNC(AddRadioChoice, nbgl_layout_t *layout, const nbgl_layoutRadioChoice_t *choices)
UNUSED_LAYOUT_FUNC(AddSeparationLine, nbgl_layout_t *layout)
UNUSED_LAYOUT_FUNC(AddSpinner,
                   nbgl_layout_t *layout,
                   const char    *text,
                   const char    *subText,
                   uint8_t        initPosition)
UNUSED_LAYOUT_FUNC(AddSplitFooter,
                   nbgl_layout_t *layout,
                   const char    *leftText,
                   uint8_t        leftToken,
                   const char    *rightText,
                   uint8_t        rightToken,
                   tune_index_e   tuneId)
UNUSED_LAYOUT_FUNC(AddSwipe,
                   nbgl_layout_t *layout,
                   uint16_t       swipesMask,
                   const char    *text,
                   uint8_t        token,
                   tune_index_e   tuneId)
UNUSED_LAYOUT_FUNC(AddSwitch, nbgl_layout_t *layout, const nbgl_layoutSwitch_t *switchLayout)
UNUSED_LAYOUT_FUNC(AddTagValueList, nbgl_layout_t *layout, const nbgl_layoutTagValueList_t *list)
UNUSED_LAYOUT_FUNC(AddText, nbgl_layout_t *layout, const char *text, const char *subText)
UNUSED_LAYOUT_FUNC(AddTextWithAlias,
                   nbgl_layout_t *layout,
                   const char    *text,
                   const char    *subText,
                   uint8_t        token,
                   uint8_t        index)
UNUSED_LAYOUT_FUNC(AddTopRightButton,
                   nbgl_layout_t             *layout,
                   const nbgl_icon_details_t *icon,
                   uint8_t                    token,
                   tune_index_e               tuneId)
UNUSED_LAYOUT_FUNC(AddTouchableBar, nbgl_layout_t *layout, const nbgl_layoutBar_t *barLayout)
UNUSED_LAYOUT_FUNC(AddUpFooter, nbgl_layout_t *layout, const nbgl_layoutUpFooter_t *upFooterDesc)

// Simulates a touch on the choice button with the given touch ID, as the layout would do
static void touch_choice_button(uint8_t touchId)
{
    for (uint8_t i = 0; i < layout_ctx.nbUsedCallbackObjs; i++) {
        layoutObj_t *layoutObj = &layout_ctx.callbackObjPool[i];
        if (layoutObj->obj->touchId == touchId) {
            layout_ctx.callback(layoutObj->token, layoutObj->index);
            return;
        }
    }
    TEST_FAIL_MESSAGE("choice button not found");
}

// ============================================================================
// Helpers
// ============================================================================

static int     nb_actions;
static int     last_token;
static uint8_t last_index;

static void action_callback(int token, uint8_t index)
{
    nb_actions++;
    last_token = token;
    last_index = index;
}

static void draw_confirmation(uint8_t confirmationToken, uint8_t cancelToken)
{
    nbgl_pageConfirmationDescription_t info = {.centeredInfo.text1 = "Sign transaction?",
                                               .confirmationText   = "Sign",
                                               .cancelText         = "Reject",
                                               .confirmationToken  = confirmationToken,
                                               .cancelToken        = cancelToken,
                                               .tuneId             = NBGL_NO_TUNE};
    TEST_ASSERT_NOT_NULL(nbgl_pageDrawConfirmation(action_callback, &info));
}

void setUp(void)
{
    nb_actions = 0;
    last_token = -1;
    last_index = 0xFF;
}

void tearDown(void) {}

// ============================================================================
// Tests
// ============================================================================

void test_confirm_button_reports_confirmation_token(void)
{
    draw_confirmation(CONFIRMATION_TOKEN, CANCEL_TOKEN);

    touch_choice_button(CHOICE_1_ID);
    TEST_ASSERT_EQUAL_INT(1, nb_actions);
    TEST_ASSERT_EQUAL_INT(CONFIRMATION_TOKEN, last_token);
    TEST_ASSERT_EQUAL_UINT8(0, last_index);
}

void test_cancel_button_reports_cancel_token(void)
{
    draw_confirmation(CONFIRMATION_TOKEN, CANCEL_TOKEN);

    touch_choice_button(CHOICE_2_ID);
    TEST_ASSERT_EQUAL_INT(1, nb_actions);
    TEST_ASSERT_EQUAL_INT(CANCEL_TOKEN, last_token);
    TEST_ASSERT_EQUAL_UINT8(1, last_index);
}

// Callers sharing a single token for both buttons still tell them apart with the index
void test_shared_token_keeps_index(void)
{
    draw_confirmation(CONFIRMATION_TOKEN, CONFIRMATION_TOKEN);

    touch_choice_button(CHOICE_1_ID);
    TEST_ASSERT_EQUAL_INT(CONFIRMATION_TOKEN, last_token);
    TEST_ASSERT_EQUAL_UINT8(0, last_index);

    touch_choice_button(CHOICE_2_ID);
    TEST_ASSERT_EQUAL_INT(CONFIRMATION_TOKEN, last_token);
    TEST_ASSERT_EQUAL_UINT8(1, last_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_confirm_button_reports_confirmation_token);
    RUN_TEST(test_cancel_button_reports_cancel_token);
    RUN_TEST(test_shared_token_keeps_index);
    return UNITY_END();
}
