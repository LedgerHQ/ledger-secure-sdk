#pragma once

/**
 * Application is allowed to use the raw master seed.
 * If not set, at least a level of derivation is required.
 */
#define APPLICATION_FLAG_DERIVE_MASTER 0x10

/**
 * If the application is loaded by a trusted issuer, it can be loaded in onboarding phase
 * (using recovery mode), without being deleted when the device boots in normal mode.
 */
#define APPLICATION_FLAG_PRELOADED 0x20

/**
 * Application is allowed to ask user pin.
 */
#define APPLICATION_FLAG_GLOBAL_PIN 0x40

/**
 * Application is allowed to change the settings
 */
#define APPLICATION_FLAG_BOLOS_SETTINGS 0x200

/**
 * The application main can be called in two ways:
 *  - with first arg (stored in r0) set to 0: The application is called from the dashboard
 *  - with first arg (stored in r0) set to != 0 (ram address likely): The application is used as a
 * library from another app.
 */
#define APPLICATION_FLAG_LIBRARY 0x800

/**
 * Reserved
 */
#define APPLICATION_FLAG_RESERVED 0x40000
