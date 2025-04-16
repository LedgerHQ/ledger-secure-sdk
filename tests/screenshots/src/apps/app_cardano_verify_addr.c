
/**
 * @file app_cardano_verify_addr.c
 * @brief Entry point of Cardano address verification
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
    {
     .item  = "Derivation path",
     .value = "m/44'/60'/0'/0",
     },
    {.item = "Staking path", .value = "m/1852'/1815'/0'/2/0"},
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0,
                                              .nbPairs            = 2,
                                              .wrapping           = true,
                                              .pairs = (nbgl_contentTagValue_t *) pairs};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void displayAddressCallback(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Address verified", true, app_fullCardano);
    }
    else {
        nbgl_useCaseStatus("Address verification\ncancelled", false, app_fullCardano);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Cardano application address verification with extra info
 *
 */
void app_cardanoVerifyAddress(void)
{
    nbgl_useCaseAddressReview(
        "addr1qyyp7u0s2zcxadzd5w9w8rzpcg0rzmw07v0tkurg32y8agjnfsh6w2r9tvuq0lrsktztgj845qdcz4lpa2x4e"
        "0me68mqpp4tzn",
        &pairList,
        &CARD_MAIN_ICON,
        "Verify Cardano\naddress",
        NULL,
        displayAddressCallback);
}
