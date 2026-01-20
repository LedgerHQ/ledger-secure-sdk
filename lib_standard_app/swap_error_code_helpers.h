#pragma once
#ifdef HAVE_SWAP

#include "io.h"
#include "buffer.h"

// clang-format off
/*
--8<-- [start:intro]
# Error responses for applications started by Exchange in SWAP context

This specification applies to the error responses returned by the Coin applications when started by
Exchange for the final payment transaction of a SWAP.

Replying valuable data when a final payment transaction is refused eases a lot the analysis,
especially if the issue happens in production context and/or is hard to reproduce.

## RAPDU status word

Each application must define a unique status word for every Exchange-related error.

## RAPDU data

The first 2 bytes of the RAPDU data represent the error code. Format is 16 bits integer in big endian.

The upper byte is common between all applications. It must be one of the following value:

| Name                          | Value  | Description                                                                                                                                     |
| ----------------------------- | ------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| ERROR_INTERNAL                | 0x00   | Internal application error, forward to the Firmware team for analysis.                                                                          |
| ERROR_WRONG_AMOUNT            | 0x01   | The amount does not match the one validated in Exchange.                                                                                        |
| ERROR_WRONG_DESTINATION       | 0x02   | The destination address does not match the one validated in Exchange.                                                                           |
| ERROR_WRONG_FEES              | 0x03   | The fees are different from what was validated in Exchange.                                                                                     |
| ERROR_WRONG_METHOD            | 0x04   | The method used is invalid in Exchange context.                                                                                                 |
| ERROR_CROSSCHAIN_WRONG_MODE   | 0x05   | The mode used for the cross-chain hash validation is not supported.                                                                             |
| ERROR_CROSSCHAIN_WRONG_METHOD | 0x06   | The method used is invalid in cross-chain Exchange context.                                                                                     |
| ERROR_CROSSCHAIN_WRONG_HASH   | 0x07   | The hash for the cross-chain transaction does not match the validated value.                                                                    |
| ERROR_GENERIC                 | 0xFF   | A generic or unspecified error not covered by the specific error codes above.<br>Refer to the remaining bytes for further details on the error. |

The lower byte can be set by the application to refine the error code returned.

So the error code for `ERROR_WRONG_METHOD` would be `0x04XX` with `XX` being application specific
(can be `00` if there is nothing to refine).

The remaining bytes of the data are application-specific and can include, but are not limited to:

- Debugging information (e.g., error logs or internal state).
- Field values (e.g., expected vs actual amounts, destination, fees).
- More specific error codes tailored to the application's context.

---

## C Helpers API

The standard application library define several helper function to return error codes from the Coin
application.
--8<-- [end:intro]
*/
// clang-format on

typedef enum swap_error_common_code_e {
    SWAP_EC_ERROR_INTERNAL                = 0x00,
    SWAP_EC_ERROR_WRONG_AMOUNT            = 0x01,
    SWAP_EC_ERROR_WRONG_DESTINATION       = 0x02,
    SWAP_EC_ERROR_WRONG_FEES              = 0x03,
    SWAP_EC_ERROR_WRONG_METHOD            = 0x04,
    SWAP_EC_ERROR_CROSSCHAIN_WRONG_MODE   = 0x05,
    SWAP_EC_ERROR_CROSSCHAIN_WRONG_METHOD = 0x06,
    SWAP_EC_ERROR_CROSSCHAIN_WRONG_HASH   = 0x07,
    SWAP_EC_ERROR_GENERIC                 = 0xFF,
} swap_error_common_code_t;

// --8<-- [start:helpers]
/**
 * Sends a basic swap error with no extra data.
 *
 * @param status_word RAPDU status word.
 * @param common_error_code Common error code defined in swap_error_common_code_t.
 * @param application_specific_error_code Application-specific error code.
 */
__attribute__((noreturn)) void send_swap_error_simple(uint16_t status_word,
                                                      uint8_t  common_error_code,
                                                      uint8_t  application_specific_error_code);

/**
 * Sends a swap error with one additional buffer data.
 *
 * @param status_word RAPDU status word.
 * @param common_error_code Common error code.
 * @param application_specific_error_code Application-specific error code.
 * @param buffer_data Additional application-specific error details.
 */
__attribute__((noreturn)) void send_swap_error_with_buffer(uint16_t status_word,
                                                           uint8_t  common_error_code,
                                                           uint8_t  application_specific_error_code,
                                                           const buffer_t buffer_data);

/**
 * Sends a swap error with multiple buffers containing error details as data.
 *
 * @param status_word RAPDU status word.
 * @param common_error_code Common error code.
 * @param application_specific_error_code Application-specific error code.
 * @param buffer_data Array of buffers with error details. SWAP_ERROR_HELPER_MAX_BUFFER_COUNT
 * @param count Number of buffers provided.
 */
#define SWAP_ERROR_HELPER_MAX_BUFFER_COUNT 8
__attribute__((noreturn)) void send_swap_error_with_buffers(uint16_t status_word,
                                                            uint8_t  common_error_code,
                                                            uint8_t application_specific_error_code,
                                                            const buffer_t *buffer_data,
                                                            size_t          count);

/**
 * Macro to send a swap error with a formatted string as data.
 *
 * Constructs a buffer from a formatted string and passes it to send_swap_error_with_buffers.
 * @param status_word RAPDU status word.
 * @param common_error_code Common error code.
 * @param application_specific_error_code Application-specific error code.
 * @param format printf-style format string.
 * @param ... Additional arguments for formatting.
 */
// Immediately call snprintf here (no function wrapping it cleanly in a .c file).
// This is because we don't have a vsnprintf implementation which would be needed if
// we were to pass the va_args to an intermediate function.
// See https://stackoverflow.com/a/150578
#define send_swap_error_with_string(                                                             \
    status_word, common_error_code, application_specific_error_code, format, ...)                \
    do {                                                                                         \
        /* Up to a full data apdu minus the status word and the swap error code */               \
        char format_buffer[sizeof(G_io_apdu_buffer) - sizeof(status_word) - 2] = {0};            \
        /* snprintf always returns 0 on our platform, don't check the return value */            \
        /* See https://github.com/LedgerHQ/ledger-secure-sdk/issues/236 */                       \
        snprintf(format_buffer, sizeof(format_buffer), format, ##__VA_ARGS__);                   \
        PRINTF("send_swap_error_with_string %s\n", format_buffer);                               \
        buffer_t string_buffer;                                                                  \
        string_buffer.ptr    = (uint8_t *) &format_buffer;                                       \
        string_buffer.size   = strnlen(format_buffer, sizeof(format_buffer));                    \
        string_buffer.offset = 0;                                                                \
        send_swap_error_with_buffers(                                                            \
            status_word, common_error_code, application_specific_error_code, &string_buffer, 1); \
    } while (0)
// --8<-- [end:helpers]

#endif  // HAVE_SWAP
