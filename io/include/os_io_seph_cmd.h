/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "seproxyhal_protocol.h"
#include "ux.h"

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

int  os_io_seph_cmd_general_status(void);
int  os_io_seph_cmd_more_time(void);
int  os_io_seph_cmd_setup_ticker(unsigned int interval_ms);
int  os_io_seph_cmd_device_shutdown(uint8_t critical_battery);
int  os_io_seph_cmd_se_reset(void);
int  os_io_seph_cmd_usb_disconnect(void);
int  os_io_seph_cmd_mcu_status(void);
int  os_io_seph_cmd_mcu_go_to_bootloader(void);
int  os_io_seph_cmd_mcu_lock(void);
int  os_io_seph_cmd_mcu_protect(void);
void os_io_seph_cmd_raw_apdu(const uint8_t *buffer, uint16_t length);

#ifdef HAVE_SHIP_MODE
int os_io_seph_cmd_set_ship_mode(void);
#endif  // HAVE_SHIP_MODE

#ifdef HAVE_PRINTF
void os_io_seph_cmd_printf(const char *str, unsigned int charcount);
#endif  // HAVE_PRINTF

#ifdef HAVE_PIEZO_SOUND
int os_io_seph_cmd_piezo_play_tune(tune_index_e tune_index);
#endif  // HAVE_PIEZO_SOUND

#ifdef HAVE_SERIALIZED_NBGL
void os_io_seph_cmd_serialized_nbgl(const uint8_t *buffer, uint16_t length);
#endif  // HAVE_SERIALIZED_NBGL

#ifdef HAVE_NOR_FLASH
void os_io_seph_cmd_spi_cs(uint8_t select);
#endif  // HAVE_NOR_FLASH

#ifdef HAVE_BLE
int  os_io_seph_cmd_ble_start_factory_test(void);
int  os_io_ble_cmd_enable(unsigned char enable);
int  os_io_ble_cmd_clear_bond_db(void);
int  os_io_ble_cmd_name_changed(void);
int  os_io_ux_cmd_ble_accept_pairing(uint8_t status);
int  os_io_ux_cmd_redisplay(void);
void os_io_ux_cmd_ble_pairing_request(bolos_ux_params_t *ux_params);
void os_io_ux_cmd_ble_pairing_status(bolos_ux_params_t *ux_params);
#endif  // HAVE_BLE

void os_io_ux_cmd_button_state(uint8_t state);
void os_io_ux_cmd_touch_state(uint8_t state, uint16_t x, uint16_t y, uint8_t w, uint8_t h);
