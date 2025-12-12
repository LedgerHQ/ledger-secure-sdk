
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

#include "os_math.h"
#include "os_ux.h"
#include "os_task.h"
#include "nbgl_screen.h"
#include "nbgl_touch.h"
#include "seproxyhal_protocol.h"

#include <string.h>

#define BUTTON_LEFT  SEPROXYHAL_TAG_BUTTON_PUSH_EVENT_LEFT
#define BUTTON_RIGHT SEPROXYHAL_TAG_BUTTON_PUSH_EVENT_RIGHT

/**
 * Common structure for applications to perform asynchronous UX aside IO operations
 */
typedef struct ux_state_s ux_state_t;

struct ux_state_s {
    bolos_task_status_t exit_code;
    bool validate_pin_from_dashboard;  // set to true when BOLOS_UX_VALIDATE_PIN is received from
                                       // Dashboard task

    char string_buffer[128];
};

extern ux_state_t        G_ux;
extern bolos_ux_params_t G_ux_params;

extern void ux_process_finger_event(const uint8_t seph_packet[]);
extern void ux_process_button_event(const uint8_t seph_packet[]);
extern void ux_process_ticker_event(void);
extern void ux_process_default_event(void);

/**
 * Initialize the user experience structure
 */
#define UX_INIT() nbgl_objInit();

/**
 * Request a wake up of the device (pin lock screen, ...) to display a new interface to the user.
 * Wake up prevents power-off features. Therefore, security wise, this function shall only
 * be called to request direct user interaction.
 */
#define UX_WAKE_UP()                      \
    G_ux_params.ux_id = BOLOS_UX_WAKE_UP; \
    G_ux_params.len   = 0;                \
    os_ux(&G_ux_params);                  \
    G_ux_params.len = os_sched_last_status(TASK_BOLOS_UX);

/**
 * forward the button push/release events to the os ux handler. if not used by it, it will
 * be used by App controls
 */
#ifdef HAVE_SE_TOUCH
#define UX_BUTTON_PUSH_EVENT(seph_packet)
#else  // HAVE_SE_TOUCH
#define UX_BUTTON_PUSH_EVENT(seph_packet) ux_process_button_event(seph_packet)
#endif  // HAVE_SE_TOUCH

/**
 * forward the finger_event to the os ux handler. if not used by it, it will
 * be used by App controls
 */
#ifdef HAVE_SE_TOUCH
#define UX_FINGER_EVENT(seph_packet) ux_process_finger_event(seph_packet)
#else  // HAVE_SE_TOUCH
#define UX_FINGER_EVENT(seph_packet)
#endif  // HAVE_SE_TOUCH

/**
 * forward the ticker_event to the os ux handler. Ticker event callback is always called whatever
 * the return code of the ux app. Ticker event interval is assumed to be 100 ms.
 */
#define UX_TICKER_EVENT(seph_packet, callback) ux_process_ticker_event()

/**
 * Forward the event, ignoring the UX return code, the event must therefore be either not processed
 * or processed with extreme care by the application afterwards
 */
#define UX_DEFAULT_EVENT() ux_process_default_event()

// discriminated from io to allow for different memory placement
typedef struct ux_seph_s {
    unsigned int button_mask;
    unsigned int button_same_mask_counter;
#ifdef HAVE_BOLOS
    unsigned int ux_id;
    unsigned int ux_status;
#endif  // HAVE_BOLOS
} ux_seph_os_and_app_t;

#ifdef HAVE_BACKGROUND_IMG
SYSCALL     PERMISSION(APPLICATION_FLAG_BOLOS_UX)
uint8_t    *fetch_background_img(bool allow_candidate);
SYSCALL     PERMISSION(APPLICATION_FLAG_BOLOS_UX)
bolos_err_t delete_background_img(void);
#endif

extern ux_seph_os_and_app_t G_ux_os;

#if defined(HAVE_LANGUAGE_PACK)
const char *get_ux_loc_string(UX_LOC_STRINGS_INDEX index);
void        bolos_ux_select_language(uint16_t language);
void        bolos_ux_refresh_language(void);

typedef struct ux_loc_language_pack_infos {
    unsigned char available;

} UX_LOC_LANGUAGE_PACK_INFO;

// To populate infos about language packs
SYSCALL PERMISSION(APPLICATION_FLAG_BOLOS_UX) void list_language_packs(
    UX_LOC_LANGUAGE_PACK_INFO *packs PLENGTH(NB_LANG * sizeof(UX_LOC_LANGUAGE_PACK_INFO)));
SYSCALL PERMISSION(APPLICATION_FLAG_BOLOS_UX) const LANGUAGE_PACK *get_language_pack(
    unsigned int language);
#endif  // defined(HAVE_LANGUAGE_PACK)

#include "glyphs.h"
