
/**
 * @file app_monero_start.c
 * @brief Entry point of Monero application, using predefined pages
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
enum {
    BAR_ACCOUNT_TOKEN = FIRST_USER_TOKEN,
    BAR_NETWORK_TOKEN,
    BAR_24_WORDS_TOKEN,
    BAR_RESET_TOKEN,
    ACCOUNT_TOKEN
};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static const char *infoTypes[]    = {"Version", "Developer"};
static const char *infoContents[] = {"1.9.18", "Ledger"};

static const char   *barTexts[] = {"Select account", "Select network", "Show 25 words", "Reset"};
static const uint8_t barTokens[]
    = {BAR_ACCOUNT_TOKEN, BAR_NETWORK_TOKEN, BAR_24_WORDS_TOKEN, BAR_RESET_TOKEN};
static const char *accountNames[] = {"0+", "1", "2", "3", "4", "5", "6", "7"};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static bool navCallback(uint8_t page, nbgl_pageContent_t *content)
{
    if (page == 0) {
        content->type              = BARS_LIST;
        content->barsList.nbBars   = 4;
        content->barsList.barTexts = barTexts;
        content->barsList.tokens   = (uint8_t *) barTokens;
    }
    else if (page == 1) {
        content->type                   = INFOS_LIST;
        content->infosList.nbInfos      = 2;
        content->infosList.infoTypes    = infoTypes;
        content->infosList.infoContents = infoContents;
    }
    return true;
}

static bool accountNavCallback(uint8_t page, nbgl_pageContent_t *content)
{
    if (page == 0) {
        content->type                  = CHOICES_LIST;
        content->choicesList.nbChoices = 4;
        content->choicesList.localized = false;
        content->choicesList.names     = accountNames;
        content->choicesList.token     = ACCOUNT_TOKEN;
    }
    else if (page == 1) {
        content->type                  = CHOICES_LIST;
        content->choicesList.nbChoices = 4;
        content->choicesList.localized = false;
        content->choicesList.names     = accountNames + 4;
        content->choicesList.token     = ACCOUNT_TOKEN;
    }
    return true;
}

static void controlsCallback(int token, uint8_t index)
{
    UNUSED(index);
    if (token == BAR_ACCOUNT_TOKEN) {
        nbgl_useCaseNavigableContent(
            "Select account", 0, 2, app_moneroSettings, accountNavCallback, controlsCallback);
    }
    else if (token == ACCOUNT_TOKEN) {
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Monero application settings page
 *
 */
void app_moneroSettings(void)
{
    nbgl_useCaseNavigableContent("Monero", 0, 2, app_fullMonero, navCallback, controlsCallback);
}
