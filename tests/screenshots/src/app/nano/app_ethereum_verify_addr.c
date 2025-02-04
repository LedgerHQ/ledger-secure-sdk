
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

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void displayAddressCallback(bool confirm)
{
    app_fullEthereum();
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
                              NULL,
                              &C_nanox_app_ethereum,
                              "Verify address",
                              NULL,
                              displayAddressCallback);
}
