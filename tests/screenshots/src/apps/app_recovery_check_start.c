
/**
 * @file app_xrp_start.c
 * @brief Entry point of Xrp application, using predefined pages
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_draw.h"
#include "nbgl_front.h"
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
static const char            *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char            *infoContents[] = {"1.9.18", "Ledger", "Ledger (c) 2025"};
static nbgl_contentInfoList_t rec_infosList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Recovery Check application start page
 *
 */
void app_fullRecoveryCheck(void)
{
#ifdef SCREEN_SIZE_WALLET
    nbgl_homeAction_t homeAction
        = {.callback = NULL, .icon = NULL, .text = "Start check", .style = STRONG_HOME_ACTION};
#endif  // SCREEN_SIZE_WALLET
    nbgl_useCaseHomeAndSettings("Recovery Check",
                                &REC_CHECK_MAIN_ICON,
                                "This app lets you enter a Secret Recovery Phrase and test if it "
                                "matches the one present on this Ledger.",
                                INIT_HOME_PAGE,
                                NULL,
                                &rec_infosList,
#ifdef SCREEN_SIZE_WALLET
                                &homeAction,
#else   // SCREEN_SIZE_WALLET
                                NULL,
#endif  // SCREEN_SIZE_WALLET
                                exit_app);
}
