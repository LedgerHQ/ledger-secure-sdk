
/**
 * @file stubs.c
 * @brief bolos functions stubs
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#include "properties.h"
#include "nbgl_driver.h"
#include "nbgl_draw.h"
#include "nbgl_screen.h"
#include "nbgl_fonts.h"
#include "nbgl_debug.h"
#include "uzlib.h"
#include "glyphs.h"
#include "os_pin.h"
#include "os_endorsement.h"
#include "os_settings.h"
#include "os_seed.h"
#include "os_id.h"
#include "os_nvm.h"
#include "os_pic.h"
#include "os_halt.h"
#include "os_io_seproxyhal.h"
#include "os_screen.h"
#include "os_task.h"
#include "os_helpers.h"
#include "appflags.h"
#include "cx_errors.h"
#include "lcx_rng.h"
#include "ux_loc.h"
#include "sha256.h"

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/
#ifdef HAVE_SE_TOUCH
nbgl_touchStatePosition_t gTouchStatePosition;
#endif
extern bool verbose;

bool         globalError = false;
extern char *dirName;

uint32_t G_interval_ms = 100;

#ifdef HAVE_SE_TOUCH
bool touchEnabled = true;
#endif
/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
///////////////////////////////////////////////////////////////////////////////////////////
// fake implementation of os functions
///////////////////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_BLE
int os_io_ble_cmd_enable(unsigned char enable)
{
    UNUSED(enable);
    return 0;
}
int os_io_ble_cmd_clear_bond_db(void)
{
    return 0;
}
int os_io_ble_cmd_name_changed(void)
{
    return 0;
}

int os_io_ux_cmd_ble_accept_pairing(uint8_t status)
{
    UNUSED(status);
    return 0;
}
#endif  // HAVE_BLE

unsigned int io_button_read(void);

unsigned int io_button_read(void)
{
    return 0;
}

int bytes_to_hex(char *out, size_t outl, const void *value, size_t len)
{
    const uint8_t *bytes = (const uint8_t *) value;
    const char    *hex   = "0123456789ABCDEF";

    if (outl < 2 * len + 1) {
        *out = '\0';
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        *out++ = hex[(bytes[i] >> 4) & 0xf];
        *out++ = hex[bytes[i] & 0xf];
    }
    *out = 0;
    return 0;
}

unsigned short io_seph_recv(unsigned char *buffer, unsigned short maxlength, unsigned int flags)
{
    UNUSED(buffer);
    UNUSED(flags);
    return maxlength;
}

void io_seph_send(const unsigned char *buffer, unsigned short length)
{
    UNUSED(buffer);
    UNUSED(length);
}

#ifdef HAVE_PIEZO_SOUND
int os_io_seph_cmd_piezo_play_tune(tune_index_e tune_index)
{
    UNUSED(tune_index);
    return 0;
}
#endif  // HAVE_PIEZO_SOUND

#ifdef HAVE_SERIALIZED_NBGL
void io_seproxyhal_send_nbgl_serialized(nbgl_serialized_event_type_e event, nbgl_obj_t *obj)
{
    UNUSED(event);
    UNUSED(obj);
}
#endif  // HAVE_SERIALIZED_NBGL

void io_seproxyhal_se_reset(void)
{
    if (verbose) {
        printf("io_seproxyhal_se_reset\n");
    }
}

int os_io_seph_cmd_se_reset(void)
{
    if (verbose) {
        printf("io_seproxyhal_se_reset\n");
    }
    return 0;
}

void io_seproxyhal_disable_io(void) {}

void io_seproxyhal_general_status(void) {}

#ifdef HAVE_BLE
void os_ux_set_status(unsigned int ux_id, unsigned int status)
{
    UNUSED(ux_id);
    UNUSED(status);
}

int os_io_ux_cmd_redisplay(void)
{
    nbgl_screenRedraw();
    return 0;
}
#endif  // HAVE_BLE

void halt(void) {}

void cx_rng_no_throw(uint8_t *buffer, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
#ifdef BUILD_SCREENSHOTS
        buffer[i] = 0;
#else   // BUILD_SCREENSHOTS
        buffer[i] = rand() & 0xFF;
#endif  // BUILD_SCREENSHOTS
    }
}

uint32_t cx_rng_u32_range_func(uint32_t a, uint32_t b, cx_rng_u32_range_randfunc_t randfunc)
{
    uint32_t range = b - a;
    uint32_t r;

    if ((range & (range - 1)) == 0) {  // special case: range is a power of 2
        r = randfunc();
        return a + r % range;
    }

    uint32_t chunk_size       = UINT32_MAX / range;
    uint32_t last_chunk_value = chunk_size * range;
    r                         = randfunc();
    while (r >= last_chunk_value) {
        r = randfunc();
    }
    return a + r / chunk_size;
}

#ifdef HAVE_SE_TOUCH
void touch_get_last_info(io_touch_info_t *info)
{
    info->state = (gTouchStatePosition.state == PRESSED) ? true : false;
    info->x     = gTouchStatePosition.x;
    info->y     = gTouchStatePosition.y;
}
#endif  // HAVE_SE_TOUCH

void os_sched_exit(bolos_task_status_t status_code)
{
    UNUSED(status_code);
    exit(-1);
}

unsigned int os_sched_current_task(void)
{
    return TASK_USER;
}

void nbgl_screen_reinit(void) {}

#ifdef HAVE_STAX_CONFIG_DISPLAY_FAST_MODE
void nbgl_screen_config_fast_mode(uint8_t setting)
{
    UNUSED(setting);
}
#endif  // HAVE_STAX_CONFIG_DISPLAY_FAST_MODE

#ifdef HAVE_STAX_DISPLAY_FAST_MODE
void nbgl_screen_update_temperature(uint8_t temp_degrees)
{
    UNUSED(temp_degrees);
}
#endif  // HAVE_STAX_DISPLAY_FAST_MODE

#ifdef HAVE_SE_EINK_DISPLAY
void nbgl_wait_pipeline(void) {}
#endif

#ifdef HAVE_SE_TOUCH
void touch_set_state(bool enable)
{
    if (enable == touchEnabled) {
        return;
    }
    if (verbose) {
        printf("touch_set_state %s\n", enable ? "ENABLED" : "DISABLED");
    }
    touchEnabled = enable;
}

uint8_t touch_exclude_borders(uint8_t excluded_borders)
{
    return excluded_borders;
}
#endif  // HAVE_SE_TOUCH

void *pic(void *linked_address)
{
    return linked_address;
}
