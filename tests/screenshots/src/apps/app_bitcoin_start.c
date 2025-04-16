
/**
 * @file app_bitcoin_start.c
 * @brief Entry point of Bitcoin application, using use case API
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

/**********************
 *  STATIC VARIABLES
 **********************/
static nbgl_contentTagValue_t pairs[] = {
    {.item = "Amount", .value = "0.0001 BTC"                                                    },
    {.item = "To",     .value = "bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg"},
    {.item = "Fees",   .value = "0.000000698 BTC"                                               },
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0,
                                              .nbPairs            = 3,
                                              .wrapping           = true,
                                              .pairs = (nbgl_contentTagValue_t *) pairs};

static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

static const nbgl_contentInfoList_t infoContentsList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Transaction signed", true, app_fullBitcoin);
    }
    else {
        nbgl_useCaseStatus("Transaction rejected", false, app_fullBitcoin);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_bitcoinSignTransaction(void)
{
    // Start review
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &C_ic_asset_btc_64,
                       "Review transaction\nto send Bitcoin",
                       NULL,
                       "Sign transaction to\nsend Bitcoin?",
                       onTransactionAccept);
}

/**
 * @brief Bitcoin application start page
 *
 */
void app_fullBitcoin(void)
{
    nbgl_useCaseHomeAndSettings(
        "Bitcoin", &C_ic_asset_btc_64, NULL, INIT_HOME_PAGE, NULL, &infoContentsList, NULL, NULL);
}
