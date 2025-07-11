
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
static nbgl_contentTagValue_t pairs[] = {
    {.item = "Account name", .value = "My Main BTC"},
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0,
                                              .nbPairs            = 1,
                                              .wrapping           = true,
                                              .pairs = (nbgl_contentTagValue_t *) pairs};

static nbgl_contentTagValue_t tagValuePairs[] = {
    {.item = "Signer 1", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 2", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 3", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 4", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
    {.item = "Signer 5", .value = "0x20bfb083C5AdaCc91C46Ac4D37905D0447968166"},
};

static nbgl_contentTagValueList_t tagValueList
    = {.nbPairs = 3, .wrapping = false, .pairs = (nbgl_contentTagValue_t *) tagValuePairs};

static nbgl_contentValueExt_t extension
    = {.tagValuelist = &tagValueList, .aliasType = TAG_VALUE_LIST_ALIAS, .title = "vitalik.eth"};

// icons for first level warning details
static const nbgl_icon_details_t *barListIcons[] = {NULL, NULL};

// first level warning details
static const char *const barListTexts[] = {"Public key", "Path"};
static const char *const barListSubTexts[]
    = {"tpubD6NzVbkrYhZ4YQdG8MKuGetgd6qKxcXzDtkp8Mwn1Ny5D7iRYa5Ti3rxFkFHVUxdY1ASVhfzycbZkimVZTSso6w"
       "t3KYj3v4rppNzK5qWsML",
       "(Master Key)"};

// second level warning details in review
static const struct nbgl_genericDetails_s barListIntroDetails[] = {
#ifdef NBGL_QRCODE
    {.title  = "Close",
                      .type   = QRCODE_WARNING,
                      .qrCode = {.url      = "tpubD6NzVbkrYhZ4YQdG8MKuGetgd6sdvdfbZkimVZTSso6wt3KYj3v4rppNzK5qWsML",
                .text1    = "tpubD6NzVbkrYhZ4YQdG8MKuGetgd6...ZkimVZTSso6wt3KYj3v4rppNzK5qWsML",
                .text2    = NULL,
                .centered = true}},
#endif  // NBGL_QRCODE
    {.type = NO_TYPE_WARNING}
};

// first level warning details in prolog
static const nbgl_warningDetails_t details = {.title            = "Public key details",
                                              .type             = BAR_LIST_WARNING,
                                              .barList.nbBars   = 2,
                                              .barList.icons    = barListIcons,
                                              .barList.details  = barListIntroDetails,
                                              .barList.texts    = barListTexts,
                                              .barList.subTexts = barListSubTexts};

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
                              &BTC_MAIN_ICON,
                              "Verify Bitcoin address",
                              NULL,
                              displayAddressCallback);
}

void app_bitcoinVerifyAddressWithAccountName(void)
{
    pairs[0].value      = "My main BTC";
    pairs[0].extension  = &extension;
    pairs[0].aliasValue = 1;
    nbgl_useCaseAddressReview("bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
                              &pairList,
                              &BTC_MAIN_ICON,
                              "Verify Bitcoin address",
                              NULL,
                              displayAddressCallback);
}

void app_bitcoinVerifyAddressWithLongAccountName(void)
{
    pairs[0].value = "My main BTC with a long account name";
    nbgl_useCaseAddressReview("bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
                              &pairList,
                              &BTC_MAIN_ICON,
                              "Verify Bitcoin address",
                              NULL,
                              displayAddressCallback);
}

void app_bitcoinShareAddress(void)
{
    UNUSED(details);
    nbgl_useCaseChoiceWithDetails(&BTC_MAIN_ICON,
                                  "Share Bitcoin\npublic key?",
#ifdef SCREEN_SIZE_WALLET
                                  "This lets the connected wallet access your accounts info.",
#else   // SCREEN_SIZE_WALLET
                                  NULL,
#endif  // SCREEN_SIZE_WALLET
                                  "Share",
                                  "Don't share",
                                  &details,
                                  displayAddressCallback);
}
