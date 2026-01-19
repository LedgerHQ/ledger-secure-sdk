#pragma once

#include "macros.h"
#include "swap_lib_calls.h"

/**
 * Exit the application and go back to the dashboard.
 */
void __attribute__((noreturn)) app_exit(void);

/**
 * Common init of the App.
 *  - UX
 *  - io
 *  - USB
 *  - BLE
 */
WEAK void common_app_init(void);

/**
 * Common setup of the App, called after common_app_init().
 * This allows to setup the application depending on the mode:
 * In lib_mode, some features can be disabled (like USB and BLE).
 */
WEAK void common_app_setup(bool lib_mode);

/**
 * Main entry point of the App, in standalone mode.
 * It calls (inside a try/catch):
 *  - common_app_init
 *  - app_main (should be defined in the App)
 */
WEAK void standalone_app_main(libargs_t *args);

/**
 * Main entry point of the App, in library mode.
 * It calls the swap handlers (inside a try/catch)
 */
WEAK void library_app_main(libargs_t *args);

/**
 * Main entry point of the Clone App.
 * This function must be provided by the application.
 * It dispatches to os_lib_call with dedicated parameters.
 */
void NORETURN clone_app_main(libargs_t *args);
