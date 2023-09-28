/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "seproxyhal_protocol.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    TUNE_RESERVED,
    TUNE_BOOT,
    TUNE_CHARGING,
    TUNE_LEDGER_MOMENT,
    TUNE_ERROR,
    TUNE_NEUTRAL,
    TUNE_LOCK,
    TUNE_SUCCESS,
    TUNE_LOOK_AT_ME,
    TUNE_TAP_CASUAL,
    TUNE_TAP_NEXT,
    NB_TUNES  // Keep at last position
} tune_index_e;

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

int io_seph_cmd_general_status(void);
int io_seph_cmd_setup_ticker(unsigned int interval_ms);
int io_seph_cmd_device_shutdown(void);
int io_seph_cmd_se_reset(void);
int io_seph_cmd_usb_disconnect(void);

int io_seproxyhal_request_mcu_status(void);

#ifdef HAVE_BLE
int io_seph_ble_enable(unsigned char enable);
int io_seph_ble_clear_bond_db(void);
int io_seph_ble_start_factory_test(void);
int io_seph_ux_redisplay(void);
#endif  // HAVE_BLE

int io_seph_cmd_mcu_go_to_bootloader(void);
int io_seph_cmd_mcu_lock(void);
#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
int io_seph_cmd_mcu_protect(void);
#endif  // (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))

int io_seph_cmd_set_ship_mode(void);

#ifdef HAVE_PRINTF
void io_seph_cmd_printf(const char *str, unsigned int charcount);
#endif  // HAVE_PRINTF

#ifdef HAVE_PIEZO_SOUND
int  io_seph_cmd_piezo_play_tune(tune_index_e tune_index);
void io_seproxyhal_play_tune(tune_index_e tune_index);
#endif  // HAVE_PIEZO_SOUND

#ifdef HAVE_SERIALIZED_NBGL
void io_seph_cmd_serialized_nbgl(const uint8_t *buffer, uint16_t length);
#endif  // HAVE_SERIALIZED_NBGL

#ifdef HAVE_NOR_FLASH
void io_seph_cmd_spi_cs(uint8_t select);
#endif  // HAVE_NOR_FLASH

int io_seph_cmd_more_time(void);

void io_seph_cmd_raw_apdu(const uint8_t *buffer, uint16_t length);

void io_seproxyhal_general_status(void);
void io_seproxyhal_se_reset(void);
void io_seproxyhal_disable_io(void);

void io_seproxyhal_power_off(uint8_t critical_battery);
void io_seph_ble_name_changed(void);
void io_seph_ux_accept_pairing(uint8_t status);

void io_seproxyhal_io_heartbeat(void);
