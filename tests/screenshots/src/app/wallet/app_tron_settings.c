
/**
 * @file app_tron_settings.c
 * @brief Settings page of Tron application, using predefined pages
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_page.h"
#include "nbgl_obj.h"
#include "apps_api.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
enum {
    QUIT_TOKEN,
    NAV_TOKEN,
    BAR_ACCOUNT_TOKEN,
    BAR_NETWORK_TOKEN,
    QUIT_ACCOUNTS_TOKEN,
    ACCOUNT_TOKEN,
    NAV_ACCOUNTS_TOKEN,
    QUIT_NETWORKS_TOKEN,
    NETWORK_TOKEN
};

/**********************
 *  STATIC VARIABLES
 **********************/
static nbgl_page_t *pageContext;

static const char   *infoTypes[]    = {"Tron", "Spec", "App"};
static const char   *infoContents[] = {"(c) 2022 Ledger SAS", "1.0", "1.7.8"};
static const char   *accountNames[] = {"0+", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
static const char   *networkNames[] = {"Main network", "Stage network", "Test network"};
static const char   *barTexts[]     = {"Select account", "Select network"};
static const uint8_t barTokens[]    = {BAR_ACCOUNT_TOKEN, BAR_NETWORK_TOKEN};
static uint8_t       currentPage;
static uint8_t       currentAccountPage;
static uint8_t       chosenAccount = 0;

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void pageCallback(int token, uint8_t index);

static void displayPage(uint8_t page)
{
    nbgl_pageNavigationInfo_t info    = {.activePage                = page,
                                         .nbPages                   = 2,
                                         .navType                   = NAV_WITH_BUTTONS,
                                         .navWithButtons.navToken   = NAV_TOKEN,
                                         .navWithButtons.quitButton = true,
                                         .quitToken                 = QUIT_TOKEN};
    nbgl_pageContent_t        content = {.title = "Tron settings", .isTouchableTitle = false};
    if (page == 0) {
        content.type              = BARS_LIST;
        content.barsList.nbBars   = 2;
        content.barsList.barTexts = barTexts;
        content.barsList.tokens   = (uint8_t *) barTokens;
    }
    else if (page == 1) {
        content.type                   = INFOS_LIST;
        content.infosList.nbInfos      = 3;
        content.infosList.infoTypes    = infoTypes;
        content.infosList.infoContents = infoContents;
    }
    if (pageContext != NULL) {
        nbgl_pageRelease(pageContext);
    }
    pageContext = nbgl_pageDrawGenericContent(&pageCallback, &info, &content);
}

static void displayAccountPage(uint8_t page)
{
    nbgl_pageNavigationInfo_t info    = {.activePage                = page,
                                         .nbPages                   = 2,
                                         .navType                   = NAV_WITH_BUTTONS,
                                         .navWithButtons.navToken   = NAV_ACCOUNTS_TOKEN,
                                         .navWithButtons.quitButton = false};
    nbgl_pageContent_t        content = {
               .title                 = "Select account",
               .isTouchableTitle      = true,
               .titleToken            = QUIT_ACCOUNTS_TOKEN,
               .type                  = CHOICES_LIST,
               .choicesList.nbChoices = 5,
               .choicesList.localized = false,
               .choicesList.names     = accountNames,
               .choicesList.token     = ACCOUNT_TOKEN,
    };
    if (page == 0) {
        if (chosenAccount >= 5) {
            content.choicesList.initChoice = 0xFF;  // no choice
        }
        else {
            content.choicesList.initChoice = chosenAccount;
        }
    }
    else if (page == 1) {
        if (chosenAccount < 5) {
            content.choicesList.initChoice = 0xFF;  // no choice
        }
        else {
            content.choicesList.initChoice = chosenAccount - 5;
        }
        content.choicesList.nbChoices = 5;
        content.choicesList.names     = &accountNames[5];
    }
    currentAccountPage = page;
    if (pageContext != NULL) {
        nbgl_pageRelease(pageContext);
    }
    pageContext = nbgl_pageDrawGenericContent(&pageCallback, &info, &content);
}

static void pageCallback(int token, uint8_t index)
{
    LOG_WARN(APP_LOGGER, "pageTouchCallback(): token = %d, index = %d\n", token, index);
    if (token == QUIT_TOKEN) {
        nbgl_pageRelease(pageContext);
        app_fullTron();
    }
    else if (token == NAV_TOKEN) {
        if (index == EXIT_PAGE) {
            nbgl_pageRelease(pageContext);
            app_fullTron();
        }
        else if (index < 2) {
            displayPage(index);
        }
    }
    else if (token == BAR_ACCOUNT_TOKEN) {
        displayAccountPage(0);
    }
    else if (token == BAR_NETWORK_TOKEN) {
        nbgl_pageContent_t content = {.title                 = "Select network",
                                      .isTouchableTitle      = true,
                                      .titleToken            = QUIT_NETWORKS_TOKEN,
                                      .type                  = CHOICES_LIST,
                                      .choicesList.nbChoices = 3,
                                      .choicesList.localized = false,
                                      .choicesList.names     = networkNames,
                                      .choicesList.token     = NETWORK_TOKEN};
        if (pageContext != NULL) {
            nbgl_pageRelease(pageContext);
        }
        pageContext = nbgl_pageDrawGenericContent(&pageCallback, NULL, &content);
    }
    else if (token == ACCOUNT_TOKEN) {
        chosenAccount = index + currentAccountPage * 5;
    }
    else if (token == QUIT_ACCOUNTS_TOKEN) {
        displayPage(0);
    }
    else if (token == NAV_ACCOUNTS_TOKEN) {
        displayAccountPage(index);
    }
    else if (token == NETWORK_TOKEN) {
    }
    else if (token == QUIT_NETWORKS_TOKEN) {
        displayPage(0);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Tron application settings page
 *
 */
void app_tronSettings(void)
{
    currentPage = 0;
    pageContext = NULL;
    displayPage(0);
}
