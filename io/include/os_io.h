/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "os_math.h"
#include "decorators.h"
#ifdef HAVE_IO_U2F
#include "u2f_transport.h"
#endif  // HAVE_IO_U2F

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    OS_IO_PACKET_TYPE_NONE             = 0x00,
    OS_IO_PACKET_TYPE_SEPH             = 0x01,
    OS_IO_PACKET_TYPE_SE_EVT           = 0x02,
    OS_IO_PACKET_TYPE_RAW_APDU         = 0x10,
    OS_IO_PACKET_TYPE_USB_HID_APDU     = 0x20,
    OS_IO_PACKET_TYPE_USB_WEBUSB_APDU  = 0x21,
    OS_IO_PACKET_TYPE_USB_CCID_APDU    = 0x22,
    OS_IO_PACKET_TYPE_USB_U2F_HID_APDU = 0x23,
    OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR = 0x24,
    OS_IO_PACKET_TYPE_USB_U2F_HID_RAW  = 0x25,
    OS_IO_PACKET_TYPE_USB_CDC_RAW      = 0x29,
    OS_IO_PACKET_TYPE_BLE_APDU         = 0x30,
    OS_IO_PACKET_TYPE_BLE_U2F_APDU     = 0x31,
    OS_IO_PACKET_TYPE_NFC_APDU         = 0x40,
    OS_IO_PACKET_TYPE_NFC_U2F_APDU     = 0x41,
} os_io_packet_type_t;

typedef enum {
    APDU_TYPE_NONE       = OS_IO_PACKET_TYPE_NONE,
    APDU_TYPE_RAW        = OS_IO_PACKET_TYPE_RAW_APDU,
    APDU_TYPE_USB_HID    = OS_IO_PACKET_TYPE_USB_HID_APDU,
    APDU_TYPE_USB_WEBUSB = OS_IO_PACKET_TYPE_USB_WEBUSB_APDU,
    APDU_TYPE_USB_CCID   = OS_IO_PACKET_TYPE_USB_CCID_APDU,
    APDU_TYPE_USB_U2F    = OS_IO_PACKET_TYPE_USB_U2F_HID_APDU,
    APDU_TYPE_BLE        = OS_IO_PACKET_TYPE_BLE_APDU,
    APDU_TYPE_NFC        = OS_IO_PACKET_TYPE_NFC_APDU,
} apdu_type_t;

typedef enum {
    // IO
    ITC_IO_BLE_DISABLE_ADV      = 0x00,
    ITC_IO_BLE_ENABLE_ADV       = 0x01,
    ITC_IO_BLE_RESET_PAIRINGS   = 0x02,
    ITC_IO_BLE_BLE_NAME_CHANGED = 0x03,
    // UX
    ITC_UX_REDISPLAY          = 0x10,
    ITC_UX_ACCEPT_BLE_PAIRING = 0x11,
    ITC_UX_ASK_BLE_PAIRING    = 0x12,
    ITC_UX_BLE_PAIRING_STATUS = 0x13,
    // MISC
    ITC_BUTTON_STATE = 0x20,
    ITC_FINGER_STATE = 0x21,
} itc_type_t;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    uint16_t pid;
    uint16_t vid;
    char    *name;
    uint16_t class_mask;  // usbd_ledger_product_e
} os_io_init_usb_t;

typedef struct {
    uint16_t profile_mask;  // ble_ledger_profile_mask_e
} os_io_init_ble_t;

typedef struct {
    uint8_t          syscall;
    os_io_init_usb_t usb;
    os_io_init_ble_t ble;
#ifdef HAVE_IO_U2F
    u2f_settings_t u2f_settings;
#endif  // HAVE_IO_U2F
} os_io_init_t;

/* Exported defines   --------------------------------------------------------*/
#define OS_IO_FLAG_CACHE 1

#ifdef CUSTOM_IO_APDU_BUFFER_SIZE
#define OS_IO_BUFFER_SIZE CUSTOM_IO_APDU_BUFFER_SIZE
#else  // !CUSTOM_IO_APDU_BUFFER_SIZE
#define OS_IO_BUFFER_SIZE OS_IO_SEPH_BUFFER_SIZE
#endif  // !CUSTOM_IO_APDU_BUFFER_SIZE

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

extern unsigned char G_io_rx_buffer[OS_IO_BUFFER_SIZE + 1];
extern unsigned char G_io_tx_buffer[OS_IO_BUFFER_SIZE + 1];

extern uint8_t G_io_syscall_flag;

/* Exported functions prototypes--------------------------------------------- */
int32_t os_io_init(uint8_t full);
int32_t os_io_de_init(void);
int32_t os_io_stop(void);

SYSCALL int os_io_start(os_io_init_t *init);
SYSCALL int os_io_rx_evt(unsigned char *buffer,
                         unsigned short buffer_max_length,
                         unsigned int  *timeout_ms);
SYSCALL int os_io_tx_cmd(unsigned char               type,  // os_io_packet_type_t
                         const unsigned char *buffer PLENGTH(length),
                         unsigned short              length,
                         unsigned int               *timeout_ms);

SYSCALL int os_io_seph_tx(const unsigned char *buffer PLENGTH(length),
                          unsigned short              length,
                          unsigned int               *timeout_ms);
SYSCALL int os_io_seph_se_rx_event(unsigned char *buffer PLENGTH(length),
                                   unsigned short        max_length,
                                   unsigned int         *timeout_ms,
                                   bool                  check_se_event,
                                   unsigned int          flags);
