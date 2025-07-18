
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

#if !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)

#include "bolos_target.h"
#include "checks.h"
#include "os_helpers.h"
#include "os_pin.h"
#include "os_screen.h"
#include "os_io.h"
#include "ux.h"

// This label ultimately comes from the application link.
extern unsigned int const _install_parameters;
extern unsigned int const _einstall_parameters;

#ifdef HAVE_BAGL

static int audited_ux_slot;

static bool ui_audited_done(void)
{
    return !G_ux.stack[audited_ux_slot].button_push_callback;
}

// This function is the button callback associated with the 'ui_audited_elements' array below.
static unsigned int ui_audited_elements_button(unsigned int button_mask,
                                               unsigned int button_mask_counter
                                               __attribute__((unused)))
{
    unsigned int slot;

    // As soon as the user presses both buttons, we reinitialize the UX and buttons,
    // and the hand is given back to the normal process.
    if ((button_mask & (BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT))
        == (BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT)) {
        // We find the correct slot.
        for (slot = 0x00; slot < G_ux.stack_count; slot++) {
            if (G_ux.stack[slot].button_push_callback == ui_audited_elements_button) {
                G_ux.stack[slot].button_push_callback = NULL;
                break;
            }
        }
    }
    return 0;
}

// This array is to be displayed under specific circumstances, right at the launch of an
// application.
// clang-format off
const bagl_element_t ui_audited_elements[] = {
#if (BAGL_WIDTH==128 && BAGL_HEIGHT==32)
  // Erasure of the whole screen,
  {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, .text=NULL},
  // First line of text,
  {
    {BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    .text="Pending",
  },
  // Last line of text.
  {
    {BAGL_LABELINE, 0x01, 0, 24, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    .text="Ledger review",
  },
#elif (BAGL_WIDTH==128 && BAGL_HEIGHT==64)
  // Erasure of the whole screen,
  {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  64, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, .text=NULL},
  // Warning icon.
  {{BAGL_ICON                           , 0x02,  57,  10,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, 0  }, .text=(const char*)&C_icon_warning},
  // First line of text,
  {
    {BAGL_LABELINE, 0x01, 0, 37, 128, 11, 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    .text="Pending",
    },
  // Last line of text.
  {
    {BAGL_LABELINE, 0x01, 0, 51, 128, 11, 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
    .text="Ledger review",
  },
#else
#error "BAGL_WIDTH/BAGL_HEIGHT not defined"
#endif // (BAGL_WIDTH==128 && BAGL_HEIGHT==64)
};
// clang-format on

void ui_audited_init(void)
{
    // We reserve the first slot for this display.
    audited_ux_slot = ux_stack_push();
    ux_stack_init(audited_ux_slot);

    // We trigger the additional display and wait for it to be completed.
    G_ux.stack[audited_ux_slot].element_arrays[0].element_array = ui_audited_elements;
    G_ux.stack[audited_ux_slot].element_arrays[0].element_array_count
        = sizeof(ui_audited_elements) / sizeof(ui_audited_elements[0]);
    G_ux.stack[audited_ux_slot].button_push_callback                   = ui_audited_elements_button;
    G_ux.stack[audited_ux_slot].screen_before_element_display_callback = NULL;
    UX_WAKE_UP();
    UX_DISPLAY_NEXT_ELEMENT();
    UX_WAIT_DISPLAYED();
}

void ui_audited_deinit(void)
{
    // We pop the reserved slot but we do not care about the returned value (since we do not need it
    // for
    // further displays at the moment) and reinitialize the UX and buttons.
    ux_stack_pop();
    io_seph_ux_init_button();
}
#endif  // HAVE_BAGL

#ifdef HAVE_NBGL
#include "nbgl_use_case.h"

static bool auditChoiceMade;

static bool ui_audited_done(void)
{
    return auditChoiceMade;
}

void ui_audited_choice(bool choice)
{
    if (choice) {
        auditChoiceMade = choice;
    }
    else {
        os_sched_exit(0);
    }
}

void ui_audited_init(void)
{
    auditChoiceMade = false;
    nbgl_useCaseChoice(&C_Warning_64px,
                       "Pending Ledger review",
                       "This app has not been\nreviewed by Ledger",
                       "Open",
                       "Don't open",
                       ui_audited_choice);
}

void ui_audited_deinit(void) {}

#endif  // HAVE_NBGL

// This function is called at the end of the seph initialization.
// It checks the install parameters of the application to be run, and if this area contains the
// CHECK_NOT_AUDITED_TLV_TAG tag with the CHECK_NOT_AUDITED_TLV_VAL value, a specific display
// is triggered before the actual application's splash screen.
void check_audited_app(void)
{
    unsigned char  data   = BOLOS_FALSE;
    unsigned char *buffer = &data;
    unsigned int   install_parameters_len
        = (unsigned char *) &_einstall_parameters - (unsigned char *) &_install_parameters;
    unsigned int length = os_parse_bertlv((unsigned char *) &_install_parameters,
                                          install_parameters_len,
                                          NULL,
                                          CHECK_NOT_AUDITED_TLV_TAG,
                                          0x00,
                                          (void **) &buffer,
                                          sizeof(data));

    // We trigger the associated behaviour only when the tag was present and the value corresponds
    // to the expected one.
    if ((length) && (CHECK_NOT_AUDITED_TLV_VAL == data)) {
        ui_audited_init();

        do {
            os_io_handle_ux_event_reject_apdu();
        } while (!ui_audited_done());

        ui_audited_deinit();
    }
}

#endif  // !defined(HAVE_BOLOS)  && defined(HAVE_PENDING_REVIEW_SCREEN)
