
/**
 * @file app_algorand_start.c
 * @brief Entry point of Algorand application, using predefined layout
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_layout.h"
#include "nbgl_obj.h"
#include "apps_api.h"
#include <string.h>
#include <stdint.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
enum {
    QUIT_TOKEN,
    SETTINGS_TOKEN,
    KBD_BACK_TOKEN,
    KBD_CONFIRM_TOKEN,
    KBD_TEXT_TOKEN
};

/**********************
 *  STATIC VARIABLES
 **********************/
static nbgl_layout_t *layout;
static int            textIndex, keyboardIndex;
static char           textToEnter[32] = "";

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void keyboardCallback(char touchedKey)
{
    LOG_DEBUG(UX_LOGGER, "keyboardCallback(): touchedKey = %d\n", touchedKey);
    // if not Backspace
    if (touchedKey != BACKSPACE_KEY) {
        int textLen              = strlen(textToEnter);
        textToEnter[textLen]     = touchedKey;
        textToEnter[textLen + 1] = '\0';
        nbgl_layoutUpdateEnteredText(layout, textIndex, true, 12, textToEnter, false);

        if (textLen == 2) {
            nbgl_layoutUpdateKeyboard(layout, keyboardIndex, 0x3456, false, LOWER_CASE);
        }
    }
    else {  // backspace
        int textLen = strlen(textToEnter);
        if (textLen == 0) {
            return;
        }
        textToEnter[textLen - 1] = '\0';
        if (textLen > 1) {
            nbgl_layoutUpdateEnteredText(layout, textIndex, true, 12, textToEnter, false);
        }
        else {
            nbgl_layoutUpdateEnteredText(layout, textIndex, true, 12, "Current text", true);
        }
    }
}

static void layoutTouchCallback(int token, uint8_t index)
{
    LOG_DEBUG(UX_LOGGER, "layoutTouchCallback(): token = %d, index = %d\n", token, index);
    if (token == QUIT_TOKEN) {
        nbgl_layoutRelease(layout);
    }
    else if (token == SETTINGS_TOKEN) {
        //
        nbgl_layoutDescription_t layoutDescription
            = {.modal = false, .onActionCallback = &layoutTouchCallback};
        nbgl_layoutKbd_t          kbdInfo      = {.callback    = keyboardCallback,
                                                  .keyMask     = 0,
                                                  .lettersOnly = false,
                                                  .mode        = MODE_LETTERS,
                                                  .casing      = UPPER_CASE};
        nbgl_layoutCenteredInfo_t centeredInfo = {.text1   = NULL,
                                                  .text2   = "Enter passphrase:",
                                                  .text3   = NULL,
                                                  .style   = LARGE_CASE_INFO,
                                                  .icon    = NULL,
                                                  .offsetY = 0,
                                                  .onTop   = true};
        layout                                 = nbgl_layoutGet(&layoutDescription);
        nbgl_layoutAddCenteredInfo(layout, &centeredInfo);
        keyboardIndex = nbgl_layoutAddKeyboard(layout, &kbdInfo);
        textIndex
            = nbgl_layoutAddEnteredText(layout, true, 12, "current text", true, 32, KBD_TEXT_TOKEN);
        nbgl_layoutDraw(layout);
    }
    else if (token == 142) {
        LOG_WARN(APP_LOGGER, "layoutTouchCallback(): toto\n");
    }
    else if (token == KBD_BACK_TOKEN) {
        nbgl_layoutRelease(layout);
        app_fullAlgorand();
    }
    else if (token == KBD_CONFIRM_TOKEN) {
        nbgl_layoutRelease(layout);
        app_fullAlgorand();
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Algorand application start page
 *
 */
void app_fullAlgorand(void)
{
    nbgl_layoutDescription_t layoutDescription
        = {.modal = false, .onActionCallback = &layoutTouchCallback};
    nbgl_layoutCenteredInfo_t centeredInfo = {
        .text1   = "Algorand",
        .text2   = "Go to Ledger Live to create a\ntransaction. You will approve it\non Fatstacks.",
        .text3   = NULL,
        .style   = LARGE_CASE_INFO,
        .icon    = &C_Algorand32px,
        .offsetY = 0};

    if (layout) {
        nbgl_layoutRelease(layout);
    }
    layout = nbgl_layoutGet(&layoutDescription);
    nbgl_layoutAddTopRightButton(layout, &CLOSE_ICON, QUIT_TOKEN, TUNE_TAP_NEXT);
    nbgl_layoutAddBottomButton(layout, &WHEEL_ICON, SETTINGS_TOKEN, false, TUNE_TAP_NEXT);
    nbgl_layoutAddCenteredInfo(layout, &centeredInfo);

    nbgl_layoutDraw(layout);
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
