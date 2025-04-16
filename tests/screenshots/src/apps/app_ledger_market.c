
/**
 * @file app_ledger_market.c
 * @brief Entry point of Ledger Market application, using Use Cases
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

#define NB_PAGES 2

#define VALIDATE_TRANSACTION_TOKEN 15

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
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

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Ledger Market application start page
 *
 */
void app_fullLedgerMarket(void)
{
    nbgl_useCaseHomeAndSettings("Ledger Market",
                                &C_ic_asset_eth_64,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                &infoContentsList,
                                NULL,
                                NULL);
}
