#pragma once

#include "macros.h"

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
 * Main entry point of the App, in standalone mode.
 * It calls (inside a try/catch):
 *  - common_app_init
 *  - app_main (should be defined in the App)
 */
WEAK void standalone_app_main(void);

/**
 * Main entry point of the App, in library mode.
 * It calls the swap handlers (inside a try/catch)
 */
WEAK void library_app_main(void);
