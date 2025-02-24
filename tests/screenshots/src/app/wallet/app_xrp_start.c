
/**
 * @file app_xrp_start.c
 * @brief Entry point of Xrp application, using predefined pages
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
    {.item = "msg/value/funds",
     .value
     = "{{\"start\":\"{amount\":\"111000000000\",\"proof\"[\"166a71e8dd11c23sfs64804{\"claim\":\"{"
       "am{\"claim\":\"{amount\":\""
       "112000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "113000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "114000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "115000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "116000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "117000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "118000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "119000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "11A000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "11B000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "11C000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "11D000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "11E000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "11F000000000\",\"proof\"[\"166a71e8dd11c2304{\"claim\":\"{am{\"claim\":\"{amount\":\""
       "120000000000\",\"proof\"[\"166a71e8dd11c2304{\"claipra"}
};

static const char *infoTypes[2] = {"Contract owner", "Contract address"};
static const char *infoValues[2]
    = {"Uniswap Labs\nwww.uniswap.org", "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D"};

static void onTipBox(int token, uint8_t index, int page);

static void long_press_callback(int token, uint8_t index, int page)
{
    (void) token;
    (void) index;
    (void) page;
    nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, app_fullXrp);
}

static nbgl_content_t contentsList[] = {
    {.type                                       = EXTENDED_CENTER,
     .contentActionCallback                      = onTipBox,
     .content.extendedCenter.contentCenter.icon  = &C_ic_asset_eth_64,
     .content.extendedCenter.contentCenter.title = "Review swap with Uniswap"},
    {.type                                    = TAG_VALUE_LIST,
     .contentActionCallback                   = NULL,
     .content.tagValueList.nbMaxLinesForValue = 0,
     .content.tagValueList.nbPairs            = 6,
     .content.tagValueList.wrapping           = true,
     .content.tagValueList.pairs              = (nbgl_layoutTagValue_t *) pairs},
    {.type                                 = INFO_LONG_PRESS,
     .contentActionCallback                = long_press_callback,
     .content.infoLongPress.icon           = &C_ic_asset_eth_64,
     .content.infoLongPress.text           = "Confirm transaction\nXrp send",
     .content.infoLongPress.longPressText  = "Hold to send",
     .content.infoLongPress.longPressToken = FIRST_USER_TOKEN},
};

static nbgl_contentTagValueList_t tagValueList = {.nbMaxLinesForValue = 0,
                                                  .nbPairs            = 6,
                                                  .wrapping           = true,
                                                  .pairs = (nbgl_layoutTagValue_t *) pairs};

static bool lightReview;

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void onConfirmAbandon(void)
{
    nbgl_useCaseStatus("Transaction rejected", false, app_fullXrp);
}

static void onCancel(void)
{
    nbgl_useCaseConfirm(
        "Reject transaction?", NULL, "Yes, Reject", "Go back to transaction", onConfirmAbandon);
}

static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Transaction signed", true, app_fullXrp);
    }
    else {
        nbgl_useCaseStatus("Transaction rejected", false, app_fullXrp);
    }
}

static void onTipBox(int token, uint8_t index, int page)
{
    static nbgl_genericContents_t contentsInfos     = {0};
    static nbgl_content_t         contentsInfosList = {0};

    UNUSED(token);
    UNUSED(index);
    UNUSED(page);

    // Values to be reviewed
    contentsInfosList.type                           = INFOS_LIST;
    contentsInfosList.content.infosList.infoTypes    = infoTypes;
    contentsInfosList.content.infosList.infoContents = infoValues;
    contentsInfosList.content.infosList.nbInfos      = 2;

    contentsInfos.contentsList = &contentsInfosList;
    contentsInfos.nbContents   = 1;
    nbgl_useCaseGenericConfiguration("Privacy Report", 0, &contentsInfos, app_xrpSignMessageLight);
}

void app_xrpSignMessage(void)
{
    nbgl_tipBox_t tipBox = {.icon = &C_Important_Circle_64px,
                            .text = "You're interacting with a smart contract from Uniswap Labs.",
                            .modalTitle         = "Smart contract information",
                            .infos.nbInfos      = 2,
                            .infos.infoTypes    = infoTypes,
                            .infos.infoContents = infoValues,
                            .type               = INFOS_LIST};

    lightReview = false;
    nbgl_useCaseAdvancedReview(TYPE_TRANSACTION,
                               &tagValueList,
                               &C_ic_asset_eth_64,
                               "Review swap with Uniswap",
                               NULL,
                               "Sign transaction to\nsend Etherum?",
                               &tipBox,
                               NULL,
                               onTransactionAccept);
}

void app_xrpSignMessageLight(void)
{
    nbgl_genericContents_t contents
        = {.callbackCallNeeded = false, .contentsList = contentsList, .nbContents = 3};

    lightReview = true;
    contentsList[0].content.extendedCenter.tipBox.text
        = "You're interacting with a smart contract from Uniswap Labs.";
    contentsList[0].content.extendedCenter.tipBox.icon  = &C_Important_Circle_64px;
    contentsList[0].content.extendedCenter.tipBox.token = FIRST_USER_TOKEN;

    nbgl_useCaseGenericReview(&contents, "Reject", onCancel);
}

/**
 * @brief Xrp application start page
 *
 */
void app_fullXrp(void)
{
    nbgl_useCaseHomeAndSettings("Xrp", &C_xrp_32px, NULL, INIT_HOME_PAGE, NULL, NULL, NULL, NULL);
}
