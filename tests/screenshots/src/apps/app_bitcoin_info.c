
/**
 * @file app_bitcoin_info.c
 * @brief Info page of Bitcoin application, using use case API
 *
 */

/*********************
 *      INCLUDES
 *********************/
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
static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool navCallback(uint8_t page, nbgl_pageContent_t *content)
{
    UNUSED(page);
    content->type                   = INFOS_LIST;
    content->infosList.nbInfos      = 3;
    content->infosList.infoTypes    = infoTypes;
    content->infosList.infoContents = infoContents;
    return true;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Bitcoin application infos page
 *
 */
void app_bitcoinInfos(void)
{
    nbgl_useCaseNavigableContent("Bitcoin", 0, 1, app_fullBitcoin, navCallback, NULL);
}
