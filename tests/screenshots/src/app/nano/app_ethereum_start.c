
/**
 * @file app_ethereum_start.c
 * @brief Entry point of Ethereum application, using predefined pages
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

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const nbgl_layoutTagValue_t pairs[] = {
    {.item = "Output", .value = "1"},
    {.item = "Amount", .value = "BTC 0.0011"},
    {
     .item  = "To",
     .value = "CELO 0.013563782407139377",
     },
    {.item = "Amount2", .value = "BTC 0.0011"},
    {
     .item  = "To2",
     .value = "bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
     },
    {.item  = "msg/value/funds",
     .value = "{{\"start\":\"{amount\":\"111000000000\",\"proof\"[\"166a71e8dd11c23sfs64804"}
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0,
                                              .nbPairs            = 6,
                                              .wrapping           = true,
                                              .pairs = (nbgl_layoutTagValue_t *) pairs};

static void onTransactionAccept(bool confirm)
{
    app_fullEthereum();
}
static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

static const nbgl_contentInfoList_t infoContentsList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

extern nbgl_genericContents_t eth_settingContents;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_ethereumSignMessage(void)
{
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &C_icon_eye,
                       "Review transaction",
                       NULL,
                       "Accept\nand send",
                       onTransactionAccept);
}

/**
 * @brief Ethereum application start page
 *
 */
void app_fullEthereum(void)
{
    nbgl_useCaseHomeAndSettings("Ethereum",
                                &C_nanox_app_ethereum,
                                NULL,
                                INIT_HOME_PAGE,
                                &eth_settingContents,
                                &infoContentsList,
                                NULL,
                                bolos_ux_dashboard);
}
