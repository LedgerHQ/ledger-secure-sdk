#pragma once

#include "appflags.h"
#include "bolos_target.h"
#include "decorators.h"
#include "os_types.h"

/* ----------------------------------------------------------------------- */
/* -                             PIN FEATURE                             - */
/* ----------------------------------------------------------------------- */

// Global PIN
#define DEFAULT_PIN_RETRIES 3

/* set_pin can update the pin if the perso is onboarded (tearing leads to perso wipe though) */
SYSCALL PERMISSION(APPLICATION_FLAG_BOLOS_UX) void os_perso_set_pin(unsigned int identity,
                                                                    const unsigned char *pin
                                                                                 PLENGTH(length),
                                                                    unsigned int length,
                                                                    bool         update_crc);
// set the currently unlocked identity pin. (change pin feature)
SYSCALL PERMISSION(APPLICATION_FLAG_BOLOS_UX) void os_perso_set_current_identity_pin(
    unsigned char *pin PLENGTH(length),
    unsigned int       length);

/*
 * @return BOLOS_UX_OK if pin validated
 */
SYSCALL bolos_bool_t os_global_pin_is_validated(void);
/**
 * Validating the pin also setup the identity linked with this pin (normal or alternate)
 * @return BOLOS_UX_OK if pin validated
 */
SYSCALL      PERMISSION(APPLICATION_FLAG_GLOBAL_PIN)
bolos_bool_t os_global_pin_check(unsigned char *pin_buffer PLENGTH(pin_length),
                                 unsigned char             pin_length);
SYSCALL      PERMISSION(APPLICATION_FLAG_GLOBAL_PIN) void os_global_pin_invalidate(void);
SYSCALL      PERMISSION(APPLICATION_FLAG_GLOBAL_PIN)
unsigned int os_global_pin_retries(void);

/**
 * This function checks whether a PIN is present
 * @return BOLOS_TRUE if the CRC of N_secure_element_nvram_user_sensitive_data
 * is correct and if a PIN value has been written
 */
SYSCALL
bolos_bool_t os_perso_is_pin_set(void);
