
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
    SWITCH4_TOKEN,
    SWITCH5_TOKEN
};

/**********************
 *  STATIC VARIABLES
 **********************/
static nbgl_layoutSwitch_t switches[] = {
    {.initState = false,
     .text      = "Blind signing",
     .subText   = "Enable transaction blind signing",
     .token     = SWITCH1_TOKEN,
#ifdef HAVE_PIEZO_SOUND
     .tuneId = TUNE_TAP_CASUAL
#endif
    },
    {.initState = false,
     .text      = "ENS addresses",
     .subText   = "Displays resolved addresses from ENS",
     .token     = SWITCH2_TOKEN,
#ifdef HAVE_PIEZO_SOUND
     .tuneId = TUNE_TAP_CASUAL
#endif
    },
    {.initState = false,
     .text      = "Nonce",
     .subText   = "Displays nonce in transactions.",
     .token     = SWITCH3_TOKEN,
#ifdef HAVE_PIEZO_SOUND
     .tuneId = TUNE_TAP_CASUAL
#endif
    },
    {.initState = false,
     .text      = "Raw messages",
     .subText   = "Displays raw content from EIP712 msg",
     .token     = SWITCH4_TOKEN,
#ifdef HAVE_PIEZO_SOUND
     .tuneId = TUNE_TAP_CASUAL
#endif
    },
    {.initState = false,
     .text      = "Debug contracts",
     .subText   = "Displays contract\ndata details",
     .token     = SWITCH5_TOKEN,
#ifdef HAVE_PIEZO_SOUND
     .tuneId = TUNE_TAP_CASUAL
#endif
    }
};
static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"1.9.18", "Ledger", "Ledger (c) 2025"};

static void controlsCallback(int token, uint8_t index, int page);

static nbgl_content_t contentsList = {.content.switchesList.nbSwitches = 5,
                                      .content.switchesList.switches   = switches,
                                      .type                            = SWITCHES_LIST,
                                      .contentActionCallback           = controlsCallback};

/**********************
 *      VARIABLES
 **********************/
nbgl_genericContents_t eth_settingContents = {.contentsList = &contentsList, .nbContents = 1};
nbgl_contentInfoList_t eth_infosList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void controlsCallback(int token, uint8_t index, int page)
{
    UNUSED(page);
    if (token == SWITCH1_TOKEN) {
        LOG_WARN(APP_LOGGER, "First switch in position %d\n", index);
    }
    switches[token - SWITCH1_TOKEN].initState = (index == ON_STATE) ? true : false;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
