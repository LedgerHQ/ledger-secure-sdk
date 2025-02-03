
/**
 * @file app_ethereum_settings.c
 * @brief Settings page of Ethereum application, using predefined pages
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_use_case.h"
#include "bolos_ux_common.h"
#include "apps_api.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    const char  *text;       ///< main text for the switch
    nbgl_state_t initState;  ///< initial state of the switch
} Switch_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static void contentActionCallback(int token, uint8_t index, int page);

static nbgl_contentSwitch_t switches[] = {
    {.initState = false, .text = "Blind signing", .token = 0},
    {.initState = true,  .text = "Debug",         .token = 1},
    {.initState = true,  .text = "Nonce",         .token = 2},
};

static const nbgl_contentSwitchesList_t switchesList = {.nbSwitches = 3, .switches = switches};

static nbgl_content_t contents[] = {
    {.type                  = SWITCHES_LIST,
     .content.switchesList  = switchesList,
     .contentActionCallback = contentActionCallback}
};

/**********************
 *      VARIABLES
 **********************/
nbgl_genericContents_t eth_settingContents = {.contentsList = contents, .nbContents = 1};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void contentActionCallback(int token, uint8_t index, int page)
{
    UNUSED(index);
    UNUSED(page);
    switches[token].initState = !switches[token].initState;
    // nbgl_useCaseSettings(page, 3, app_fullEthereum, navCallback, actionCallback);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
