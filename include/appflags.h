#pragma once

/**
 * Application is allowed to use the raw master seed.
 * If not set, at least a level of derivation is required.
 */
#define APPLICATION_FLAG_DERIVE_MASTER 0x10

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
