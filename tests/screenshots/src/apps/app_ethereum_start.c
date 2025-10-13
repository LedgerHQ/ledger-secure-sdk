
/**
 * @file app_ethereum_start.c
 * @brief Entry point of Ethereum application, using predefined pages
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

static nbgl_contentTagValue_t skipPairs[] = {
    {.item = "Message",
     .value
     = "I confirm that I have read and agreed to the terms and conditions (paraswap.io/tos3). By "
       "signing this message, you confirm that you are not accessing this app from, or are a..."},
    {
     .item  = "Message",
     .value = "...resident of the USA or any other restricted country.",
     }
};

#ifdef SCREEN_SIZE_WALLET
static nbgl_contentValueExt_t ensExtension
    = {.fullValue   = "0x8aE57A027c63fcA8070D1Bf38622321dE8004c67",
       .explanation = NULL,
       .aliasType   = ENS_ALIAS};
#endif
static nbgl_contentTagValue_t pairs[] = {
    {.item = "Send", .value = "2.325196105098179\n072 ETH"},
    {.item = "Receive Min", .value = "38,287.689054894232100909 RLB"},
    {.item  = "From",
     .value = "rollbot.eth",
#ifdef SCREEN_SIZE_WALLET
     .aliasValue = 1,
     .extension  = &ensExtension
#endif
    },
    {.item = "Input data",
     .value
     = "0x04e45aaf000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc200000000000000000"
       "0000000046eee2cc3188071c02bfc1745a6b17c656e3f3d0000000000000000000000000000000000004e45aaf0"
       "00000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2000000GREGEGEGGGGEGEGE"},
    {.item = "Amount", .value = "8.127467 ETH"}
};

static nbgl_contentTagValue_t pairsBS[] = {
    {.item = "Hash",     .value = "0xC6F0A719DB049E7BC8A6205AF89222DD2078D0EF68A8E2D209E2BC85DF8C8BD4"},
    {.item = "From",     .value = "0x123Aje763D2ee523a2206206994597C13D83de33"                        },
    {.item = "To",       .value = "0x51917F958D2ee523a2206206994597C13D83de3a"                        },
    {.item = "Max Fees", .value = "ETH 0.0047303"                                                     },
};

static nbgl_contentValueExt_t extensionENS
    = {.fullValue   = "0x51917F958D2ee523a2206206994597C13D83de3a",
       .explanation = NULL,
       .aliasType   = ENS_ALIAS,
       .title       = "vitalik.eth"};

static nbgl_contentTagValue_t pairsENS[] = {
    {.item = "From", .value = "0x123Aje763D2ee523a2206206994597C13D83de33"},
    {.item = "Amount", .value = "ETH 0.01062490"},
    {.item = "To", .value = "vitalik.eth", .aliasValue = 1, .extension = &extensionENS},
    {.item = "Max Fees", .value = "ETH 0.0009"},
};

static const char *dAppsInfoTypes[4]
    = {"Contract owner", "Smart contract", "Contract address", "Deployed on"};
static const char *dAppsInfoValues[4] = {"Uniswap Labs\nwww.uniswap.org",
                                         "Uniswap v3 Router 2",
                                         "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D",
                                         "2023-04-28"};

static nbgl_contentInfoList_t dAppInfoList = {.nbInfos        = 4,
                                              .infoTypes      = dAppsInfoTypes,
                                              .infoContents   = dAppsInfoValues,
                                              .infoExtensions = NULL,
                                              .withExtensions = false};
static nbgl_contentValueExt_t extensionDapp
    = {.infolist = &dAppInfoList, .aliasType = INFO_LIST_ALIAS, .title = "vitalik.eth"};

static nbgl_contentTagValue_t pairsDapp[] = {
    {.item = "Interact with", .value = "Uniswap", .aliasValue = 1, .extension = &extensionDapp},
    {.item = "Send", .value = "WETH 2.3251961 "},
    {.item = "Get minimum", .value = "RLB 380000"},
    {.item = "Max Fees", .value = "ETH 0.0029418664 "},
};

static nbgl_contentTagValueList_t pairList
    = {.wrapping = true, .pairs = (nbgl_contentTagValue_t *) pairs};

static uint8_t nbPairsSent;

/**********************
 *      VARIABLES
 **********************/
extern nbgl_genericContents_t eth_settingContents;
extern nbgl_contentInfoList_t eth_infosList;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, app_fullEthereum);
    }
    else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, app_fullEthereum);
    }
}

static void onMessageAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, app_fullEthereum);
    }
    else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, app_fullEthereum);
    }
}

static void onSkip(void)
{
    nbgl_useCaseReviewStreamingFinish("Sign message?", onTransactionAccept);
}

static void onTransactionContinue(bool askMore)
{
    if (askMore) {
        if (nbPairsSent == 0) {
            pairList.nbPairs = 2;
            pairList.pairs   = &skipPairs[nbPairsSent];
            nbPairsSent += pairList.nbPairs;
            nbgl_useCaseReviewStreamingContinueExt(&pairList, onTransactionContinue, onSkip);
        }
        else {
            nbgl_useCaseReviewStreamingFinish("Sign message?", onTransactionAccept);
        }
    }
    else {
        onTransactionAccept(false);
    }
}

void app_ethereumSignMessage(void)
{
    pairList.wrapping = false;
    pairList.pairs    = pairs;
    pairList.nbPairs  = 5;
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &ETH_MAIN_ICON,
                       "Review transaction\nto send Etherum",
                       NULL,
                       "Sign transaction to\nsend Etherum?",
                       onMessageAccept);
}

void app_ethereumReviewBlindSigningTransaction(void)
{
    pairList.pairs   = pairsBS;
    pairList.nbPairs = 4;
    nbgl_useCaseReviewBlindSigning(TYPE_TRANSACTION,
                                   &pairList,
                                   &ETH_MAIN_ICON,
                                   "Review transaction",
                                   NULL,
                                   "Sign transaction to\nsend Etherum?",
                                   NULL,
                                   onTransactionAccept);
}

void app_ethereumReviewENSTransaction(void)
{
    pairList.pairs   = pairsENS;
    pairList.nbPairs = 4;
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &ETH_MAIN_ICON,
                       "Review transaction to send ETH",
                       NULL,
                       "Sign transaction to\nsend Etherum?",
                       onTransactionAccept);
}

void app_ethereumReviewDappTransaction(void)
{
    pairList.pairs   = pairsDapp;
    pairList.nbPairs = 4;
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &ETH_MAIN_ICON,
                       "Review transaction to send ETH",
                       NULL,
                       "Sign transaction to\nsend Etherum?",
                       onTransactionAccept);
}

void app_ethereumSignForwardOnlyMessage(void)
{
    nbPairsSent    = 0;
    pairList.pairs = skipPairs;
    nbgl_useCaseReviewStreamingStart(TYPE_MESSAGE | SKIPPABLE_OPERATION,
                                     &ETH_MAIN_ICON,
                                     "Review message",
                                     NULL,
                                     onTransactionContinue);
}

void app_ethereumSignUnknownLengthMessage(void)
{
    nbPairsSent = 0;
    nbgl_useCaseReviewStreamingStart(
        TYPE_MESSAGE, &ETH_MAIN_ICON, "Review message", NULL, onTransactionContinue);
}

/**
 * @brief Ethereum application start page
 *
 */
void app_fullEthereum(void)
{
    nbgl_useCaseHomeAndSettings("Ethereum",
                                &ETH_MAIN_ICON,
                                NULL,
                                INIT_HOME_PAGE,
                                &eth_settingContents,
                                &eth_infosList,
                                NULL,
                                exit_app);
}

/**
 * @brief Ethereum application start page, with extra action
 *
 */
void app_fullEthereum2(void)
{
#ifdef SCREEN_SIZE_WALLET
    nbgl_homeAction_t homeAction
        = {.callback = NULL, .icon = &WHEEL_ICON, .text = "My accounts", .style = SOFT_HOME_ACTION};
#endif  // SCREEN_SIZE_WALLET
    nbgl_useCaseHomeAndSettings("Ethereum",
                                &ETH_MAIN_ICON,
                                NULL,
                                INIT_HOME_PAGE,
                                &eth_settingContents,
                                &eth_infosList,
#ifdef SCREEN_SIZE_WALLET
                                &homeAction,
#else   // SCREEN_SIZE_WALLET
                                NULL,
#endif  // SCREEN_SIZE_WALLET
                                exit_app);
}
