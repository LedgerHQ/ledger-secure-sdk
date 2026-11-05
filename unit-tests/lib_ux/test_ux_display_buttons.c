#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "os.h"
#include "ux.h"

/**
 * Stubs
 */

ux_state_t                G_ux;
bolos_ux_params_t         G_ux_params;
const bagl_icon_details_t C_icon_left;
const bagl_icon_details_t C_icon_right;

void bagl_draw_with_context(const bagl_component_t *component,
                            const void             *text,
                            unsigned short          text_length,
                            unsigned char           text_encoding)
{
    (void) component;
    (void) text;
    (void) text_length;
    (void) text_encoding;
}

void bagl_draw_glyph(const bagl_component_t *component, const bagl_icon_details_t *icon_details)
{
    (void) component;
    (void) icon_details;
}

unsigned int os_io_handle_ux_event_reject_apdu(void)
{
    return 0;
}

bolos_task_status_t os_sched_last_status(unsigned int task_idx)
{
    (void) task_idx;
    return 0;
}

unsigned int os_ux(bolos_ux_params_t *params)
{
    (void) params;
    return 0;
}

bolos_bool_t os_perso_isonboarded(void)
{
    return BOLOS_TRUE;
}

bolos_bool_t os_global_pin_is_validated(void)
{
    return BOLOS_TRUE;
}

void screen_update(void) {}

unsigned int ux_layout_paging_compute(const char               *text_to_split,
                                      unsigned int              page_to_display,
                                      ux_layout_paging_state_t *paging_state,
                                      bagl_font_id_e            font)
{
    (void) text_to_split;
    (void) page_to_display;
    (void) paging_state;
    (void) font;
    return 1;
}

/**
 * Test screens, displayed with the BAGL screen macros (UX_DISPLAY & co) that applications using
 * flows may still use for custom screens
 */

static unsigned int nb_home_events;  // releases received by the home screen
static unsigned int nb_approvals;

// screen displayed before the host request
static const bagl_element_t screen_home[] = {
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
     .text = NULL},
};

static unsigned int screen_home_button(unsigned int button_mask, unsigned int button_mask_counter)
{
    (void) button_mask_counter;
    if (button_mask & BUTTON_EVT_RELEASED) {
        nb_home_events++;
    }
    return 0;
}

// confirmation screen displayed on host request, approved by a dual-button release
static const bagl_element_t screen_confirm[] = {
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
     .text = NULL},
};

static unsigned int screen_confirm_button(unsigned int button_mask,
                                          unsigned int button_mask_counter)
{
    (void) button_mask_counter;
    if (button_mask == (BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT)) {
        nb_approvals++;
    }
    return 0;
}

static void buttons(unsigned int mask)
{
    io_seproxyhal_button_push(G_ux.stack[0].button_push_callback, mask);
}

static void click(unsigned int mask)
{
    buttons(mask);
    buttons(0);
}

static void display_home(void)
{
    UX_DISPLAY(screen_home, NULL);
}

static void display_confirm(void)
{
    UX_DISPLAY(screen_confirm, NULL);
}

static void display_confirm_request(void)
{
    UX_DISPLAY_REQUEST(screen_confirm, NULL);
}

void setUp(void)
{
    memset(&G_ux, 0, sizeof(G_ux));
    memset(&G_ux_os, 0, sizeof(G_ux_os));
    G_ux.stack_count = 1;
    nb_home_events   = 0;
    nb_approvals     = 0;
    display_home();
}

void tearDown(void) {}

/**
 * Tests
 */

// V-395: a dual-button gesture started on the previous screen must not approve a confirmation
// displayed by UX_DISPLAY while it is held, even with held-button updates after the replacement
void test_ux_display_discards_held_gesture(void)
{
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    display_confirm();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(0);
    TEST_ASSERT_EQUAL_UINT(0, nb_approvals);

    // a fresh gesture on the confirmation screen approves it
    click(BUTTON_LEFT | BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_approvals);
}

// V-395: same with UX_DISPLAY_REQUEST, which used to switch the callback without any discard
void test_ux_display_request_discards_held_gesture(void)
{
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    display_confirm_request();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(0);
    TEST_ASSERT_EQUAL_UINT(0, nb_approvals);

    click(BUTTON_LEFT | BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_approvals);
}

// a single held button completed into a chord after the replacement, then partially released
void test_ux_display_discards_partial_releases(void)
{
    buttons(BUTTON_LEFT);
    display_confirm();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(BUTTON_RIGHT);
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(0);
    TEST_ASSERT_EQUAL_UINT(0, nb_approvals);

    click(BUTTON_LEFT | BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_approvals);
}

// a discard in progress spans several screen replacements, until the final release
void test_ux_display_discard_spans_replacements(void)
{
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    display_confirm();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    display_home();
    display_confirm();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(0);
    TEST_ASSERT_EQUAL_UINT(0, nb_approvals);

    click(BUTTON_LEFT | BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_approvals);
}

// UX_REDISPLAY (e.g. on BOLOS_UX_REDRAW after an OS modal) also discards the ongoing gesture
void test_ux_redisplay_discards_held_gesture(void)
{
    display_confirm();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    UX_REDISPLAY();
    buttons(BUTTON_LEFT | BUTTON_RIGHT);
    buttons(0);
    TEST_ASSERT_EQUAL_UINT(0, nb_approvals);

    click(BUTTON_LEFT | BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_approvals);
}

// a screen displayed after a complete gesture (all buttons released) handles the next one
void test_ux_display_after_release_keeps_working(void)
{
    click(BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_home_events);
    display_confirm();
    click(BUTTON_LEFT | BUTTON_RIGHT);
    TEST_ASSERT_EQUAL_UINT(1, nb_approvals);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ux_display_discards_held_gesture);
    RUN_TEST(test_ux_display_request_discards_held_gesture);
    RUN_TEST(test_ux_display_discards_partial_releases);
    RUN_TEST(test_ux_display_discard_spans_replacements);
    RUN_TEST(test_ux_redisplay_discards_held_gesture);
    RUN_TEST(test_ux_display_after_release_keeps_working);
    return UNITY_END();
}
