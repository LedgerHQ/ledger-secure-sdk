/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
	U2F_TRANSPORT_TYPE_USB_HID = 0x00,
	U2F_TRANSPORT_TYPE_BLE     = 0x01,
} u2f_transport_type_t;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    u2f_transport_type_t type;

    uint32_t cid;
    uint8_t  state;

    const uint8_t *tx_message_buffer;
    uint16_t       tx_message_length;
    uint16_t       tx_message_sequence_number;
    uint16_t       tx_message_offset;

    uint8_t *tx_packet_buffer;
    uint16_t tx_packet_buffer_size;
    uint8_t  tx_packet_length;

    uint8_t *rx_message_buffer;
    uint16_t rx_message_buffer_size;
    uint16_t rx_message_expected_sequence_number;
    uint16_t rx_message_length;
    uint16_t rx_message_offset;

} u2f_transport_t;

typedef struct {
    uint8_t protocol_version;
    uint8_t major_device_version_number;
    uint8_t minor_device_version_number;
    uint8_t build_device_version_number;
    uint8_t capabilities_flag; // u2f_hid_capability_t Mask
} u2f_settings_t;

/* Exported defines   --------------------------------------------------------*/
#define U2F_FORBIDDEN_CID (0x00000000)
#define U2F_BROADCAST_CID (0xFFFFFFFF)

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
void U2F_TRANSPORT_init(u2f_transport_t *handle, uint8_t type);
void U2F_TRANSPORT_rx(u2f_transport_t *handle, uint8_t *buffer, uint16_t length);
void U2F_TRANSPORT_tx(u2f_transport_t *handle, uint8_t cmd, const uint8_t *buffer, uint16_t length);
