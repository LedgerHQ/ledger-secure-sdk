
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
#include "apps_api.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
enum {
    SWITCH1_TOKEN = FIRST_USER_TOKEN,
    SWITCH2_TOKEN,
    SWITCH3_TOKEN,
    SWITCH4_TOKEN
};

/**********************
 *  STATIC VARIABLES
 **********************/
static const nbgl_layoutSwitch_t switches[] = {
    {.initState = false,
     .text      = "ENS addresses",
     .subText   = "Displays the resolved address of ENS domains.",
     .token     = SWITCH1_TOKEN,
     .tuneId    = TUNE_TAP_CASUAL},
    {.initState = true,
     .text      = "Raw messages",
     .subText   = "Displays raw content from EIP712 messages.",
     .token     = SWITCH2_TOKEN,
     .tuneId    = TUNE_TAP_CASUAL},
    {.initState = true,
     .text      = "Nonce",
     .subText   = "Displays nonce information in transactions.",
     .token     = SWITCH3_TOKEN,
     .tuneId    = TUNE_TAP_CASUAL},
    {.initState = true,
     .text      = "Debug smart contracts",
     .subText   = "Displays contract data details.",
     .token     = SWITCH4_TOKEN,
     .tuneId    = TUNE_TAP_CASUAL},
};
static const char *infoTypes[]    = {"Version", "Developer"};
static const char *infoContents[] = {"1.9.18", "Ledger"};

static void controlsCallback(int token, uint8_t index, int page);

static nbgl_content_t contentsList = {.content.switchesList.nbSwitches = 4,
                                      .content.switchesList.switches   = switches,
                                      .type                            = SWITCHES_LIST,
                                      .contentActionCallback           = controlsCallback};

/**********************
 *      VARIABLES
 **********************/
nbgl_genericContents_t eth_settingContents = {.contentsList = &contentsList, .nbContents = 1};
nbgl_contentInfoList_t eth_infosList
    = {.nbInfos = 2, .infoTypes = infoTypes, .infoContents = infoContents};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void controlsCallback(int token, uint8_t index, int page)
{
    UNUSED(page);
    if (token == SWITCH1_TOKEN) {
        LOG_WARN(APP_LOGGER, "First switch in position %d\n", index);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
