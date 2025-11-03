/*****************************************************************************
 *   (c) 2020 Ledger SAS.
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
 *****************************************************************************/

#include <stdint.h>  // uint*_t
#include <string.h>  // memset, explicit_bzero

#include "app_storage_internal.h"
#include "os.h"
#include "io.h"
#include "ledger_assert.h"

#ifdef HAVE_SWAP
#include "swap.h"
#include "swap_error_code_helpers.h"
#endif  // HAVE_SWAP

#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#endif  // HAVE_NBGL

ux_state_t        G_ux;
bolos_ux_params_t G_ux_params;

/**
 * Exit the application and go back to the dashboard.
 */
WEAK void __attribute__((noreturn)) app_exit(void)
{
    // send automatic error response in case of potentially pending APDU
    io_send_pending_response();
#ifndef USE_OS_IO_STACK
    os_io_stop();
#endif  // USE_OS_IO_STACK
    os_sched_exit(-1);
}

WEAK void common_app_init(void)
{
    UX_INIT();
    io_seproxyhal_init();

#ifdef HAVE_APP_STORAGE
    /* Implicit app storage initialization */
    app_storage_init();
#endif  // #ifdef HAVE_APP_STORAGE
}

WEAK void standalone_app_main(void)
{
    PRINTF("standalone_app_main\n");
#ifdef HAVE_SWAP
    G_called_from_swap                  = false;
    G_swap_response_ready               = false;
    G_swap_signing_return_value_address = NULL;
#endif  // HAVE_SWAP

    BEGIN_TRY
    {
        TRY
        {
            common_app_init();

            app_main();
        }
        CATCH_OTHER(e)
        {
            (void) e;
#ifdef HAVE_DEBUG_THROWS
            // Disable USB and BLE, the app have crashed and is going to be exited
            // This is necessary to avoid device freeze while displaying throw error
            // in a specific case:
            // - the app receives an APDU
            // - the app throws before replying
            // - the app displays the error on screen
            // - the user unplug the NanoX instead of confirming the screen
            // - the NanoX goes on battery power and display the lock screen
            // - the user plug the NanoX instead of entering its pin
            // - the device is frozen, battery should be removed
            os_io_stop();
            // Display crash info on screen for debug purpose
            assert_display_exit();
#else   // HAVE_DEBUG_THROWS
            PRINTF("Exiting following exception: 0x%04X\n", e);
#endif  // HAVE_DEBUG_THROWS
        }
        FINALLY {}
    }
    END_TRY;

    // Exit the application and go back to the dashboard.
    app_exit();
}

#ifdef HAVE_SWAP
// --8<-- [start:library_app_main]
/* This function is called by the main() function if this application was started by Exchange
 * through an os_lib_call() as opposed to being started from the Dashboard.
 *
 * We dispatch the Exchange request to the correct handler.
 * Handlers content are not defined in the `lib_standard_app`
 */
WEAK void library_app_main(libargs_t *args)
{
    BEGIN_TRY
    {
        TRY
        {
            PRINTF("Inside library\n");
            switch (args->command) {
                case SIGN_TRANSACTION: {
                    // Backup up transaction parameters and wipe BSS to avoid collusion with
                    // app-exchange BSS data.
                    bool success = swap_copy_transaction_parameters(args->create_transaction);
                    if (success) {
                        // BSS was wiped, we can now init these globals
                        G_called_from_swap    = true;
                        G_swap_response_ready = false;
                        // Keep the address at which we'll reply the signing status
                        G_swap_signing_return_value_address = &args->create_transaction->result;

                        common_app_init();

#ifdef HAVE_NBGL
                        nbgl_useCaseSpinner("Signing");
#endif  // HAVE_NBGL

                        app_main();
                    }
                    break;
                }
                case CHECK_ADDRESS:
                    swap_handle_check_address(args->check_address);
                    break;
                case GET_PRINTABLE_AMOUNT:
                    swap_handle_get_printable_amount(args->get_printable_amount);
                    break;
                default:
                    break;
            }
        }
        CATCH_OTHER(e)
        {
            (void) e;
            PRINTF("Exiting following exception: 0x%04X\n", e);
        }
        FINALLY
        {
            os_lib_end();
        }
    }
    END_TRY;
}
// --8<-- [end:library_app_main]
#endif  // HAVE_SWAP

WEAK __attribute__((section(".boot"))) int main(int arg0)
{
    // exit critical section
    __asm volatile("cpsie i");

    // Ensure exception will work as planned
    os_boot();

    if (arg0 == 0) {
        // Called from dashboard as standalone App
        standalone_app_main();
    }
#ifdef HAVE_SWAP
    else {
        // Called as library from another app
        libargs_t *args = (libargs_t *) arg0;
        if (args->id == 0x100) {
            library_app_main(args);
        }
        else {
            app_exit();
        }
    }
#endif  // HAVE_SWAP

    return 0;
}
