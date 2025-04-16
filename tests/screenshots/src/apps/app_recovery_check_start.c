
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
#include "nbgl_touch.h"
#include "nbgl_use_case.h"
#include "apps_api.h"

/*********************
 *      DEFINES
 *********************/
#define NB_COLS    4
#define NB_ROWS    6
#define KEY_WIDTH  (SCREEN_WIDTH / NB_COLS)
#define KEY_HEIGHT (SCREEN_HEIGHT / NB_ROWS)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static uint32_t    mask;
static const char *infoTypes[]    = {"Version", "Developer", "Copyright"};
static const char *infoContents[] = {"2.0.5", "Ledger", "(c) 2024 Ledger"};

static const nbgl_contentInfoList_t infoContentsList
    = {.nbInfos = 3, .infoTypes = infoTypes, .infoContents = infoContents};
/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void buildScreen(uint32_t digitsMask);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static uint8_t getKeyIndex(uint16_t x, uint16_t y)
{
    uint8_t row = 0;

    while (row < NB_ROWS) {
        if (y < ((row + 1) * KEY_HEIGHT)) {
            return row * NB_COLS + x / KEY_WIDTH;
        }
        row++;
    }
    return 0xFF;
}

static void touchCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType)
{
    uint8_t                    firstIndex, lastIndex;
    nbgl_touchStatePosition_t *firstPosition, *lastPosition;

    LOG_DEBUG(MISC_LOGGER, "touchCallback(): eventType = %d\n", eventType);
    if ((eventType != TOUCHING) && (eventType != TOUCH_RELEASED)) {
        return;
    }

    if (eventType == TOUCH_RELEASED) {
        LOG_WARN(MISC_LOGGER, "touchCallback(): mask = 0x%X\n", mask);
        buildScreen(mask);
        return;
    }
    if (nbgl_touchGetTouchedPosition(obj, &firstPosition, &lastPosition) == false) {
        return;
    }

    // use positions relative to keypad position
    firstIndex = getKeyIndex(firstPosition->x - obj->area.x0, firstPosition->y - obj->area.y0);
    if (firstIndex > ((NB_COLS * NB_ROWS) - 1)) {
        return;
    }
    lastIndex = getKeyIndex(lastPosition->x - obj->area.x0, lastPosition->y - obj->area.y0);
    if (lastIndex > ((NB_COLS * NB_ROWS) - 1)) {
        return;
    }

    // LOG_WARN(MISC_LOGGER,"touchCallback(): firstIndex = %d, lastIndex = %d\n",firstIndex,
    // lastIndex);
    mask &= ~(1 << lastIndex);
}

static void keypadDrawGrid(void)
{
    nbgl_area_t rectArea;
    uint8_t     i;

    // clean full area
    rectArea.backgroundColor = WHITE;
    rectArea.x0              = 0;
    rectArea.y0              = 0;
    rectArea.width           = SCREEN_WIDTH;
    rectArea.height          = SCREEN_HEIGHT;
    nbgl_frontDrawRect(&rectArea);

    /// draw horizontal lines
    rectArea.backgroundColor = WHITE;
    rectArea.x0              = 0;
    rectArea.y0              = 0;
    rectArea.width           = SCREEN_WIDTH;
    rectArea.height          = 4;
    for (i = 0; i < NB_ROWS - 1; i++) {
        rectArea.y0 += KEY_HEIGHT;
        nbgl_frontDrawHorizontalLine(&rectArea, 0x1, BLACK);
    }

    /// then draw vertical lines
    rectArea.backgroundColor = BLACK;
    rectArea.x0              = 0;
    rectArea.y0              = 0;
    rectArea.width           = 1;
    rectArea.height          = SCREEN_HEIGHT;
    for (i = 0; i < NB_COLS - 1; i++) {
        rectArea.x0 += KEY_WIDTH;
        nbgl_frontDrawRect(&rectArea);  // 1st full line, on the left
    }
}

static void buildScreen(uint32_t digitsMask)
{
    nbgl_container_t *container;
    nbgl_obj_t      **screenChildren;

    mask = digitsMask;
    nbgl_screenSet(&screenChildren, 1, NULL, (nbgl_touchCallback_t) touchCallback);

    container         = (nbgl_container_t *) nbgl_objPoolGet(CONTAINER, 0);
    screenChildren[0] = (nbgl_obj_t *) container;

    container->nbChildren = 0;
    container->children   = NULL;

    container->obj.area.width       = SCREEN_WIDTH;
    container->obj.area.height      = SCREEN_HEIGHT;
    container->layout               = HORIZONTAL;
    container->obj.alignmentMarginX = 0;
    container->obj.alignmentMarginY = 0;
    container->obj.alignment        = NO_ALIGNMENT;
    container->obj.alignTo          = NULL;
    container->obj.touchMask        = (1 << TOUCHING) | (1 << TOUCH_RELEASED);

    nbgl_screenRedraw();

    // At first, draw grid
    keypadDrawGrid();
}

static void onAction(void)
{
    // 24 bits for 24 digits
    buildScreen(0xFFFFFF);
}

/**
 * @brief Recovery Check application start page
 *
 */
void app_fullRecoveryCheck(void)
{
    nbgl_homeAction_t homeAction
        = {.callback = onAction, .icon = NULL, .text = "Start check", .style = STRONG_HOME_ACTION};
    nbgl_useCaseHomeAndSettings(
        "Recovery Check",
        &C_Recovery_Check_64px,
        "This app lets you enter a Secret Recovery Phrase and test if it matches "
        "the one present on this Ledger.",
        INIT_HOME_PAGE,
        NULL,
        &infoContentsList,
        &homeAction,
        NULL);
}
