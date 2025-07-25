
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#pragma once

#if defined(HAVE_BOLOS)
#include "bolos_privileged_ux.h"
#endif  // HAVE_BOLOS

#include "bolos_target.h"
#include "lcx_ecfp.h"
#include "os_math.h"
#include "os_ux.h"
#include "os_task.h"
#include "os_screen.h"

#ifndef HAVE_BOLOS_UX
#ifndef HAVE_UX_FLOW
#define COMPLIANCE_UX_160
#define HAVE_UX_LEGACY
#endif  // HAVE_UX_FLOW
#endif  // HAVE_BOLOS_UX

#include "ux_layouts.h"
#include "ux_flow_engine.h"
#if defined(HAVE_INDEXED_STRINGS)
#include "ux_loc.h"
#include "bolos_ux_loc_strings.h"
#include "ux_loc_layouts.h"
#include "ux_loc_flow_engine.h"
#endif  // defined(HAVE_INDEXED_STRINGS)

#include "bagl.h"
#include <string.h>

typedef struct bagl_element_e bagl_element_t;

// callback returns NULL when element must not be redrawn (with a changing color or what so ever)
typedef const bagl_element_t *(*bagl_element_callback_t)(const bagl_element_t *element);

// a graphic element is an element with defined text and actions depending on user touches
struct bagl_element_e {
    bagl_component_t component;

#if defined(HAVE_INDEXED_STRINGS)
    // Nameless union, to be able to access one member of the union or the other.
    // No space won when using index with bagl_element_e, but headaches are avoided :)
    union {
        const char          *text;
        UX_LOC_STRINGS_INDEX index;
    };
#else   // defined(HAVE_INDEXED_STRINGS)
    const char *text;
#endif  // defined(HAVE_INDEXED_STRINGS)
};

// When not using indexed strings, some functions can be inlined, to save space
#if !defined(HAVE_INDEXED_STRINGS)
#define STATIC_IF_NOT_INDEXED static
#else
#define STATIC_IF_NOT_INDEXED
#endif

// touch management helper function (callback the call with the element for the given position,
// taking into account touch release)
void io_seproxyhal_touch(const bagl_element_t *elements,
                         unsigned short        element_count,
                         unsigned short        x,
                         unsigned short        y,
                         unsigned char         event_kind);
void io_seproxyhal_touch_element_callback(const bagl_element_t   *elements,
                                          unsigned short          element_count,
                                          unsigned short          x,
                                          unsigned short          y,
                                          unsigned char           event_kind,
                                          bagl_element_callback_t before_display);
// callback to be implemented by the se
void io_seproxyhal_touch_callback(const bagl_element_t *element, unsigned char event);

/**
 * Common strings prepro tosave space
 */
const bagl_element_t *ux_layout_strings_prepro(const bagl_element_t *element);

// meta type to share amongst all multiline layouts
typedef struct ux_layout_strings_params_s {
    const char *lines[5];
} ux_layout_strings_params_t;

// meta type to share amongst all icon + multiline layouts
typedef struct ux_layout_icon_strings_params_s {
    const bagl_icon_details_t *icon;
    const char                *lines[5];
} ux_layout_icon_strings_params_t;

#if defined(HAVE_INDEXED_STRINGS)
const bagl_element_t *ux_loc_layout_strings_prepro(const bagl_element_t *element);

// Prototypes related to localization
const bagl_icon_details_t *get_glyphs_icon(unsigned char id);
const char                *get_string_buffer(unsigned char id);
const char                *get_ux_loc_string(UX_LOC_STRINGS_INDEX index);
#endif  // defined(HAVE_INDEXED_STRINGS)
#if defined(HAVE_LANGUAGE_PACK)
void bolos_ux_select_language(uint16_t language);
void bolos_ux_refresh_language(void);

typedef struct ux_loc_language_pack_infos {
    unsigned char available;

} UX_LOC_LANGUAGE_PACK_INFO;

// To populate infos about language packs
SYSCALL PERMISSION(APPLICATION_FLAG_BOLOS_UX) void list_language_packs(
    UX_LOC_LANGUAGE_PACK_INFO *packs PLENGTH(NB_LANG * sizeof(UX_LOC_LANGUAGE_PACK_INFO)));
SYSCALL PERMISSION(APPLICATION_FLAG_BOLOS_UX) const LANGUAGE_PACK *get_language_pack(
    unsigned int language);
#endif  // defined(HAVE_LANGUAGE_PACK)

#ifndef BUTTON_FAST_THRESHOLD_CS
#define BUTTON_FAST_THRESHOLD_CS 8  // x100MS
#endif                              // BUTTON_FAST_THRESHOLD_CS
#ifndef BUTTON_FAST_ACTION_CS
#define BUTTON_FAST_ACTION_CS 3  // x100MS
#endif                           // BUTTON_FAST_ACTION_CS

typedef unsigned int (*button_push_callback_t)(unsigned int button_mask,
                                               unsigned int button_mask_counter);
#define BUTTON_LEFT         1
#define BUTTON_RIGHT        2
// flag set when fast threshold is reached and above
#define BUTTON_EVT_FAST     0x40000000UL
#define BUTTON_EVT_RELEASED 0x80000000UL
void io_seproxyhal_button_push(button_push_callback_t button_push_callback,
                               unsigned int           new_button_mask);

// hal point (if application has to reprocess elements)
void io_seproxyhal_display(const bagl_element_t *element);

// Helper function that give a realistic timing of scrolling for label with text larger than screen
unsigned int bagl_label_roundtrip_duration_ms(const bagl_element_t *e,
                                              unsigned int          average_char_width);
unsigned int bagl_label_roundtrip_duration_ms_buf(const bagl_element_t *e,
                                                  const char           *str,
                                                  unsigned int          average_char_width);

// default version to be called by ::io_seproxyhal_display if nothing to be done by the application
void io_seproxyhal_display_default(const bagl_element_t *element);

#ifndef UX_STACK_SLOT_COUNT
#define UX_STACK_SLOT_COUNT 1
#endif  // UX_STACK_SLOT_COUNT

#ifndef UX_STACK_SLOT_ARRAY_COUNT
#define UX_STACK_SLOT_ARRAY_COUNT 1
#endif  // UX_STACK_SLOT_ARRAY_COUNT

typedef unsigned int (*callback_int_t)(unsigned int);

typedef struct ux_stack_slot_s ux_stack_slot_t;

/**
 * Common structure for applications to perform asynchronous UX aside IO operations
 */
typedef struct ux_state_s ux_state_t;

// returns 0 if the element_array is not found, else stack_index + 1 if the element_array is found
unsigned int ux_stack_is_element_array_present(const bagl_element_t *element_array);

// push if a slot exists and returns the new slot, otherwise returns the top spot
unsigned int ux_stack_push(void);

// pops the top slot exists and returns the new slot, then returns the new top slot
unsigned int ux_stack_pop(void);

// inserts a new slot at the stack_slot
void ux_stack_insert(unsigned int stack_slot);  // insert slot space as given index

// removes the slot at the stack_slot
void ux_stack_remove(unsigned int stack_slot);

void ux_stack_init(unsigned int stack_slot);

// display the slot at index stack_slot
void ux_stack_display(unsigned int stack_slot);

/** Function to be implemented by the UX manager (to allow specific callback and processing of the
 * target) The next displayable element of the given stack slot must be displayed
 */
void ux_stack_al_display_next_element(unsigned int stack_slot);

// redisplay the top stacked slot.
void ux_stack_redisplay(void);

const bagl_element_t *ux_stack_display_element_callback(const bagl_element_t *element);
void                  ux_stack_display_elements(ux_stack_slot_t *slot);

#ifdef HAVE_UX_LEGACY
// a menu callback is called with a given userid provided within the menu entry to allow for fast
// switch of the action to be taken
typedef void (*ux_menu_callback_t)(unsigned int userid);

typedef struct ux_menu_entry_s ux_menu_entry_t;

/**
 * Menu entry descriptor.
 */
struct ux_menu_entry_s {
    // other menu shown when validated
    const ux_menu_entry_t *menu;
    // callback called when entered (not executed when a menu entry is present)
    ux_menu_callback_t callback;
    // user identifier to allow for indirection in a separated table and mutualise even more menu
    // handling, passed to the given callback is any
    unsigned int               userid;
    const bagl_icon_details_t *icon;
    const char                *line1;
    const char                *line2;
    char                       text_x;
    char                       icon_x;
};

typedef const bagl_element_t *(*ux_menu_preprocessor_t)(const ux_menu_entry_t *,
                                                        bagl_element_t *element);
typedef const ux_menu_entry_t *(*ux_menu_iterator_t)(unsigned int entry_idx);
typedef struct ux_menu_state_s {
    const ux_menu_entry_t *menu_entries;
    unsigned int           menu_entries_count;
    unsigned int           current_entry;
    ux_menu_preprocessor_t menu_entry_preprocessor;
    ux_menu_iterator_t     menu_iterator;
} ux_menu_state_t;

// a menu callback is called with a given userid provided within the menu entry to allow for fast
// switch of the action to be taken
typedef void (*ux_turner_callback_t)(void);

typedef struct ux_turner_step_s {
    const bagl_icon_details_t *icon;
    unsigned short             fontid1;
    const char                *line1;
    unsigned short             fontid2;
    const char                *line2;
    char                       text_x;
    char                       icon_x;
    unsigned int               next_step_ms;
} ux_turner_step_t;

typedef struct ux_turner_state_s {
    const ux_turner_step_t *steps;
    unsigned int            steps_count;
    unsigned int            current_step;
    button_push_callback_t  button_callback;
    unsigned int            elapsed_ms;
} ux_turner_state_t;
#endif  // HAVE_UX_LEGACY

struct ux_stack_slot_s {
    // arrays of element to be displayed (to automate when dealing with static and dynamic elements)
    bolos_task_status_t exit_code_after_elements_displayed;
    unsigned char       element_arrays_count;
    unsigned short      element_index;
    // unsigned char displayed;
    struct {
        const bagl_element_t *element_array;
        unsigned char         element_array_count;
    } element_arrays[UX_STACK_SLOT_ARRAY_COUNT];

#if defined(HAVE_UX_FLOW)
    callback_int_t displayed_callback;
#endif  // defined(HAVE_UX_FLOW)
    // callback called before the screen callback to change the keyboard face
    bagl_element_callback_t screen_before_element_display_callback;
    button_push_callback_t  button_push_callback;

    callback_int_t ticker_callback;
    unsigned int   ticker_value;
    unsigned int   ticker_interval;
};

struct ux_state_s {
    unsigned char       stack_count;  // initialized @0 by the bolos ux initialize
    bolos_task_status_t exit_code;

#ifdef HAVE_UX_FLOW
    // global context, therefore, don't allow for multiple paging overlaid in a graphic stack
    ux_layout_paging_state_t layout_paging;

    // the flow for each stack slot
    ux_flow_state_t flow_stack[UX_STACK_SLOT_COUNT];

#endif  // HAVE_UX_FLOW

#if defined(HAVE_UX_FLOW)
    // after an int to make sure it's aligned
    char string_buffer[MAX(128, sizeof(bagl_icon_details_t) - 1)];
#endif  // defined(HAVE_UX_FLOW)

    bagl_element_t tmp_element;

    // unified arrays
    // maxstack: [onboarding/dashboard/settings] | pairing | pin | batterylow | batterycrit |
    // screensaver
    ux_stack_slot_t stack[UX_STACK_SLOT_COUNT];

#ifdef HAVE_UX_FLOW
    // for menulist display
    unsigned int               menulist_current;
    ux_layout_strings_params_t menulist_params;
    list_item_value_t          menulist_getter;
    list_item_select_t         menulist_selector;
#endif  // HAVE_UX_FLOW

#ifdef COMPLIANCE_UX_160
    bolos_ux_params_t params;
#endif  // COMPLIANCE_UX_160

    char *externalText;
};

#ifdef COMPLIANCE_UX_160

#define G_ux                 ux
#define G_ux_params          ux.params
#define callback_interval_ms stack[0].ticker_interval
#define UX_INIT()                   \
    memset(&G_ux, 0, sizeof(G_ux)); \
    ux_stack_push();
extern ux_state_t G_ux;

#else  // COMPLIANCE_UX_160

extern ux_state_t        G_ux;
extern bolos_ux_params_t G_ux_params;

/**
 * Initialize the user experience structure
 */
#define UX_INIT() memset(&G_ux, 0, sizeof(G_ux));

#endif  // COMPLIANCE_UX_160

/**
 * Request displaying the next element in the UX structure.
 * Take into account if a seproxyhal status has already been issued.
 * Take into account if the next element is allowed/denied for display by the registered
 * preprocessor
 */
#define UX_DISPLAY_NEXT_ELEMENT()                                                                 \
    if (G_ux.stack[0].element_arrays[0].element_array                                             \
        && G_ux.stack[0].element_index < G_ux.stack[0].element_arrays[0].element_array_count      \
        && (os_perso_isonboarded() != BOLOS_UX_OK                                                 \
            || os_global_pin_is_validated() == BOLOS_UX_OK)) {                                    \
        while (G_ux.stack[0].element_index                                                        \
               < G_ux.stack[0].element_arrays[0].element_array_count) {                           \
            const bagl_element_t *element                                                         \
                = &G_ux.stack[0].element_arrays[0].element_array[G_ux.stack[0].element_index];    \
            if (!G_ux.stack[0].screen_before_element_display_callback                             \
                || (element = G_ux.stack[0].screen_before_element_display_callback(element))) {   \
                if ((unsigned int) element                                                        \
                    == 1) { /*backward compat with coding to avoid smashing everything*/          \
                    element = &G_ux.stack[0]                                                      \
                                   .element_arrays[0]                                             \
                                   .element_array[G_ux.stack[0].element_index];                   \
                }                                                                                 \
                io_seph_ux_display_bagl_element(element);                                         \
            }                                                                                     \
            G_ux.stack[0].element_index++;                                                        \
        }                                                                                         \
        if (G_ux.stack[0].element_index == G_ux.stack[0].element_arrays[0].element_array_count) { \
            screen_update();                                                                      \
        }                                                                                         \
    }

/**
 * Request a wake up of the device (backlight, pin lock screen, ...) to display a new interface to
 * the user. Wake up prevent both autolock and power off features. Therefore, security wise, this
 * function shall only be called to request direct user interaction.
 */
#define UX_WAKE_UP()                      \
    G_ux_params.ux_id = BOLOS_UX_WAKE_UP; \
    G_ux_params.len   = 0;                \
    os_ux(&G_ux_params);                  \
    G_ux_params.len = os_sched_last_status(TASK_BOLOS_UX);

/**
 * Redisplay request (no immediate display status sent)
 */
#define UX_REDISPLAY_REQUEST() \
    io_seph_ux_init_button();  \
    G_ux.stack[0].element_index = 0;

/**
 * Force redisplay of the screen from the given index in the screen's element array
 */
#define UX_REDISPLAY_IDX(index)                                                             \
    io_seph_ux_init_button(); /*ensure to avoid release of a button from a nother screen to \
                                    mess up with the redisplayed screen */                  \
    G_ux.stack[0].element_index = index;                                                    \
    /* REDRAW is redisplay already, use os_ux return value to check */                      \
    G_ux_params.len = os_sched_last_status(TASK_BOLOS_UX);                                  \
    if (G_ux_params.len != BOLOS_UX_IGNORE && G_ux_params.len != BOLOS_UX_CONTINUE) {       \
        UX_DISPLAY_NEXT_ELEMENT();                                                          \
    }

/**
 * Redisplay all elements of the screen
 */
#define UX_REDISPLAY() UX_REDISPLAY_IDX(0)

#define UX_DISPLAY(elements_array, preprocessor)                                    \
    G_ux.stack[0].element_arrays[0].element_array = elements_array;                 \
    G_ux.stack[0].element_arrays[0].element_array_count                             \
        = sizeof(elements_array) / sizeof(elements_array[0]);                       \
    G_ux.stack[0].button_push_callback                   = elements_array##_button; \
    G_ux.stack[0].screen_before_element_display_callback = preprocessor;            \
    UX_WAKE_UP();                                                                   \
    UX_REDISPLAY();

/**
 * Request the given UX to be redisplayed without emitting a display status right now (to continue
 * current operation, like transferring an USB reply)
 */
#define UX_DISPLAY_REQUEST(elements_array, preprocessor)                            \
    G_ux.stack[0].element_arrays[0].element_array = elements_array;                 \
    G_ux.stack[0].element_arrays[0].element_array_count                             \
        = sizeof(elements_array) / sizeof(elements_array[0]);                       \
    G_ux.stack[0].button_push_callback                   = elements_array##_button; \
    G_ux.stack[0].screen_before_element_display_callback = preprocessor;            \
    UX_WAKE_UP();

/**
 * Request a screen redisplay after the given milliseconds interval has passed. Interval is not
 * repeated, it's a single shot callback. must be re-enabled (the JS way).
 */
#define UX_CALLBACK_SET_INTERVAL(ms) G_ux.stack[0].ticker_value = ms;

/**
 * internal bolos ux event processing with callback in case event is to be processed by the
 * application
 */
#define UX_FORWARD_EVENT(callback, ignoring_app_if_ux_busy)                                     \
    G_ux_params.ux_id = BOLOS_UX_EVENT;                                                         \
    G_ux_params.len   = 0;                                                                      \
    os_ux(&G_ux_params);                                                                        \
    G_ux_params.len = os_sched_last_status(TASK_BOLOS_UX);                                      \
    if (G_ux_params.len == BOLOS_UX_REDRAW) {                                                   \
        UX_REDISPLAY();                                                                         \
    }                                                                                           \
    else if (!ignoring_app_if_ux_busy                                                           \
             || (G_ux_params.len != BOLOS_UX_IGNORE && G_ux_params.len != BOLOS_UX_CONTINUE)) { \
        callback;                                                                               \
    }

#define UX_CONTINUE_DISPLAY_APP(displayed_callback)                                           \
    UX_DISPLAY_NEXT_ELEMENT();                                                                \
    /* all items have been displayed */                                                       \
    if (G_ux.stack[0].element_index >= G_ux.stack[0].element_arrays[0].element_array_count) { \
        displayed_callback                                                                    \
    }

/**
 * Process display processed event (by the os_ux or by the application code)
 */
#define UX_DISPLAYED_EVENT(displayed_callback) \
    UX_FORWARD_EVENT({ UX_CONTINUE_DISPLAY_APP(displayed_callback); }, 1)

/**
 * Deprecated version to be removed
 */
#define UX_DISPLAYED() \
    (G_ux.stack[0].element_index >= G_ux.stack[0].element_arrays[0].element_array_count)

/**
 * Macro to process sequentially display a screen. The call finishes when the UX is completely
 * displayed, and the state of the MCU <-> SE exchanges is the same as before this macro call.
 */
#define UX_WAIT_DISPLAYED()        \
    while (!UX_DISPLAYED()) {      \
        UX_DISPLAY_NEXT_ELEMENT(); \
    }

/**
 * Process button push events. Application's button event handler is called only if the ux app does
 * not deny it (modal frame displayed).
 */
#define UX_BUTTON_PUSH_EVENT(seph_packet)                                     \
    UX_FORWARD_EVENT(                                                         \
        {                                                                     \
            if (G_ux.stack[0].button_push_callback) {                         \
                io_seproxyhal_button_push(G_ux.stack[0].button_push_callback, \
                                          seph_packet[3] >> 1);               \
            }                                                                 \
            UX_CONTINUE_DISPLAY_APP({});                                      \
        },                                                                    \
        1);

#define UX_FINGER_EVENT(seph_packet)
/**
 * forward the ticker_event to the os ux handler. Ticker event callback is always called whatever
 * the return code of the ux app. Ticker event interval is assumed to be 100 ms.
 */
#define UX_TICKER_EVENT(seph_packet, callback)                                                  \
    UX_FORWARD_EVENT(                                                                           \
        {                                                                                       \
            unsigned int UX_ALLOWED                                                             \
                = (G_ux_params.len != BOLOS_UX_IGNORE && G_ux_params.len != BOLOS_UX_CONTINUE); \
            if (G_ux.stack[0].ticker_value) {                                                   \
                G_ux.stack[0].ticker_value -= MIN(G_ux.stack[0].ticker_value, 100);             \
                if (!G_ux.stack[0].ticker_value) {                                              \
                    if (!G_ux.stack[0].ticker_callback) {                                       \
                        callback                                                                \
                    }                                                                           \
                    else {                                                                      \
                        G_ux.stack[0].ticker_value = G_ux.stack[0].ticker_interval;             \
                        G_ux.stack[0].ticker_callback(0);                                       \
                    }                                                                           \
                }                                                                               \
            }                                                                                   \
            if (UX_ALLOWED) {                                                                   \
                UX_CONTINUE_DISPLAY_APP({});                                                    \
            }                                                                                   \
        },                                                                                      \
        0);

/**
 * Forward the event, ignoring the UX return code, the event must therefore be either not processed
 * or processed with extreme care by the application afterwards
 */
#define UX_DEFAULT_EVENT() UX_FORWARD_EVENT({ UX_CONTINUE_DISPLAY_APP({}); }, 0);

/**
 * Start displaying the system keyboard input to allow. keyboard entry ends when any ux call returns
 * with an OK status.
 */
#define UX_DISPLAY_KEYBOARD(callback)      \
    G_ux_params.ux_id = BOLOS_UX_KEYBOARD; \
    G_ux_params.len   = 0;                 \
    os_ux(&G_ux_params);                   \
    G_ux_params.len = os_sched_last_status(TASK_BOLOS_UX);

// discriminated from io to allow for different memory placement
typedef struct ux_seph_s {
    unsigned int button_mask;
    unsigned int button_same_mask_counter;
#ifdef HAVE_BOLOS
    unsigned int ux_id;
    unsigned int ux_status;
#endif  // HAVE_BOLOS
} ux_seph_os_and_app_t;

extern ux_seph_os_and_app_t G_ux_os;

#ifdef HAVE_UX_LEGACY
#define UX_MENU_END                           \
    {                                         \
        NULL, NULL, 0, NULL, NULL, NULL, 0, 0 \
    }

#define UX_MENU_INIT() memset(&ux_menu, 0, sizeof(ux_menu));

#define UX_MENU_DISPLAY(current_entry, menu_entries, menu_entry_preprocessor) \
    ux_menu_display(current_entry, menu_entries, menu_entry_preprocessor);

// if current_entry == -1UL, then don't change the current entry
#define UX_MENU_UNCHANGED_ENTRY (-1UL)
void                  ux_menu_display(unsigned int           current_entry,
                                      const ux_menu_entry_t *menu_entries,
                                      ux_menu_preprocessor_t menu_entry_preprocessor);
const bagl_element_t *ux_menu_element_preprocessor(const bagl_element_t *element);
unsigned int ux_menu_elements_button(unsigned int button_mask, unsigned int button_mask_counter);
extern ux_menu_state_t ux_menu;

#define UX_TURNER_INIT() memset(&ux_turner, 0, sizeof(ux_turner));

#define UX_TURNER_DISPLAY(current_step, steps, steps_count, button_push_callback) \
    ux_turner_display(current_step, steps, steps_count, button_push_callback);

// if current_entry == -1UL, then don't change the current entry
#define UX_TURNER_UNCHANGED_ENTRY (-1UL)
void ux_turner_display(unsigned int            current_step,
                       const ux_turner_step_t *steps,
                       unsigned int            steps_count,
                       button_push_callback_t  button_callback);
// function to be called to advance to the next turner step when the programmed delay is expired
void ux_turner_ticker(unsigned int elpased_ms);

extern ux_turner_state_t ux_turner;
#endif  // HAVE_UX_LEGACY

#ifdef HAVE_UX_LEGACY
// current ux_menu context (could be pluralised if multiple nested levels of menu are required
// within bolos_ux for example)
#ifdef BOLOS_RELEASE
#ifdef TARGET_NANOX
#error HAVE_UX_LEGACY must be removed in the release
#else
#warning Refactor UX plz
#endif  // TARGET_NANOX
#endif  // BOLOS_RELEASE
#endif  // HAVE_UX_LEGACY

#include "glyphs.h"
