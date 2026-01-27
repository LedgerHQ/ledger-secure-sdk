#pragma once
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "nbgl_touch.h"
#include <openssl/evp.h>

/**********************
 *     DEFINE
 **********************/
#ifndef MAX_MATH
#define MAX_PATH 255
#endif  // MAX_PATH

#define NB_MAX_PAGES 1024

#define NB_MAX_STRINGS_PER_PAGE 20

#define DEFAULT_BATT_LEVEL 100
#define NB_MAX_PAGE_IDS    256

#ifdef SCREEN_SIZE_WALLET
#define INVALID_STRING_ID INVALID_LOC_STRING
#else  // SCREEN_SIZE_WALLET
#define INVALID_STRING_ID INVALID_ID
#endif  // SCREEN_SIZE_WALLET

#define MAX_TEXT_ISSUES (NB_MAX_PAGES * NB_MAX_STRINGS_PER_PAGE)

/**********************
 *     ENUMS
 **********************/

// Battery events
enum {
    NO_BATT_EVENT = 0,
    CHARGING_ISSUE_EVENT,
    NO_CHARGING_ISSUE_EVENT,
    CHARGING_START,
    CHARGING_TEMP_TOO_LOW,
    NO_CHARGING_TEMP_TOO_LOW,
    CHARGING_TEMP_TOO_HIGH,
    NO_CHARGING_TEMP_TOO_HIGH,
    BATT_TEMP_TOO_CRITICAL
};

// Application events
enum {
    BTC_SIGN = 0,
    CARDANO_SIGN,
    STELLAR_SIGN,
    STELLAR_STREAM_SIGN,
    BTC_VERIFY_ADDR,
    CARDANO_VERIFY_ADDR,
    BTC_OPEN,
    ETH_OPEN,
    ETH_OPEN_2,
    MONERO_OPEN,
    MON_MESSAGE_SKIP,
    ETH_MESSAGE_SKIP,
    ETH_SIGN,
    ETH_BLIND,
    ETH_ENS,
    ETH_DAPP,
    ETH_VERIFY_ADDR,
    ETH_VERIFY_MULTISIG,
    BTC_SHARE_ADDR,
    RECOV_OPEN,
    DOGE_SIGN,
    DOGE_PRELUDE_SIGN
};

/**********************
 *     TYPEDEF
 **********************/

typedef struct {
    const char *name;
    uint8_t     id;
} Event_t;

// list of page indexes (in pages[] variable) for a given string Id (in BOLOS_UX_LOC_STRINGS enum)
typedef struct {
    uint32_t nbPages;
    uint32_t Ids[NB_MAX_PAGE_IDS];
} PageIds_t;

typedef struct {
#ifdef HAVE_SE_TOUCH
    uint16_t x;
    uint16_t y;
    uint8_t  objTouchId;
    char     objTouchSubId;
#else   // HAVE_SE_TOUCH
    uint8_t keyState;
#endif  // HAVE_SE_TOUCH
    char        *name;
    uint16_t     wait;
    uint16_t     wait_initial;
    uint8_t      power_press;
    uint8_t      forced_key_press;
    uint8_t      ble_event_idx;
    uint8_t      batt_level;
    bool         auto_transition;
    bool         reset;  // if true, reset the counter of parent
    uint8_t      batt_event_idx;
    uint8_t      ux_event_idx;
    uint8_t      app_event_idx;
    uint8_t      seed_algorithm;
    uint32_t     param;
    uint32_t     os_flags;
    uint16_t     long_press_wait;
    unsigned int features;
} PageStep_t;

typedef struct {
    void *next_issue;
    char *name;      // name of this scenario page
    bool  optional;  // if set to true, it means it's a page only existing for some langs, so saving
                     // is not mandatory
    uint32_t      nbSteps;       // number of steps for this scenario page
    PageStep_t   *steps;         // dynamic array of steps for this scenario page
    uint32_t      currentStep;   // current step in steps[] array
    bool          saved;         // set to true once the page is saved (.png file created)
    uint32_t      nb_sub_pages;  // for Nanos, when a string is plit on several sub pages
    uint32_t      cur_sub_page;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_len;
} ScenarioPage_t;

typedef struct {
    uint16_t    id;
    const char *text;
} StringDigits_t;

/**********************
 *   GLOBAL VARIABLES
 **********************/
extern ScenarioPage_t *current_scenario_page;

/**********************
 *   GLOBAL PROTOTYPES
 **********************/
void store_string_infos(const char    *text,
                        nbgl_font_id_e font_id,
                        nbgl_area_t   *area,
                        bool           wrapping,
                        uint16_t       nb_lines,
                        uint16_t       nb_pages,
                        bool           bold);

void store_button_infos(nbgl_area_t *button_area);

uint8_t strxcmp(char *str1, char *str2);
bool    same_string(uint16_t string_id, char *text);

void  scenario_parse_args(int argc, char **argv);
int   scenario_parse_json(void);
int   scenario_get_position(nbgl_touchStatePosition_t *touchStatePosition, bool previousState);
int   scenario_get_keys(uint8_t *state, uint8_t previousState);
int   scenario_get_features(void);
void  scenario_save_json(void);
char *scenario_get_current_page(void);

int save_png(char *framebuffer, char *fullPath, uint16_t width, uint16_t height);
int scenario_save_screen(char *framebuffer, nbgl_area_t *area);
