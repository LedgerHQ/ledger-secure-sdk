
/**
 * @file app_bitcoin_start.c
 * @brief Entry point of Bitcoin application, using predefined layout
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_use_case.h"
#include "bolos_ux_common.h"
#include "apps_api.h"

/*********************
 *      DEFINES
 *********************/
#define NB_PAIRS 3

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

static void choiceCallback(bool confirm)
{
    UNUSED(confirm);
    bolos_ux_dashboard();
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_bitcoinSignTransaction(void)
{
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &C_nanox_app_bitcoin,
                       "Review transaction",
                       NULL,
                       "Accept and\nSend",
                       choiceCallback);
}

/**
 * @brief Bitcoin application start page
 *
 */
void app_fullBitcoin(void)
{
    nbgl_useCaseHomeAndSettings("Bitcoin",
                                &C_bitcoin_logo,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                &infoContentsList,
                                NULL,
                                bolos_ux_dashboard);
}
