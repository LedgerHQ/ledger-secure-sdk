#pragma once

/* ----------------------------------------------------------------------- */
/* -                            UX DEFINITIONS                           - */
/* ----------------------------------------------------------------------- */

// Enumeration of the UX events usable by the UX library.
typedef enum bolos_ux_public_e {
    BOLOS_UX_INITIALIZE = 0,
    BOLOS_UX_EVENT,
    BOLOS_UX_KEYBOARD,
    BOLOS_UX_WAKE_UP,
    BOLOS_UX_STATUS_BAR,

    BOLOS_UX_VALIDATE_PIN,
    BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST,  // ask the ux to display a modal to accept/reject the
                                           // current pairing request
    BOLOS_UX_ASYNCHMODAL_PAIRING_CANCEL,
    BOLOS_UX_IO_RESET,
    BOLOS_UX_DELAY_LOCK,  // delay the power-off/lock timer
    NB_BOLOS_UX_IDS
} bolos_ux_public_t;
