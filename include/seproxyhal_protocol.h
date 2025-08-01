/*******************************************************************************
 *   (c) 2022-2025 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <os_utils.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/
typedef struct seph_s {
    uint8_t  tag;
    uint16_t size;
    uint8_t *data;
} seph_t;

static inline size_t seph_get_header_size(void)
{
    seph_t tmp = {0};
    return sizeof(tmp.tag) + sizeof(tmp.size);
}

static inline bool seph_parse_header(const uint8_t *buf, size_t size, seph_t *seph)
{
    bool   status      = false;
    seph_t tmp_seph    = {0};
    size_t header_size = seph_get_header_size();
    if (!buf || !seph || (size < header_size)) {
        goto error;
    }
    tmp_seph.tag  = buf[0];
    tmp_seph.size = U2BE(buf, sizeof(tmp_seph.tag));
    if (tmp_seph.size > (size - header_size)) {
        goto error;
    }
    tmp_seph.data = (uint8_t *) ((uintptr_t) &buf[header_size]);
    *seph         = tmp_seph;

    status = true;

error:
    return status;
}

////////////
// EVENTS //
////////////

// EVT : SESSION START
#define SEPROXYHAL_TAG_SESSION_START_EVENT (0x01)
enum seph_protocol_evt_session_start_type {
    SEPROXYHAL_TAG_SESSION_START_EVENT_RECOVERY  = 0x02,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FLASHBACK = 0x04,
    SEPROXYHAL_TAG_SESSION_START_EVENT_BOOTMENU  = 0x08,
};

#define SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_HW_VERSION_POS 12
enum seph_protocol_evt_session_start_feature_mask {
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_USB             = 0x00000001,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_BLE             = 0x00000002,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_TOUCH           = 0x00000004,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_BATTERY         = 0x00000008,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_BUTTON          = 0x00000010,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_NO_SCREEN       = 0x00000000,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG      = 0x00000100,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_SSD1312  = 0x00000300,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_MASK     = 0x00000F00,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_HW_VERSION_MASK = 0x0000F000,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_ISET_MCUSEC     = 0x10000000,
    SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_ISET_MCUBL      = 0x20000000,
};

// EVT : BUTTON
#define SEPROXYHAL_TAG_BUTTON_PUSH_EVENT (0x05)
enum seph_protocol_evt_button_mask {
    SEPROXYHAL_TAG_BUTTON_PUSH_EVENT_LEFT  = 0x01,
    SEPROXYHAL_TAG_BUTTON_PUSH_EVENT_RIGHT = 0x02,
};

// EVT : TOUCH
#define SEPROXYHAL_TAG_FINGER_EVENT (0x0C)
enum seph_protocol_evt_finger_mask {
    SEPROXYHAL_TAG_FINGER_EVENT_TOUCH   = 0x01,
    SEPROXYHAL_TAG_FINGER_EVENT_RELEASE = 0x02,
};

// EVT : DISPLAY PROCESSED
#define SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT (0x0D)

// EVT : TICKER
#define SEPROXYHAL_TAG_TICKER_EVENT (0x0E)

// EVT : USB
#define SEPROXYHAL_TAG_USB_EVENT (0x0F)
enum seph_protocol_evt_usb_type {
    SEPROXYHAL_TAG_USB_EVENT_RESET     = 0x01,
    SEPROXYHAL_TAG_USB_EVENT_SOF       = 0x02,
    SEPROXYHAL_TAG_USB_EVENT_SUSPENDED = 0x04,
    SEPROXYHAL_TAG_USB_EVENT_RESUMED   = 0x08,
};

// EVT : USB ENDPOINT XFER
#define SEPROXYHAL_TAG_USB_EP_XFER_EVENT (0x10)
enum seph_protocol_evt_usb_mask {
    SEPROXYHAL_TAG_USB_EP_XFER_SETUP = 0x01,
    SEPROXYHAL_TAG_USB_EP_XFER_IN    = 0x02,
    SEPROXYHAL_TAG_USB_EP_XFER_OUT   = 0x04,
};

// EVT : MCU CHUNK READ RSP
#define SEPROXYHAL_TAG_UNSEC_CHUNK_EVENT (0x12)

// EVT : STATUS
#define SEPROXYHAL_TAG_STATUS_EVENT (0x15)
enum seph_protocol_evt_status_flag_mask {
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_CHARGING          = 0x00000001,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_ON            = 0x00000002,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_BLE_ON            = 0x00000004,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED       = 0x00000008,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_CHARGING_ISSUE    = 0x00000010,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_TEMPERATURE_ISSUE = 0x00000020,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_BATTERY_ISSUE     = 0x00000040,
    SEPROXYHAL_TAG_STATUS_EVENT_FLAG_GAS_GAUGE_ISSUE   = 0x00000080,
};

// EVT : CAPDU
#define SEPROXYHAL_TAG_CAPDU_EVENT (0x16)

// EVT : BLE RX
#define SEPROXYHAL_TAG_BLE_RECV_EVENT (0x18)

// EVT : BOOTLOADER RAPDU
#define SEPROXYHAL_TAG_BOOTLOADER_RAPDU_EVENT (0x19)

// EVT : ITC
#define SEPROXYHAL_TAG_ITC_EVENT (0x1A)
// EVT : UX
#define SEPROXYHAL_TAG_UX_EVENT  (0x1A)

// EVT : POWER BUTTON
#define SEPROXYHAL_TAG_POWER_BUTTON_EVENT (0x1B)

// EVT : NFC APDU EVENT
#define SEPROXYHAL_TAG_NFC_APDU_EVENT (0x1C)

// EVT : NFC EVENT
#define SEPROXYHAL_TAG_NFC_EVENT (0x1E)
enum seph_protocol_evt_nfc_type {
    SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED = 0x01,  // card_detected + type a/b + nfcid[max 7]
    SEPROXYHAL_TAG_NFC_EVENT_CARD_LOST     = 0x02   // card lost
};

enum seph_protocol_evt_nfc_card_detected_type {
    SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED_A = 0x01,
    SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED_B = 0x02
};

// EVT : QI FLASH CHECKSUM
#define SEPROXYHAL_TAG_QI_FLASH_CHECKSUM_EVENT (0x1D)

// EVT : SE_READY_TO_RECEIVE_START_SESSION_EVENT
#define SEPROXYHAL_TAG_SE_READY_TO_RECEIVE_START_SESSION_EVENT (0x1F)

//////////////
// COMMANDS //
//////////////

// CMD : MCU
#define SEPROXYHAL_TAG_MCU (0x31)
enum seph_protocol_cmd_mcu_type {
    SEPROXYHAL_TAG_MCU_TYPE_BOOTLOADER = 0x00,
    SEPROXYHAL_TAG_MCU_TYPE_LOCK       = 0x01,
    SEPROXYHAL_TAG_MCU_TYPE_PROTECT    = 0x02,
    SEPROXYHAL_TAG_MCU_TYPE_BD_ADDR    = 0x03,
};

// CMD : MCU CHUNK READ
#define SEPROXYHAL_TAG_UNSEC_CHUNK_READ_EXT (0x33)
enum seph_protocol_cmd_mcu_chunk_read_type {
    SEPROXYHAL_TAG_UNSEC_CHUNK_READ_EXT_TYPE_IDLE         = 0x00,
    SEPROXYHAL_TAG_UNSEC_CHUNK_READ_EXT_TYPE_FROM_OFFSET  = 0x02,
    SEPROXYHAL_TAG_UNSEC_CHUNK_READ_EXT_TYPE_TOTAL_LENGTH = 0x04,
};

// CMD : NFC POWER
#define SEPROXYHAL_TAG_NFC_POWER (0x34)
enum seph_protocol_cmd_nfc_power_type {
    SEPROXYHAL_TAG_NFC_POWER_OFF       = 0x00,
    SEPROXYHAL_TAG_NFC_POWER_ON_CE     = 0x01,
    SEPROXYHAL_TAG_NFC_POWER_ON_READER = 0x02
};

// CMD : BLE SEND
#define SEPROXYHAL_TAG_BLE_SEND (0x38)

// CMD : SET SCREEN CONFIG
#define SEPROXYHAL_TAG_SET_SCREEN_CONFIG (0x3E)

// CMD : BLE RADIO POWER
#define SEPROXYHAL_TAG_BLE_RADIO_POWER (0x44)
enum seph_protocol_cmd_ble_radio_power_type {
    SEPROXYHAL_TAG_BLE_RADIO_POWER_ACTION_ON     = 0x02,
    SEPROXYHAL_TAG_BLE_RADIO_POWER_ACTION_DBWIPE = 0x04,
    SEPROXYHAL_TAG_BLE_RADIO_POWER_FACTORY_TEST  = 0x40,
};

// CMD : SE POWER OFF
#define SEPROXYHAL_TAG_SE_POWER_OFF (0x46)

// CMD : SPI CS CTRL
#define SEPROXYHAL_TAG_SPI_CS (0x47)

// CMD : NFC_RAPDU
#define SEPROXYHAL_TAG_NFC_RAPDU (0x4A)

// CMD : DEVICE SHUT DOWN
#define SEPROXYHAL_TAG_DEVICE_OFF (0x4B)

// CMD : MORE TIME
#define SEPROXYHAL_TAG_MORE_TIME (0x4C)

// CMD : SET TICKER INTERVAL
#define SEPROXYHAL_TAG_SET_TICKER_INTERVAL (0x4E)

// CMD : USB CONFIG
#define SEPROXYHAL_TAG_USB_CONFIG (0x4F)
enum seph_protocol_cmd_usb_config_type {
    SEPROXYHAL_TAG_USB_CONFIG_CONNECT    = 0x01,
    SEPROXYHAL_TAG_USB_CONFIG_DISCONNECT = 0x02,
    SEPROXYHAL_TAG_USB_CONFIG_ADDR       = 0x03,
    SEPROXYHAL_TAG_USB_CONFIG_ENDPOINTS  = 0x04,
};
enum seph_protocol_cmd_usb_config_end_point_type {
    SEPROXYHAL_TAG_USB_CONFIG_TYPE_DISABLED    = 0x00,
    SEPROXYHAL_TAG_USB_CONFIG_TYPE_CONTROL     = 0x01,
    SEPROXYHAL_TAG_USB_CONFIG_TYPE_INTERRUPT   = 0x02,
    SEPROXYHAL_TAG_USB_CONFIG_TYPE_BULK        = 0x03,
    SEPROXYHAL_TAG_USB_CONFIG_TYPE_ISOCHRONOUS = 0x04,
};

// CMD : USB PREPARE
#define SEPROXYHAL_TAG_USB_EP_PREPARE (0x50)
enum seph_protocol_cmd_usb_prepare_type {
    SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_SETUP   = 0x10,
    SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_IN      = 0x20,
    SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_OUT     = 0x30,
    SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_STALL   = 0x40,
    SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_UNSTALL = 0x80,
};

// CMD : REQUEST STATUS
#define SEPROXYHAL_TAG_REQUEST_STATUS (0x52)

// CMD : RAPDU
#define SEPROXYHAL_TAG_RAPDU (0x53)

// CMD : PIEZO SOUND
#define SEPROXYHAL_TAG_PLAY_TUNE (0x56)

// CMD : SET SHIP MODE
#define SEPROXYHAL_TAG_SET_SHIP_MODE (0x57)

// CMD : QI FLASH
#define SEPROXYHAL_TAG_QI_FLASH (0x58)

// CMD : SET TOUCH_STATE
#define SEPROXYHAL_TAG_SET_TOUCH_STATE (0x5B)

// CMD:  NBGL SERIALIZED
#define SEPROXYHAL_TAG_NBGL_SERIALIZED (0x5C)

// CMD : ITC
#define SEPROXYHAL_TAG_ITC_CMD (0x5D)

// CMD : PRINTF
#define SEPROXYHAL_TAG_PRINTF (0x5F)

// DBG : SCREEN DISPLAY
#define SEPROXYHAL_TAG_DBG_SCREEN_DISPLAY_STATUS (0x5E)

////////////
// STATUS //
////////////
#define SEPROXYHAL_TAG_STATUS_MASK 0x60

// STATUS : GENERAL
#define SEPROXYHAL_TAG_GENERAL_STATUS (0x60)
enum seph_protocol_cmd_general_status_type {
    SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND = 0x0000,
};

// STATUS : SCREEN DISPLAY
#define SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS (0x65)

// STATUS : BOOTLOADER CAPDU
#define SEPROXYHAL_TAG_BOOTLOADER_CAPDU_STATUS (0x6A)

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
