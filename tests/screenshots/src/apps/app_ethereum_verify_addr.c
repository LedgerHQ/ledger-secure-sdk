
/**
 * @file app_ethereum_verify_addr.c
 * @brief Verify Ethereum application, using predefined use cases
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
static nbgl_layoutTagValue_t pairs[] = {
    {.item = "tutu",  .value = "myvalue1"},
    {.item = "tutu2", .value = "myvalue2"}
};
static nbgl_contentTagValueList_t tagValueList = {.nbMaxLinesForValue = 6,
                                                  .nbPairs            = 2,
                                                  .pairs              = pairs,
                                                  .smallCaseForValue  = false,
                                                  .wrapping           = false};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void displayAddressCallback(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("ADDRESS\nVERIFIED", true, app_fullEthereum);
    }
    else {
        nbgl_useCaseStatus("Address verification\nrejected", false, app_fullEthereum);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Ethereum application verify address page
 *
 */
void app_ethereumVerifyAddress(void)
{
    nbgl_useCaseAddressReview("bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
                              &tagValueList,
                              &C_ic_asset_eth_64,
                              "Verify Ethereum\naddress",
                              NULL,
                              displayAddressCallback);
}
