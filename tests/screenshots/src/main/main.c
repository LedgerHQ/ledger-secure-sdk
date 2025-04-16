
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL.h>
#include "properties.h"
#include "apps_api.h"
#include "glyphs.h"
#include "os_settings.h"
#include "os_helpers.h"
#include "json_scenario.h"
#include "app_icons.h"
#include "nbgl_debug.h"
#include "nbgl_driver.h"
#include "nbgl_buttons.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      VARIABLES
 **********************/

extern nbgl_touchStatePosition_t gTouchStatePosition;
extern uint32_t                  G_interval_ms;

char *dirName;
char *propertiesName;

unsigned long gLogger = 0
    //  | (1<<LOW_LOGGER)
    //  | (1<<DRAW_LOGGER)
    //  | (1<<OBJ_LOGGER)
    //  | (1<<OBJ_POOL_LOGGER)
    //  | (1<<SCREEN_LOGGER)
    //  | (1<<LAYOUT_LOGGER)
    //  | (1<<PAGE_LOGGER)
    //  | (1<<TOUCH_LOGGER)
    //  | (1<<APP_LOGGER)
    //  | (1<<UX_LOGGER)
    //  | (1<<MISC_LOGGER)
    //  | (1<<STEP_LOGGER)
    //  | (1<<FLOW_LOGGER)
    ;

/**********************
 *  STATIC PROTOTYPES
 **********************/

void mainExit(int exitCode)
{
    printf("Fatal error in scenario page %s when creating pages in %s\n",
           scenario_get_current_page(),
           dirName);

    exit(exitCode);
}

/**
 * @brief Entry point of the simulator
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char **argv)
{
    uint32_t currentTime = 1000;
    int      running     = 1;

    srand(time(NULL));
    // parse args
    scenario_parse_args(argc, argv);

    // open .properties file to get info
    properties_init(propertiesName);

    // analyze scenarios in JSON files
    int res = scenario_parse_json();
    if (res) {
        printf("Wrong Json scenario\n");
        return 1;
    }

    nbgl_objInit();

    while (running) {
#ifdef HAVE_SE_TOUCH
        static nbgl_touchState_t previousState = RELEASED;
        // simulate touch during ticker event if pressed (for long press button)
        if (gTouchStatePosition.state == PRESSED) {
            nbgl_touchHandler(false, &gTouchStatePosition, currentTime);
        }
#else   // HAVE_SE_TOUCH
        static uint8_t previousKeyState = 0;
        uint8_t        keyState;
#endif  // HAVE_SE_TOUCH

#ifdef HAVE_SE_TOUCH
        res = scenario_get_position(&gTouchStatePosition, previousState);
#else                    // HAVE_SE_TOUCH
        res = scenario_get_keys(&keyState, previousKeyState);
#endif                   // HAVE_SE_TOUCH
        if (res == 1) {  // normal end
            scenario_save_json();
            break;
        }
        else if (res == -1) {  // error
            fprintf(stderr, "Error in scenario\n");
            return -1;
        }

#ifdef HAVE_SE_TOUCH
        if (gTouchStatePosition.x >= (FULL_SCREEN_WIDTH - SCREEN_WIDTH)) {
            gTouchStatePosition.x -= FULL_SCREEN_WIDTH - SCREEN_WIDTH;
        }
        else {
            gTouchStatePosition.state = RELEASED;
        }

        if (gTouchStatePosition.state != previousState) {
            nbgl_touchHandler(false, &gTouchStatePosition, currentTime);
            nbgl_refresh();
        }
        previousState = gTouchStatePosition.state;
#else   // HAVE_SE_TOUCH
        if ((previousKeyState != keyState) || (keyState != 0)) {
            nbgl_buttonsHandler(keyState, currentTime);
            nbgl_refresh();
        }
        previousKeyState = keyState;
#endif  // HAVE_SE_TOUCH

        currentTime += G_interval_ms;
    }

    return 0;
}
