
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
static nbgl_contentTagValue_t *getPair(uint8_t pairIndex);

static uint8_t nbPairsSent      = 0;
static uint8_t currentPairIndex = 0;

static nbgl_contentValueExt_t extension
    = {.fullValue   = "bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
       .explanation = NULL,
       .aliasType   = ENS_ALIAS};

static nbgl_contentValueExt_t extension2
    = {.fullValue   = "arersnjvchvbhjvsbjdbvnfdlbvnscknblnldkfnlkbndfnblfdknblkndknb",
       .explanation = NULL,
       .aliasType   = ADDRESS_BOOK_ALIAS};

static nbgl_contentTagValue_t pairs[] = {
    {.item = "To", .value = "toto.mon", .extension = &extension, .aliasValue = 1},
    {.item = "Amount", .value = "8.127467 MON"},
    {.item = "To2", .value = "toto2.mon", .extension = &extension2, .aliasValue = 1},
    {.item = "Amount2", .value = "8.127467 MON"},
    {.item = "Fees", .value = "0.175157 MON"}
};

static nbgl_contentTagValueList_t pairList
    = {.nbMaxLinesForValue = 0, .wrapping = true, .callback = getPair};

static const char *infoTypes[]    = {"Version", "Developer"};
static const char *infoContents[] = {"1.9.18", "Ledger"};

static const char   *barTexts[] = {"Select account", "Select network", "Show 25 words", "Reset"};
static const uint8_t barTokens[]
    = {BAR_ACCOUNT_TOKEN, BAR_NETWORK_TOKEN, BAR_24_WORDS_TOKEN, BAR_RESET_TOKEN};
static const char *accountNames[] = {"0+", "1", "2", "3", "4", "5", "6", "7"};

static void barControlsCallback(int token, uint8_t index, int page);

static nbgl_content_t         contentsList        = {.type                      = BARS_LIST,
                                                     .content.barsList.nbBars   = 4,
                                                     .content.barsList.tokens   = barTokens,
                                                     .content.barsList.barTexts = barTexts,

                                                     .contentActionCallback = barControlsCallback};
static nbgl_content_t         accountContentsList = {.type                           = CHOICES_LIST,
                                                     .content.choicesList.initChoice = 0,
                                                     .content.choicesList.names      = accountNames,
                                                     .content.choicesList.nbChoices  = 8,
                                                     .content.choicesList.token = ACCOUNT_TOKEN,
                                                     .contentActionCallback = barControlsCallback};
static nbgl_genericContents_t accountContents
    = {.contentsList = &accountContentsList, .nbContents = 1};

static nbgl_genericContents_t mon_settingContents
    = {.contentsList = &contentsList, .nbContents = 1};
static nbgl_contentInfoList_t mon_infosList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Transaction signed", true, app_fullCardano);
    }
    else {
        nbgl_useCaseStatus("Transaction rejected", false, app_fullCardano);
    }
}

static nbgl_contentTagValue_t *getPair(uint8_t pairIndex)
{
    return &pairs[currentPairIndex + pairIndex];
}

static void onSkip(void)
{
    nbgl_useCaseReviewStreamingFinish("Sign transaction to transfer Monero?", onTransactionAccept);
}

static void onTransactionContinue(bool askMore)
{
    if (askMore) {
        if (nbPairsSent < 5) {
            if (nbPairsSent == 0) {
                pairList.nbPairs = 4;
            }
            else {
                currentPairIndex += nbPairsSent;
                pairList.nbPairs = 1;
            }
            nbPairsSent += pairList.nbPairs;
            nbgl_useCaseReviewStreamingContinueExt(&pairList, onTransactionContinue, onSkip);
        }
        else {
            nbgl_useCaseReviewStreamingFinish("Sign transaction to transfer Monero?",
                                              onTransactionAccept);
        }
    }
    else {
        onTransactionAccept(false);
    }
}

static void quitAccountCallback(void)
{
    nbgl_useCaseHomeAndSettings(
        "Monero", &C_ic_asset_monero_64, NULL, 0, &mon_settingContents, &mon_infosList, NULL, NULL);
}

static void barControlsCallback(int token, uint8_t index, int page)
{
    UNUSED(page);
    if (token == BAR_ACCOUNT_TOKEN) {
#ifndef BUILD_SCREENSHOTS
        LOG_WARN(APP_LOGGER, "First switch in position %d\n", index);
#endif  // BUILD_SCREENSHOTS
        nbgl_useCaseGenericConfiguration(
            "Select account", 0, &accountContents, quitAccountCallback);
    }
    else if (token == ACCOUNT_TOKEN) {
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_moneroSignForwardOnlyMessage(void)
{
    nbPairsSent      = 0;
    currentPairIndex = 0;
    nbgl_useCaseReviewStreamingStart(
        TYPE_TRANSACTION, &C_ic_asset_monero_64, "Review transaction", NULL, onTransactionContinue);
}

/**
 * @brief Monero application start page
 *
 */
void app_fullMonero(void)
{
    nbgl_useCaseHomeAndSettings("Monero",
                                &C_ic_asset_monero_64,
                                NULL,
                                INIT_HOME_PAGE,
                                &mon_settingContents,
                                &mon_infosList,
                                NULL,
                                NULL);
}
