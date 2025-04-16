
/**
 * @file app_cardano_start.c
 * @brief Entry point of Cardano application, using use case API
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
     .item  = "To",
     .value = "addr1qyyp7u0s2zcxadzd5w9w8rzpcg0rzmw07v0tkurg32y8agjnfsh6w2r9tvuq0lrsktztgj845qdc"
                 "z4lpa2x4e0me68mqpp4tzn", },
    {.item = "Amount", .value = "8.127467 ADA"},
    {.item = "Fees", .value = "0.175157 ADA"}
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0, .wrapping = true};

static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

static const nbgl_contentInfoList_t infoContentsList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

static uint8_t nbPairsSent;

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Transaction signed", true, app_fullCardano);
    }
    else {
        nbgl_useCaseStatus("Transaction rejected", false, app_fullCardano);
    }
}

static void onTransactionContinue(bool askMore)
{
    if (askMore) {
        if (nbPairsSent < 3) {
            if (nbPairsSent == 0) {
                pairList.nbPairs = 3;
                pairList.pairs   = pairs;
            }
            else {
                pairList.nbPairs = 1;
                pairList.pairs   = &pairs[2];
            }
            nbPairsSent += pairList.nbPairs;
            nbgl_useCaseReviewStreamingContinue(&pairList, onTransactionContinue);
        }
        else {
            nbgl_useCaseReviewStreamingFinish("Sign transaction to transfer Cardano?",
                                              onTransactionAccept);
        }
    }
    else {
        onTransactionAccept(false);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_cardanoSignTransaction(void)
{
    nbPairsSent = 0;
    nbgl_useCaseReviewStreamingStart(TYPE_TRANSACTION,
                                     &C_ic_asset_cardano_64,
                                     "Review transaction",
                                     NULL,
                                     onTransactionContinue);
}

/**
 * @brief Cardano application start page
 *
 */
void app_fullCardano(void)
{
    nbgl_useCaseHomeAndSettings(
        "Cardano",
        &C_ic_asset_cardano_64,
        "Use Ledger Live to create transactions and confirm them on your Europa.",
        INIT_HOME_PAGE,
        NULL,
        &infoContentsList,
        NULL,
        NULL);
}
