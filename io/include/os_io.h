/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "decorators.h"
#include "os_io_legacy.h"

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
#include "ble_ledger.h"
#endif  // HAVE_BLE

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    OS_IO_PACKET_TYPE_INVALID         = 0x00,
    OS_IO_PACKET_TYPE_SEPH            = 0x10,
    OS_IO_PACKET_TYPE_SE_EVT          = 0x20,
    OS_IO_PACKET_TYPE_RAW_APDU        = 0x30,
    OS_IO_PACKET_TYPE_USB_HID_APDU    = 0x40,
    OS_IO_PACKET_TYPE_USB_WEBUSB_APDU = 0x41,
    OS_IO_PACKET_TYPE_USB_CDC_RAW     = 0x52,
    OS_IO_PACKET_TYPE_BLE_APDU        = 0x60,
    OS_IO_PACKET_TYPE_NFC_APDU        = 0x70,
} os_io_packet_type_t;

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
} os_io_init_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern unsigned char G_io_rx_buffer[IO_APDU_BUFFER_SIZE + 1];
extern unsigned char G_io_tx_buffer[IO_APDU_BUFFER_SIZE + 1];

extern uint16_t      G_io_seph_rx_buffer_length;
extern unsigned char G_io_seph_rx_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B + 1];

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
