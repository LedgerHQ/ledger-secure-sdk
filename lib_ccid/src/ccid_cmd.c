/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os.h"

#include "ccid_types.h"
#include "ccid_transport.h"
#include "ccid_cmd.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

#ifdef HAVE_PRINTF
#define DEBUG PRINTF
// #define DEBUG(...)
#else  // !HAVE_PRINTF
#define DEBUG(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static uint8_t ccid_cmd_check_length(ccid_transport_t *transport, uint32_t max);

static void    PC_to_RDR_IccPowerOn(ccid_device_t *handle);
static void    PC_to_RDR_IccPowerOff(ccid_device_t *handle);
static void    PC_to_RDR_GetSlotStatus(ccid_device_t *handle);
static int32_t PC_to_RDR_XfrBlock(ccid_device_t *handle);
static void    PC_to_RDR_GetParameters(ccid_device_t *handle);
static void    PC_to_RDR_ResetParameters(ccid_device_t *handle);
static void    PC_to_RDR_SetParameters(ccid_device_t *handle);
static void    PC_to_RDR_Escape(ccid_device_t *handle);
static void    PC_to_RDR_IccClock(ccid_device_t *handle);
static void    PC_to_RDR_T0APDU(ccid_device_t *handle);
static int32_t PC_to_RDR_Secure(ccid_device_t *handle);
static void    PC_to_RDR_Mechanical(ccid_device_t *handle);
static void    PC_to_RDR_Abort(ccid_device_t *handle);
static void    PC_to_RDR_SetDataRateAndClockFrequency(ccid_device_t *handle);

static void RDR_to_PC_DataBlock(ccid_device_t *handle);
static void RDR_to_PC_SlotStatus(ccid_device_t *handle);
static void RDR_to_PC_Parameters(ccid_device_t *handle);
static void RDR_to_PC_Escape(ccid_device_t *handle);
static void RDR_to_PC_DataRateAndClockFrequency(ccid_device_t *handle);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static uint8_t ccid_cmd_check_length(ccid_transport_t *transport, uint32_t max)
{
    if (transport->bulk_msg_header.out.length > max) {
        // Bad dwLength
        transport->bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        transport->bulk_msg_header.in.error = CCID_ERROR_CMD_BAD_DWLENGTH;
        return 1;
    }
    return 0;
}

static void PC_to_RDR_IccPowerOn(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    if (handle->abort.requested) {
        // Abort in progress
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_ABORTED;
        goto end;
    }

    uint8_t bPowerSelect = handle->transport.bulk_msg_header.out.specific[0];
    if (bPowerSelect >= CCID_VOLTAGE_MAX) {
        // Bad bPowerSelect
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_BAD_POWER_SELECT;
        goto end;
    }

    // ATR
    // Send ATR
    handle->transport.bulk_msg_header.in.length = 2;
    handle->transport.rx_msg_buffer[0]          = 0x3B;
    handle->transport.rx_msg_buffer[1]          = 0x00;

end:
    RDR_to_PC_DataBlock(handle);
}

static void PC_to_RDR_IccPowerOff(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    if (!handle->card_inserted) {
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE
              | CCID_SLOT_STATUS_CMD_PROCESSED_WITHOUT_ERROR;
    }

end:
    RDR_to_PC_SlotStatus(handle);
}

static void PC_to_RDR_GetSlotStatus(ccid_device_t *handle)
{
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        ccid_cmd_check_length(&handle->transport, 0);
    }

    RDR_to_PC_SlotStatus(handle);
}

static int32_t PC_to_RDR_XfrBlock(ccid_device_t *handle)
{
    int32_t status = 0;

    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 261))) {  // TODO_IO
        goto end;
    }

    if (handle->abort.requested) {
        // Abort in progress
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_ABORTED;
        goto end;
    }

    if (handle->transport.bulk_msg_header.out.length + 10 > handle->transport.rx_msg_buffer_size) {
        // abData field too long
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_BAD_DWLENGTH;
    }

    handle->transport.bulk_msg_header.in.length
        = U2LE(handle->transport.bulk_msg_header.out.specific,
               1);  // wLevelParameter : expected rsp length

    status = 1;

end:
    if (status == 0) {
        RDR_to_PC_DataBlock(handle);
    }
    else {
        handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_DATA_BLOCK;
    }

    return status;
}

static void PC_to_RDR_GetParameters(ccid_device_t *handle)
{
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        ccid_cmd_check_length(&handle->transport, 0);
    }

    RDR_to_PC_Parameters(handle);
}

static void PC_to_RDR_ResetParameters(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    CCID_TRANSPORT_reset_parameters(&handle->transport);

end:
    RDR_to_PC_Parameters(handle);
}

static void PC_to_RDR_SetParameters(ccid_device_t *handle)
{
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        goto end;
    }

    if (handle->transport.bulk_msg_header.out.specific[0] != 0x00) {
        // Invalid bProtocolNum
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_INVALID_PROTOCOL;
        goto end;
    }

    if ((handle->transport.bulk_msg_header.out.length != 5)
        || (handle->transport.rx_msg_length != 5)) {
        // Invalid dwLength
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_BAD_DWLENGTH;
        goto end;
    }

    /*if (handle->transport.rx_msg_buffer[3] != 0x00) {
        // Invalid bWaitingIntegerT0
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_INVALID_WI;
        goto end;
    }*/

    if (handle->transport.rx_msg_buffer[4] != 0x00) {
        // Invalid bClockStop
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_INVALID_CLOCK_STOP;
        goto end;
    }

    handle->transport.protocol_data.bmFindexDindex    = handle->transport.rx_msg_buffer[0];
    handle->transport.protocol_data.bmTCCKST0         = handle->transport.rx_msg_buffer[1];
    handle->transport.protocol_data.bGuardTimeT0      = handle->transport.rx_msg_buffer[2];
    handle->transport.protocol_data.bWaitingIntegerT0 = handle->transport.rx_msg_buffer[3];
    handle->transport.protocol_data.bClockStop        = handle->transport.rx_msg_buffer[4];

end:
    RDR_to_PC_Parameters(handle);
}

static void PC_to_RDR_Escape(ccid_device_t *handle)
{
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        goto end;
    }

    if (handle->abort.requested) {
        // Abort in progress
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_ABORTED;
        goto end;
    }

    // For now this is not presented to the upper layer
    handle->transport.bulk_msg_header.in.length = 0;

end:
    RDR_to_PC_Escape(handle);
}

static void PC_to_RDR_IccClock(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    if (handle->transport.bulk_msg_header.out.specific[0] != 0x00) {
        // Invalid bClockCommand
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
        goto end;
    }

end:
    RDR_to_PC_SlotStatus(handle);
}

static void PC_to_RDR_T0APDU(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    if (handle->abort.requested) {
        // Abort in progress
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_ABORTED;
        goto end;
    }

    if (handle->transport.bulk_msg_header.out.specific[0] > 0x03) {
        // Invalid bmChanges
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
        goto end;
    }

end:
    RDR_to_PC_SlotStatus(handle);
}

static int32_t PC_to_RDR_Secure(ccid_device_t *handle)
{
    int32_t status = 0;

    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        goto end;
    }

    if (handle->abort.requested) {
        // Abort in progress
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_ABORTED;
        goto end;
    }

    uint16_t wLevelParameter = U2LE(handle->transport.bulk_msg_header.out.specific, 1);

    if (wLevelParameter != 0x0000) {
        // Invalid wLevelParameter
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
        goto end;
    }

    uint8_t apdu_offset = 0;
    switch (handle->transport.rx_msg_buffer[0]) {  // bPINOperation
        case 0x00:
            // PIN Verification
            apdu_offset = 15;
            break;

        case 0x01:
            // PIN Verification
            switch (handle->transport.rx_msg_buffer[11]) {  // bNumberMessage
                case 0x00:
                    apdu_offset = 18;
                    break;

                case 0x01:
                case 0x02:
                    apdu_offset = 19;
                    break;

                case 0x03:
                    apdu_offset = 20;
                    break;

                default:
                    handle->transport.bulk_msg_header.in.status
                        = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
                    handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
                    break;
            }
            break;

        default:
            // Transfer PIN
            // Wait ICC response
            // Cancel PIN function
            // Re-send last I-Block
            // Send next part of APDU
            handle->transport.bulk_msg_header.in.status
                = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
            handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
            break;
    }

    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        goto end;
    }

    handle->transport.bulk_msg_header.in.length = 5;  // TODO_IO : check this
    handle->transport.rx_msg_length             = 5;
    memmove(handle->transport.rx_msg_buffer, &handle->transport.rx_msg_buffer[apdu_offset], 5);
    handle->transport.rx_msg_buffer[0] = 0xEF;  // TODO_IO : define

    status = 1;

end:
    if (status == 0) {
        RDR_to_PC_DataBlock(handle);
    }
    else {
        handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_DATA_BLOCK;
    }

    return status;
}

static void PC_to_RDR_Mechanical(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    if (handle->transport.bulk_msg_header.out.specific[0] > 0x05) {
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
    }

end:
    RDR_to_PC_SlotStatus(handle);
}

static void PC_to_RDR_Abort(ccid_device_t *handle)
{
    if ((handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR)
        || (ccid_cmd_check_length(&handle->transport, 0))) {
        goto end;
    }

    CCID_CMD_abort(handle,
                   handle->transport.bulk_msg_header.in.slot_number,
                   handle->transport.bulk_msg_header.in.seq_number);

end:
    RDR_to_PC_SlotStatus(handle);
}

static void PC_to_RDR_SetDataRateAndClockFrequency(ccid_device_t *handle)
{
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        goto end;
    }

    if (handle->transport.bulk_msg_header.out.length != 8) {
        // Bad dwLength
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_BAD_DWLENGTH;
        goto end;
    }

    handle->transport.clock_frequency = U4LE(handle->transport.rx_msg_buffer, 0);
    handle->transport.data_rate       = U4LE(handle->transport.rx_msg_buffer, 4);

    handle->transport.bulk_msg_header.in.length = 8;

end:
    RDR_to_PC_DataRateAndClockFrequency(handle);
}

static void RDR_to_PC_DataBlock(ccid_device_t *handle)
{
    handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_DATA_BLOCK;
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        handle->transport.bulk_msg_header.in.length = 0;
        goto end;
    }

    handle->transport.bulk_msg_header.in.specific = 0;

end:
    return;
}

static void RDR_to_PC_SlotStatus(ccid_device_t *handle)
{
    handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_SLOT_STATUS;
    handle->transport.bulk_msg_header.in.length   = 0;
    handle->transport.bulk_msg_header.in.specific = 0x00;  // bClockStatus = Clock running
}

static void RDR_to_PC_Parameters(ccid_device_t *handle)
{
    handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_PARAMETERS;
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        handle->transport.bulk_msg_header.in.length = 0;
        goto end;
    }
    handle->transport.bulk_msg_header.in.specific = 0x00;  // bProtocolNum = protocol T=0

    handle->transport.rx_msg_buffer[0] = handle->transport.protocol_data.bmFindexDindex;
    handle->transport.rx_msg_buffer[1] = handle->transport.protocol_data.bmTCCKST0;
    handle->transport.rx_msg_buffer[2] = handle->transport.protocol_data.bGuardTimeT0;
    handle->transport.rx_msg_buffer[3] = handle->transport.protocol_data.bWaitingIntegerT0;
    handle->transport.rx_msg_buffer[4] = handle->transport.protocol_data.bClockStop;

    handle->transport.bulk_msg_header.in.length = 5;

end:
    return;
}

static void RDR_to_PC_Escape(ccid_device_t *handle)
{
    handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_ESCAPE;
    handle->transport.bulk_msg_header.in.specific = 0x00;  // bRFU = 0x00
}

static void RDR_to_PC_DataRateAndClockFrequency(ccid_device_t *handle)
{
    handle->transport.bulk_msg_header.in.msg_type = CCID_COMMAND_RDR_TO_PC_DR_CLK_FREQ;
    if (handle->transport.bulk_msg_header.in.error != CCID_ERROR_CMD_NO_ERROR) {
        handle->transport.bulk_msg_header.in.length = 0;
        goto end;
    }
    handle->transport.bulk_msg_header.in.specific = 0x00;  // bRFU = 0x00

end:
    return;
}

/* Exported functions --------------------------------------------------------*/
int32_t CCID_CMD_process(ccid_device_t *handle)
{
    int32_t status = 0;

    if (!handle) {
        return -1;
    }

    handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NO_ERROR;
    handle->transport.bulk_msg_header.in.status
        = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_PROCESSED_WITHOUT_ERROR;

    // Check slot & card insertion
    if (handle->transport.bulk_msg_header.out.slot_number > 1) {
        // Bad slot
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_NOT_PRESENT | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_SLOT_DOES_NOT_EXIST;
    }
    else if ((handle->card_inserted == 0)
             && (handle->transport.bulk_msg_header.out.msg_type
                 != CCID_COMMAND_PC_TO_RDR_ICC_POWER_OFF)
             && (handle->transport.bulk_msg_header.out.msg_type != CCID_COMMAND_PC_TO_RDR_ABORT)) {
        // Card not inserted
        handle->transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_NOT_PRESENT | CCID_SLOT_STATUS_CMD_FAILED;
        handle->transport.bulk_msg_header.in.error = CCID_ERROR_ICC_MUTE;
    }

    switch (handle->transport.bulk_msg_header.out.msg_type) {
        case CCID_COMMAND_PC_TO_RDR_ICC_POWER_ON:
            PC_to_RDR_IccPowerOn(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_ICC_POWER_OFF:
            PC_to_RDR_IccPowerOff(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_GET_SLOT_STATUS:
            PC_to_RDR_GetSlotStatus(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_XFR_BLOCK:
            status = PC_to_RDR_XfrBlock(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_GET_PARAMETERS:
            PC_to_RDR_GetParameters(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_RESET_PARAMETERS:
            PC_to_RDR_ResetParameters(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_SET_PARAMETERS:
            PC_to_RDR_SetParameters(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_ESCAPE:
            PC_to_RDR_Escape(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_ICC_CLOCK:
            PC_to_RDR_IccClock(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_T0_APDU:
            PC_to_RDR_T0APDU(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_SECURE:
            status = PC_to_RDR_Secure(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_MECHANICAL:
            PC_to_RDR_Mechanical(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_ABORT:
            PC_to_RDR_Abort(handle);
            break;
        case CCID_COMMAND_PC_TO_RDR_SET_DR_CLK_FREQ:
            PC_to_RDR_SetDataRateAndClockFrequency(handle);
            break;
        default:
            handle->transport.bulk_msg_header.in.status
                = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_SLOT_STATUS_CMD_FAILED;
            handle->transport.bulk_msg_header.in.error = CCID_ERROR_CMD_NOT_SUPPORTED;
            RDR_to_PC_SlotStatus(handle);
            break;
    }

    return status;
}

uint8_t CCID_CMD_abort(ccid_device_t *handle, uint8_t slot, uint8_t seq)
{
    if (!handle) {
        return 1;
    }

    if (slot > 0) {
        return 1;
    }

    if (!handle->abort.requested) {
        handle->abort.requested = 1;
        handle->abort.seq       = seq;
    }
    else if (handle->abort.seq == seq) {
        handle->abort.requested = 0;
    }

    return 0;
}

void io_usb_ccid_configure_pinpad(uint8_t enabled)
{
    UNUSED(enabled);
}

void io_usb_ccid_set_card_inserted(uint32_t inserted)
{
    UNUSED(inserted);
}
