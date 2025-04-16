
/**
 * @file app_bitcoin_verify_addr.c
 * @brief Entry point of Bitcoin application, using predefined layout
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

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void displayAddressCallback(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Address verified", true, app_fullBitcoin);
    }
    else {
        nbgl_useCaseStatus("Address verification\ncancelled", false, app_fullBitcoin);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Bitcoin application address verification
 *
 */
void app_bitcoinVerifyAddress(void)
{
    nbgl_useCaseAddressReview("bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
                              NULL,
                              &C_ic_asset_btc_64,
                              "Verify Bitcoin address",
                              NULL,
                              displayAddressCallback);
}
