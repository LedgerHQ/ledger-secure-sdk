#pragma once

#include <stdint.h>

#include "ux.h"
#include "os_io_seproxyhal.h"

#include "buffer.h"
#include "macros.h"

/**
 * Enumeration for the status of IO.
 */
typedef enum {
    READY,     /// ready for new event
    RECEIVED,  /// data received
    WAITING    /// waiting
} io_state_e;

#ifdef HAVE_BAGL
WEAK void io_seproxyhal_display(const bagl_element_t *element);
#endif  // HAVE_BAGL

/**
 * Function to be declared by the application to trigger execute code upon
 * SEPROXYHAL_TAG_TICKER_EVENT
 *
 *
 */
WEAK void app_ticker_event_callback(void);

/**
 * IO callback called when an interrupt based channel has received
 * data to be processed.
 * This function is responsible of forwarding received events to the UX lib.
 *
 * @return 1 if success, 0 otherwise.
 *
 */
WEAK uint8_t io_event(uint8_t channel __attribute__((unused)));

WEAK uint16_t io_exchange_al(uint8_t channel, uint16_t tx_len);

/**
 * Initialize the APDU I/O state.
 *
 * This function must be called before calling any other I/O function.
 */
WEAK void io_init(void);

/**
 * Receive APDU command in G_io_apdu_buffer.
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
WEAK int io_recv_command(void);

/**
 * Send APDU response (response data + status word) by filling
 * G_io_apdu_buffer.
 *
 * @param[in] ptr
 *   Pointer to a buffer with APDU response data.
 * @param[in] size
 *   Size of the buffer with the APDU response data.
 * @param[in] sw
 *   Status word of APDU response.
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
WEAK int io_send_response_pointer(const uint8_t *ptr, size_t size, uint16_t sw);

/**
 * Send APDU response (response data + status word) by filling
 * G_io_apdu_buffer.
 *
 * @param[in] rdata
 *   Buffer with APDU response data.
 * @param[in] sw
 *   Status word of APDU response.
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
WEAK int io_send_response_buffer(const buffer_t *rdata, uint16_t sw);

/**
 * Send APDU response (only status word) by filling
 * G_io_apdu_buffer.
 *
 * @param[in] sw
 *   Status word of APDU response.
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
WEAK int io_send_sw(uint16_t sw);
