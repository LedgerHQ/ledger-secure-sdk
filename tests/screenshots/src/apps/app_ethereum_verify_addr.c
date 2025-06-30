
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

static nbgl_contentTagValue_t tagValuePairs[] = {
    {.item = "Signer 1", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 2", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 3", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 4", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 5", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 6", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 7", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 8", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
};

static nbgl_contentTagValueList_t tagValueList
    = {.nbPairs = 8, .wrapping = false, .pairs = (nbgl_contentTagValue_t *) tagValuePairs};

static nbgl_contentValueExt_t extension
    = {.tagValuelist = &tagValueList, .aliasType = TAG_VALUE_LIST_ALIAS, .title = ""};

static nbgl_contentTagValue_t pairs[] = {
    {.item = "Your role", .value = "Signer"},
    {.item = "Threshold", .value = "3 out of 8", .extension = &extension, .aliasValue = 1},
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
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, app_fullEthereum);
    }
    else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, app_fullEthereum);
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
                              NULL,
                              &ETH_MAIN_ICON,
                              "Verify Ethereum\naddress",
                              NULL,
                              displayAddressCallback);
}

/**
 * @brief Ethereum application verify address page
 *
 */
void app_ethereumVerifyMultiSig(void)
{
    nbgl_useCaseAddressReview("0xe87eD43dE2bf1D340e426920F103A10fD5fb8e47",
                              &pairList,
                              &ETH_MAIN_ICON,
                              "Verify Safe address",
                              NULL,
                              displayAddressCallback);
}
