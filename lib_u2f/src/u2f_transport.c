/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "os.h"
#include "os_math.h"
#include "os_utils.h"
#include "os_io.h"

#include "u2f_types.h"
#include "u2f_transport.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#ifdef HAVE_PRINTF
//#define DEBUG PRINTF
 #define DEBUG(...)
#else  // !HAVE_PRINTF
#define DEBUG(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static u2f_error_t process_packet(u2f_transport_t *handle, uint8_t *buffer, uint16_t length);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static u2f_error_t process_packet(u2f_transport_t *handle, uint8_t *buffer, uint16_t length)
{
    u2f_error_t error = CTAP1_ERR_SUCCESS;

    // Check CID for USB HID transport
    if (handle->type == U2F_TRANSPORT_TYPE_USB_HID) {
        if (length < 4) {
            // CID not complete, answer with braodcast CID?
            error       = CTAP1_ERR_OTHER;
            handle->cid = U2F_BROADCAST_CID;
            goto end;
        }
        uint32_t message_cid = U4BE(buffer, 0);
        if (message_cid == U2F_FORBIDDEN_CID) {
            // Forbidden CID
            error = CTAP1_ERR_INVALID_CHANNEL;
            goto end;
        }
        else if ((message_cid == U2F_BROADCAST_CID) && (length >= 5)
                 && (buffer[4] != (U2F_COMMAND_HID_INIT | 0x80))) {
            // Broadcast CID but not an init message
            error = CTAP1_ERR_INVALID_CHANNEL;
            goto end;
        }
        else if ((handle->cid != U2F_FORBIDDEN_CID) && (handle->cid != message_cid)) {
            // CID is already set
            error = CTAP1_ERR_CHANNEL_BUSY;
            goto end;
        }
        else if (handle->state > U2F_STATE_CMD_FRAMING) {
            // Good CID but a request is already in process
            error = CTAP1_ERR_CHANNEL_BUSY;
            goto end;
        }
        else if (handle->cid == U2F_FORBIDDEN_CID) {
            // Set new CID
            handle->cid = message_cid;
        }
        buffer += 4;
        length -= 4;
    }

    // Check header length
    if (length < 1) {
        error = CTAP1_ERR_OTHER;
        goto end;
    }

    if (buffer[0] & 0x80) {
        // Initialization packet
        if (length < 3) {
            error = CTAP1_ERR_OTHER;
            goto end;
        }

        // Check packet will fit in the rx buffer
        handle->rx_message_length = (uint16_t) U2BE(buffer, 1) + 3;
        if (handle->rx_message_length > handle->rx_message_buffer_size) {
            error = CTAP1_ERR_OTHER;
            goto end;
        }

        handle->state                                          = U2F_STATE_CMD_FRAMING;
        handle->rx_message_offset                              = 0;
        handle->rx_message_buffer[handle->rx_message_offset++] = buffer[0] & 0x7F;  // CMD
        handle->rx_message_buffer[handle->rx_message_offset++] = buffer[1];         // BCNTH
        handle->rx_message_buffer[handle->rx_message_offset++] = buffer[2];         // BCNTL
        handle->rx_message_expected_sequence_number            = 0;
        buffer += 3;
        length -= 3;
    }
    else {
        // Continuation packet
        if (handle->state != U2F_STATE_CMD_FRAMING) {
            error = CTAP1_ERR_OTHER;
            goto end;
        }
        else if (buffer[0] != handle->rx_message_expected_sequence_number) {
            error = CTAP1_ERR_INVALID_SEQ;
            goto end;
        }
        handle->rx_message_expected_sequence_number++;
        buffer += 1;
        length -= 1;
    }

    if ((handle->rx_message_offset + length) > handle->rx_message_length) {
        length = handle->rx_message_length - handle->rx_message_offset;
    }

    memcpy(&handle->rx_message_buffer[handle->rx_message_offset], buffer, length);
    handle->rx_message_offset += length;

    if (handle->rx_message_offset == handle->rx_message_length) {
        handle->state = U2F_STATE_CMD_COMPLETE;
    }

end:
    return error;
}

/* Exported functions --------------------------------------------------------*/
void U2F_TRANSPORT_init(u2f_transport_t *handle, uint8_t type)
{
    if (!handle) {
        return;
    }

    handle->state = U2F_STATE_IDLE;
    handle->cid   = U2F_FORBIDDEN_CID;
    handle->type  = type;
}

void U2F_TRANSPORT_rx(u2f_transport_t *handle, uint8_t *buffer, uint16_t length)
{
    if (!handle || !buffer || length < 3) {
        return;
    }

    uint8_t error = (uint8_t) process_packet(handle, buffer, length);

    if (error != CTAP1_ERR_SUCCESS) {
        U2F_TRANSPORT_tx(handle, U2F_COMMAND_ERROR, &error, 1);
        return;
    }

    switch (handle->rx_message_buffer[0]) {
        case U2F_COMMAND_PING:
            U2F_TRANSPORT_tx(handle,
                             U2F_COMMAND_PING,
                             &handle->rx_message_buffer[3],
                             handle->rx_message_length - 3);
            DEBUG("U2F_COMMAND_PING %d\n", handle->rx_message_length);
            handle->state = U2F_STATE_CMD_PROCESSING;
            break;

        default:
            break;
    }
}

void U2F_TRANSPORT_tx(u2f_transport_t *handle, uint8_t cmd, const uint8_t *buffer, uint16_t length)
{
    if (!handle || (!buffer && !handle->tx_message_buffer)) {
        return;
    }

    if (buffer) {
        DEBUG("INITIALIZATION PACKET\n");
        handle->tx_message_buffer          = buffer;
        handle->tx_message_length          = length;
        handle->tx_message_sequence_number = 0;
        handle->tx_message_offset          = 0;
        handle->tx_packet_length           = 0;
        memset(handle->tx_packet_buffer, 0, handle->tx_packet_buffer_size);
    }
    else {
        DEBUG("CONTINUATION PACKET\n");
    }

    uint16_t tx_packet_offset = 0;
    memset(handle->tx_packet_buffer, 0, handle->tx_packet_buffer_size);

    // Add CID if necessary
    if (handle->type == U2F_TRANSPORT_TYPE_USB_HID) {
        U4BE_ENCODE(handle->tx_packet_buffer, 0, handle->cid);
        tx_packet_offset += 4;
    }

    // Fill header
    if (buffer) {
        handle->tx_packet_buffer[tx_packet_offset++] = cmd | 0x80;        // CMD
        U2BE_ENCODE(handle->tx_packet_buffer, tx_packet_offset, length);  // BCNT
        tx_packet_offset += 2;
    }
    else {
        handle->tx_packet_buffer[tx_packet_offset++] = handle->tx_message_sequence_number++;  // SEQ
    }

    if ((handle->tx_message_length + tx_packet_offset)
        >= (handle->tx_packet_buffer_size + handle->tx_message_offset)) {
        // Remaining message length doesn't fit the max packet size
        memcpy(&handle->tx_packet_buffer[tx_packet_offset],
               &handle->tx_message_buffer[handle->tx_message_offset],
               handle->tx_packet_buffer_size - tx_packet_offset);
        handle->tx_message_offset += handle->tx_packet_buffer_size - tx_packet_offset;
        tx_packet_offset = handle->tx_packet_buffer_size;
    }
    else {
        // Remaining message fits the max packet size
        memcpy(&handle->tx_packet_buffer[tx_packet_offset],
               &handle->tx_message_buffer[handle->tx_message_offset],
               handle->tx_message_length - handle->tx_message_offset);
        tx_packet_offset += (handle->tx_message_length - handle->tx_message_offset);
        handle->tx_message_offset = handle->tx_message_length;
        handle->tx_message_buffer = NULL;
        handle->state             = U2F_STATE_IDLE;
        handle->cid               = U2F_FORBIDDEN_CID;
    }

    handle->tx_packet_length = tx_packet_offset;
    DEBUG(" %d\n", handle->tx_packet_length);
}
