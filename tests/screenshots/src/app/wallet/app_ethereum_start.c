
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

static nbgl_contentValueExt_t ensExtension
    = {.fullValue   = "0x8aE57A027c63fcA8070D1Bf38622321dE8004c67",
       .explanation = NULL,
       .aliasType   = ENS_ALIAS};

static nbgl_contentTagValue_t pairs[] = {
    {.item = "Send", .value = "2.325196105098179\n072 ETH"},
    {.item = "Receive Min", .value = "38,287.689054894232100909 RLB"},
    {.item = "From", .value = "rollbot.eth", .aliasValue = 1, .extension = &ensExtension},
 //    {.item = "To contract", .value = "0x68b3465833fb72A70ecDF485E0e4C7bD8665Fc45"},
    {.item = "Input data",
     .value
     = "0x04e45aaf000000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc200000000000000000"
       "0000000046eee2cc3188071c02bfc1745a6b17c656e3f3d0000000000000000000000000000000000004e45aaf0"
       "00000000000000000000000c02aaa39b223fe8d0a0e5c4f27ead9083c756cc2000000GREGEGEGGGGEGEGE"},
    {.item = "Amount", .value = "8.127467 ETH"}
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
    pairList.pairs   = pairs;
    pairList.nbPairs = 5;
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &C_ic_asset_eth_64,
                       "Review transaction\nto send Etherum",
                       NULL,
                       "Sign transaction to\nsend Etherum?",
                       onTransactionAccept);
}

void app_ethereumSignForwardOnlyMessage(void)
{
    nbPairsSent    = 0;
    pairList.pairs = skipPairs;
    nbgl_useCaseReviewStreamingStart(TYPE_MESSAGE | SKIPPABLE_OPERATION,
                                     &C_ic_asset_eth_64,
                                     "Review message",
                                     NULL,
                                     onTransactionContinue);
}

void app_ethereumSignUnknownLengthMessage(void)
{
    nbPairsSent = 0;
    nbgl_useCaseReviewStreamingStart(
        TYPE_MESSAGE, &C_ic_asset_eth_64, "Review message", NULL, onTransactionContinue);
}

/**
 * @brief Ethereum application start page
 *
 */
void app_fullEthereum(void)
{
    nbgl_useCaseHomeAndSettings("Ethereum",
                                &C_ic_asset_eth_64,
                                NULL,
                                INIT_HOME_PAGE,
                                &eth_settingContents,
                                &eth_infosList,
                                NULL,
                                NULL);
}
