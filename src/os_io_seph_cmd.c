/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "os.h"
#include "os_io.h"
#include "os_io_seph_cmd.h"

#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const unsigned char seph_io_general_status[] = {
    SEPROXYHAL_TAG_GENERAL_STATUS,
    0,
    2,
    SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND >> 8,
    SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND,
};

static const unsigned char seph_io_request_status[] = {
    SEPROXYHAL_TAG_REQUEST_STATUS,
    0,
    0,
};

static const unsigned char seph_io_device_off[] = {
    SEPROXYHAL_TAG_DEVICE_OFF,
    0,
    0,
};

static const unsigned char seph_io_se_power_off[] = {
    SEPROXYHAL_TAG_SE_POWER_OFF,
    0,
    0,
};

static const unsigned char seph_io_usb_disconnect[] = {
    SEPROXYHAL_TAG_USB_CONFIG,
    0,
    1,
    SEPROXYHAL_TAG_USB_CONFIG_DISCONNECT,
};

static const unsigned char seph_io_cmd_mcu_bootloader[] = {
    SEPROXYHAL_TAG_MCU,
    0,
    1,
    SEPROXYHAL_TAG_MCU_TYPE_BOOTLOADER,
};

static const unsigned char seph_io_cmd_mcu_lock[] = {
    SEPROXYHAL_TAG_MCU,
    0,
    1,
    SEPROXYHAL_TAG_MCU_TYPE_LOCK,
};

#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
static const unsigned char seph_io_mcu_protect[] = {
    SEPROXYHAL_TAG_MCU,
    0,
    1,
    SEPROXYHAL_TAG_MCU_TYPE_PROTECT,
};
#endif  // (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))

static const unsigned char seph_io_cmd_set_ship_mode[] = {
    SEPH_PROTOCOL_CMD_SET_SHIP_MODE,
    0,
    0,
};

static const unsigned char seph_io_cmd_more_time[] = {
    SEPROXYHAL_TAG_MORE_TIME,
    0,
    0,
};

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int io_seph_cmd_general_status(void)
{
    return os_io_tx_cmd(seph_io_general_status, sizeof(seph_io_general_status), NULL);
}

int io_seph_cmd_setup_ticker(unsigned int interval_ms)
{
    uint8_t buffer[5];
    buffer[0] = SEPROXYHAL_TAG_SET_TICKER_INTERVAL;
    buffer[1] = 0;
    buffer[2] = 2;
    buffer[3] = (interval_ms >> 8) & 0xff;
    buffer[4] = (interval_ms) &0xff;
    return os_io_tx_cmd(buffer, 5, NULL);
}

int io_seph_cmd_device_shutdown(void)
{
    return os_io_tx_cmd(seph_io_device_off, sizeof(seph_io_device_off), NULL);
}

int io_seph_cmd_se_reset(void)
{
    return os_io_tx_cmd(seph_io_se_power_off, sizeof(seph_io_se_power_off), NULL);
}

int io_seph_cmd_usb_disconnect(void)
{
    return os_io_tx_cmd(seph_io_usb_disconnect, sizeof(seph_io_usb_disconnect), NULL);
}

int io_seproxyhal_request_mcu_status(void)
{
    return os_io_tx_cmd(seph_io_request_status, sizeof(seph_io_request_status), NULL);
}

#ifdef HAVE_BLE
int io_seph_ble_enable(unsigned char enable)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_UX_CMD;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3]
        = (enable ? SEPROXYHAL_TAG_UX_CMD_BLE_ENABLE_ADV : SEPROXYHAL_TAG_UX_CMD_BLE_DISABLE_ADV);
    return os_io_tx_cmd(buffer, 4, NULL);
}

int io_seph_ble_clear_bond_db(void)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_UX_CMD;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = SEPROXYHAL_TAG_UX_CMD_BLE_RESET_PAIRINGS;
    return os_io_tx_cmd(buffer, 4, NULL);
}

int io_seph_ble_start_factory_test(void)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_BLE_RADIO_POWER;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = SEPROXYHAL_TAG_BLE_RADIO_POWER_FACTORY_TEST;
    return os_io_tx_cmd(buffer, 4, NULL);
}

int io_seph_ux_redisplay(void)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_UX_CMD;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = SEPROXYHAL_TAG_UX_CMD_REDISPLAY;
    return os_io_tx_cmd(buffer, 4, NULL);
}
#endif  // HAVE_BLE

int io_seph_cmd_mcu_go_to_bootloader(void)
{
    return os_io_tx_cmd(seph_io_cmd_mcu_bootloader, sizeof(seph_io_cmd_mcu_bootloader), NULL);
}

int io_seph_cmd_mcu_lock(void)
{
    return os_io_tx_cmd(seph_io_cmd_mcu_lock, sizeof(seph_io_cmd_mcu_lock), NULL);
}

#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
int io_seph_cmd_mcu_protect(void)
{
    return os_io_tx_cmd(seph_io_mcu_protect, sizeof(seph_io_mcu_protect), NULL);
}
#endif  // (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))

int io_seph_cmd_set_ship_mode(void)
{
    return os_io_tx_cmd(seph_io_cmd_set_ship_mode, sizeof(seph_io_cmd_set_ship_mode), NULL);
}

#ifdef HAVE_PRINTF
void io_seph_cmd_printf(const char *str, unsigned int charcount)
{
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_PRINTF;
    hdr[1] = charcount >> 8;
    hdr[2] = charcount;
    os_io_tx_cmd(hdr, 3, NULL);
    os_io_tx_cmd((const uint8_t *) str, charcount, NULL);
}
#endif  // HAVE_PRINTF

#ifdef HAVE_PIEZO_SOUND
int io_seph_cmd_piezo_play_tune(tune_index_e tune_index)
{
    int status = 0;

    uint8_t buffer[4];
    if (tune_index >= NB_TUNES) {
        status = -22;  // EINVAL
        goto end;
    }

    uint32_t sound_setting = os_setting_get(OS_SETTING_PIEZO_SOUND, NULL, 0);

    if ((!IS_NOTIF_ENABLED(sound_setting)) && (tune_index < TUNE_TAP_CASUAL)) {
        goto end;
    }
    if ((!IS_TAP_ENABLED(sound_setting)) && (tune_index >= TUNE_TAP_CASUAL)) {
        goto end;
    }

    buffer[0] = SEPROXYHAL_TAG_PLAY_TUNE;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = (uint8_t) tune_index;
    status    = os_io_tx_cmd(buffer, 4, NULL);

end:
    return status;
}

void io_seproxyhal_play_tune(tune_index_e tune_index)
{
    (void) io_seph_cmd_piezo_play_tune(tune_index);
}
#endif  // HAVE_PIEZO_SOUND

#ifdef HAVE_SERIALIZED_NBGL
void io_seph_cmd_serialized_nbgl(const uint8_t *buffer, uint16_t length)
{
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_NBGL_SERIALIZED;
    hdr[1] = length >> 8;
    hdr[2] = length;
    os_io_tx_cmd(hdr, 3, NULL);
    os_io_tx_cmd(buffer, length, NULL);
}
#endif  // HAVE_SERIALIZED_NBGL

#ifdef HAVE_NOR_FLASH
void io_seph_cmd_spi_cs(uint8_t select)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_SPI_CS;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = select;
    os_io_tx_cmd(buffer, 4, NULL);
}
#endif  // HAVE_NOR_FLASH

int io_seph_cmd_more_time(void)
{
    return os_io_tx_cmd(seph_io_cmd_more_time, sizeof(seph_io_cmd_more_time), NULL);
}

void io_seph_cmd_raw_apdu(const uint8_t *buffer, uint16_t length)
{
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_RAPDU;
    hdr[1] = length >> 8;
    hdr[2] = length;
    os_io_tx_cmd(hdr, 3, NULL);
    os_io_tx_cmd(buffer, length, NULL);
}

void io_seproxyhal_general_status(void)
{
    (void) io_seph_cmd_general_status();
}

void io_seproxyhal_se_reset(void)
{
    io_seph_cmd_se_reset();
    for (;;)
        ;
}

void io_seproxyhal_disable_io(void)
{
    io_seph_cmd_usb_disconnect();
}

void io_seproxyhal_power_off(uint8_t critical_battery)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_DEVICE_OFF;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = critical_battery;
    os_io_tx_cmd(buffer, 4, NULL);
    for (;;)
        ;
}

void io_seph_ble_name_changed(void)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_UX_CMD;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = SEPROXYHAL_TAG_UX_CMD_BLE_NAME_CHANGED;
    (void) os_io_tx_cmd(buffer, 4, NULL);
}

void io_seph_ux_accept_pairing(unsigned char status)
{
    uint8_t buffer[5];
    buffer[0] = SEPROXYHAL_TAG_UX_CMD;
    buffer[1] = 0;
    buffer[2] = 2;
    buffer[3] = SEPROXYHAL_TAG_UX_CMD_ACCEPT_PAIRING;
    buffer[4] = status;
    (void) os_io_tx_cmd(buffer, 5, NULL);
}

void io_seproxyhal_io_heartbeat(void)
{
    // TODO
}
