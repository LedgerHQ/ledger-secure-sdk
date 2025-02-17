
/**
 * @file app_dogecoin_start.c
 * @brief Entry point of Dogecoin application, using use case API
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
#define VALUE_TOKEN (FIRST_USER_TOKEN + 1)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static void contentActionCallback(int token, uint8_t index, int page);

static nbgl_contentValueExt_t extension
    = {.fullValue   = "bc1pkdcufjh6dxjaaa05hudvxqg5fhspfmwmp8g92gq8cv4gwwnmgrfqfd4jlg",
       .explanation = NULL,
       .aliasType   = ENS_ALIAS};

static nbgl_contentTagValue_t pairs[] = {
    {.item = "Amount", .value = "0.0001 BTC"},
    {.item = "To", .value = "toto.eth", .extension = &extension, .aliasValue = 1},
    {.item = "Fees", .value = "0.000000698 BTC"},
};

static nbgl_contentTagValueList_t pairList = {.nbMaxLinesForValue = 0,
                                              .nbPairs            = 3,
                                              .wrapping           = true,
                                              .pairs          = (nbgl_contentTagValue_t *) pairs,
                                              .token          = VALUE_TOKEN,
                                              .actionCallback = contentActionCallback};

static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

static const nbgl_contentInfoList_t infoContentsList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

static nbgl_warning_t warning = {0};

static const char *dAppsInfoTypes[3] = {"Contract owner", "Contract", "Deployed on"};
static const char *dAppsInfoValues[3]
    = {"Uniswap Labs\nwww.uniswap.org", "Uniswap v3 Router 2", "2023-04-28"};
static const nbgl_contentValueExt_t dAppsInfoExtensions[3] = {
    {0},
    {.aliasType   = QR_CODE_ALIAS,
     .title       = "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D",
     .fullValue   = "0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D",
     .explanation = "Scan to view on Etherscan"},
    {0}
};
static nbgl_tipBox_t tipBox = {0};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void validatePinCallback(const uint8_t *content, uint8_t page)
{
    UNUSED(content);
    UNUSED(page);
    app_fullDogecoin();
}

static void touchCallback(int token, uint8_t index)
{
    UNUSED(token);
    UNUSED(index);
}

static void actionCallback(void)
{
    nbgl_useCaseKeypadDigits("my title",
                             4,
                             8,
                             FIRST_USER_TOKEN,
                             false,
                             NBGL_NO_TUNE,
                             validatePinCallback,
                             touchCallback);
}

static void contentActionCallback(int token, uint8_t index, int page)
{
    printf("contentActionCallback : token = %d, index = %d, page = %d\n", token, index, page);
}

static void onTransactionAccept(bool confirm)
{
    if (confirm) {
        nbgl_useCaseStatus("Transaction signed", true, app_fullDogecoin);
    }
    else {
        nbgl_useCaseStatus("Transaction rejected", false, app_fullDogecoin);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_dogecoinSignTransaction(bool blind,
                                 bool dApp,
                                 bool w3c_enabled,
                                 bool w3c_issue,
                                 bool w3c_threat)
{
    nbgl_tipBox_t *tipBoxPtr = NULL;
    warning.predefinedSet    = 0;

    if (blind) {
        warning.predefinedSet |= 1 << BLIND_SIGNING_WARN;
    }
    if (w3c_enabled) {
        if (w3c_issue) {
            warning.predefinedSet |= 1 << W3C_ISSUE_WARN;
        }
        else if (w3c_threat) {
            warning.predefinedSet |= 1 << W3C_THREAT_DETECTED_WARN;
        }
        else {
            warning.predefinedSet |= 1 << W3C_NO_THREAT_WARN;
        }
    }
    if (dApp) {
        warning.predefinedSet |= 1 << VERIFIED_DAPP_WARN;
        tipBoxPtr                   = &tipBox;
        tipBox.text                 = "Interaction with official dApp: Uniswap";
        tipBox.modalTitle           = "Smart contract details";
        tipBox.infos.nbInfos        = 3;
        tipBox.infos.infoTypes      = dAppsInfoTypes;
        tipBox.infos.infoContents   = dAppsInfoValues;
        tipBox.infos.infoExtensions = dAppsInfoExtensions;
        tipBox.infos.withExtensions = true;
        tipBox.type                 = INFOS_LIST;
    }
    warning.reportProvider = "Blockaid";
    warning.providerMessage
        = "The address is linked to a scammer. Your funds will be stolen if you continue.";
    warning.reportUrl    = "url.com/od24xz";
    warning.dAppProvider = "Uniswap";

    // Start review
    nbgl_useCaseAdvancedReview(TYPE_TRANSACTION,
                               &pairList,
                               &C_ic_asset_doge_64,
                               "Review transaction\nto send Dogecoin",
                               NULL,
                               "Sign transaction to\nsend Dogecoin?",
                               tipBoxPtr,
                               &warning,
                               onTransactionAccept);
}

/**
 * @brief Dogecoin application start page
 *
 */
void app_fullDogecoin(void)
{
    nbgl_homeAction_t homeAction = {.callback = actionCallback,
                                    .icon     = &WHEEL_ICON,
                                    .text     = "action",
                                    .style    = STRONG_HOME_ACTION};
    nbgl_useCaseHomeAndSettings(
        "Dogecoin",
        &C_ic_asset_doge_64,
        "Use Ledger Live to create transactions and confirm them on your Europa.",
        INIT_HOME_PAGE,
        NULL,
        &infoContentsList,
        &homeAction,
        NULL);
}
