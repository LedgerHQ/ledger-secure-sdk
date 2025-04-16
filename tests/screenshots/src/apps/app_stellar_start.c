
/**
 * @file app_stellar_start.c
 * @brief Entry point of Stellar application, using predefined layout
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
    {.item = "Memo Text", .value = "hello world"},
    {.item = "Max Fee", .value = "0.00003 XLM"},
    {.item = "Valid Before (UTC)", .value = "2024-12-12 04:12:12"},
    {.item = "Tc Source", .value = "GDUTHC...XM2FN7"},
    {
#ifdef SCREEN_SIZE_WALLET
     .centeredInfo = true,
#endif
     .item  = "Review transaction info",
     .value = "1 of 2"},
    {.item = "Send", .value = "922,337,203,685 XLM"},
    {.item = "Destination", .value = "GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX"},
    {.item = "Op Source", .value = "GDUTHC...XM2FN7"},
    {
#ifdef SCREEN_SIZE_WALLET
     .centeredInfo = true,
#endif
     .item  = "Review operation type",
     .value = "2 of 2"},
    {.item = "Operation Type", .value = "Set Options"},
    {.item = "Home Domain", .value = "stellar.org"}
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0,
                                              .nbPairs            = 11,
                                              .wrapping           = true,
                                              .pairs = (nbgl_contentTagValue_t *) pairs};

static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

static const nbgl_contentInfoList_t infoContentsList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

static uint8_t nbPairsSent;

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/
static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Transaction signed", true, app_fullStellar);
    }
    else {
        nbgl_useCaseStatus("Transaction rejected", false, app_fullStellar);
    }
}

static void onSkip(void)
{
    nbgl_useCaseReviewStreamingFinish("Sign transaction to transfer Stellar?", onTransactionAccept);
}

static void onTransactionContinue(bool askMore)
{
    if (askMore) {
        if (nbPairsSent < 11) {
            if (nbPairsSent == 0) {
                pairList.nbPairs = 5;
            }
            else if (nbPairsSent == 5) {
                pairList.nbPairs = 4;
            }
            else if (nbPairsSent == 9) {
                pairList.nbPairs = 2;
            }
            pairList.pairs = &pairs[nbPairsSent];
            nbPairsSent += pairList.nbPairs;
            nbgl_useCaseReviewStreamingContinueExt(&pairList, onTransactionContinue, onSkip);
        }
        else {
            nbgl_useCaseReviewStreamingFinish("Sign transaction to transfer Stellar?",
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

void app_stellarSignTransaction(void)
{
    // Start review
    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &STELLAR_MAIN_ICON,
                       "Review transaction",
                       NULL,
                       "Sign transaction?",
                       onTransactionAccept);
}

void app_stellarSignStreamedTransaction(void)
{
    nbPairsSent = 0;
    nbgl_useCaseReviewStreamingStart(
        TYPE_TRANSACTION, &STELLAR_MAIN_ICON, "Review transaction", NULL, onTransactionContinue);
}

/**
 * @brief Stellar application start page
 *
 */
void app_fullStellar(void)
{
    nbgl_useCaseHomeAndSettings(
        "Stellar",
        &STELLAR_MAIN_ICON,
        "Use Ledger Live to create transactions and confirm them on your Europa.",
        INIT_HOME_PAGE,
        NULL,
        &infoContentsList,
        NULL,
        exit_app);
}
