
/**
 * @file nbgl_layout_navigation.c
 * @brief The construction of a navigation bar with buttons super-object
 *
 */

#ifdef HAVE_SE_TOUCH

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_draw.h"
#include "nbgl_obj.h"
#include "nbgl_layout_internal.h"
#include "os_print.h"
#include "os_helpers.h"
#include "glyphs.h"

/*********************
 *      DEFINES
 *********************/
#define INTERNAL_SMALL_MARGIN 8

#define BORDER_COLOR WHITE
#ifdef TARGET_STAX
#define NAVIGATION_HEIGHT 92
#define NAV_BUTTON_HEIGHT 80
#define NAV_BUTTON_WIDTH  80
#else  // TARGET_STAX
#define NAVIGATION_HEIGHT 96
#define NAV_BUTTON_HEIGHT NAVIGATION_HEIGHT
#define NAV_BUTTON_WIDTH  104
#endif  // TARGET_STAX

/**********************
 *      TYPEDEFS
 **********************/
enum {
    EXIT_BUTTON_INDEX = 0,
    PREVIOUS_PAGE_INDEX,
    NEXT_PAGE_INDEX,
    PAGE_INDICATOR_INDEX,
    NB_MAX_CHILDREN
};

/**********************
 *  STATIC VARIABLES
 **********************/
static char navText[11];  // worst case is "ccc of nnn"

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void configButtons(nbgl_container_t *navContainer, uint8_t navNbPages, uint8_t navActivePage)
{
    nbgl_button_t *buttonPrevious = (nbgl_button_t *) navContainer->children[PREVIOUS_PAGE_INDEX];
    nbgl_button_t *buttonNext     = (nbgl_button_t *) navContainer->children[NEXT_PAGE_INDEX];

    if (buttonPrevious) {
        buttonPrevious->foregroundColor = (navActivePage == 0) ? LIGHT_GRAY : BLACK;
    }
    if (navNbPages > 1) {
        buttonNext->foregroundColor = (navActivePage == (navNbPages - 1)) ? LIGHT_GRAY : BLACK;
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief function to be called when any of the controls in navigation bar is touched
 *
 * @param obj touched object (button or container)
 * @param eventType type of touch (only TOUCHED or SWIPED is accepted)
 * @param nbPages number of pages for navigation (if < 2, no navigation keys)
 * @param activePage current active page
 * @return true if actually navigated (page change)
 */
bool layoutNavigationCallback(nbgl_obj_t      *obj,
                              nbgl_touchType_t eventType,
                              uint8_t          nbPages,
                              uint8_t         *activePage)
{
    // if direct touch of buttons within the navigation bar, the given obj is
    // the touched object
    if (eventType == TOUCHED) {
        nbgl_container_t *navContainer = (nbgl_container_t *) obj->parent;

        if (obj == navContainer->children[EXIT_BUTTON_INDEX]) {
            // fake page when Quit button is touched
            *activePage = EXIT_PAGE;
            return true;
        }
        else if (obj == navContainer->children[PREVIOUS_PAGE_INDEX]) {
            if (*activePage > 0) {
                *activePage = *activePage - 1;
                configButtons(navContainer, nbPages, *activePage);
                return true;
            }
        }
        else if (obj == navContainer->children[NEXT_PAGE_INDEX]) {
            if ((nbPages < 2) || (*activePage < (nbPages - 1))) {
                *activePage = *activePage + 1;
                configButtons(navContainer, nbPages, *activePage);
                return true;
            }
        }
    }
    // otherwise the given object is the navigation container itself
    else if (eventType == SWIPED_RIGHT) {
        if (*activePage > 0) {
            *activePage = *activePage - 1;
            configButtons((nbgl_container_t *) obj, nbPages, *activePage);
            return true;
        }
    }
    else if (eventType == SWIPED_LEFT) {
        if ((nbPages < 2) || (*activePage < (nbPages - 1))) {
            *activePage = *activePage + 1;
            configButtons((nbgl_container_t *) obj, nbPages, *activePage);
            return true;
        }
    }
    return false;
}

/**
 * @brief This function creates a full navigation bar "object", with buttons and returns it as a
 * container
 *
 * @param navContainer container used for the objects of the navigation
 * @param navConfig configuration to create the navigation bar, at the bottom of the screen
 * @param layer layer (screen) to create the navigation bar in
 *
 */
void layoutNavigationPopulate(nbgl_container_t                 *navContainer,
                              const nbgl_layoutNavigationBar_t *navConfig,
                              uint8_t                           layer)
{
    nbgl_button_t *button;

    if (navConfig->withExitKey) {
        button                  = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layer);
        button->innerColor      = WHITE;
        button->borderColor     = BORDER_COLOR;
        button->obj.area.width  = BUTTON_DIAMETER;
        button->obj.area.height = BUTTON_DIAMETER;
        button->radius          = BUTTON_RADIUS;
        button->icon            = &CLOSE_ICON;
#ifdef TARGET_FLEX
        button->obj.alignmentMarginX = (navConfig->nbPages > 1) ? 8 : 0;
#endif  // TARGET_FLEX

        button->obj.alignment                     = (navConfig->nbPages > 1) ? MID_LEFT : CENTER;
        button->obj.touchMask                     = (1 << TOUCHED);
        button->obj.touchId                       = BOTTOM_BUTTON_ID;
        navContainer->children[EXIT_BUTTON_INDEX] = (nbgl_obj_t *) button;
    }
    // create previous page button (back)
    if (navConfig->withBackKey) {
        button                  = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layer);
        button->innerColor      = WHITE;
        button->borderColor     = BORDER_COLOR;
        button->obj.area.width  = NAV_BUTTON_WIDTH;
        button->obj.area.height = NAV_BUTTON_HEIGHT;
        button->radius          = BUTTON_RADIUS;
        button->icon            = &CHEVRON_BACK_ICON;
        // align on the right of the container, leaving space for "Next" button
        button->obj.alignment                       = MID_RIGHT;
        button->obj.alignmentMarginX                = NAV_BUTTON_WIDTH;
        button->obj.touchMask                       = (1 << TOUCHED);
        button->obj.touchId                         = LEFT_BUTTON_ID;
        navContainer->children[PREVIOUS_PAGE_INDEX] = (nbgl_obj_t *) button;
    }

    // create next page button
    button                                  = (nbgl_button_t *) nbgl_objPoolGet(BUTTON, layer);
    button->innerColor                      = WHITE;
    button->borderColor                     = BORDER_COLOR;
    button->foregroundColor                 = BLACK;
    button->obj.area.width                  = NAV_BUTTON_WIDTH;
    button->obj.area.height                 = NAV_BUTTON_HEIGHT;
    button->radius                          = BUTTON_RADIUS;
    button->icon                            = &CHEVRON_NEXT_ICON;
    button->obj.alignment                   = MID_RIGHT;
    button->obj.touchMask                   = (1 << TOUCHED);
    button->obj.touchId                     = RIGHT_BUTTON_ID;
    navContainer->children[NEXT_PAGE_INDEX] = (nbgl_obj_t *) button;

    // potentially create page indicator (with a text area, and "page of nb_page")
    if (navConfig->withPageIndicator) {
        if (navConfig->visibleIndicator) {
            nbgl_text_area_t *textArea = (nbgl_text_area_t *) nbgl_objPoolGet(TEXT_AREA, layer);

            SPRINTF(navText, "%d of %d", navConfig->activePage + 1, navConfig->nbPages);

            textArea->textColor = DARK_GRAY;
#ifdef TARGET_STAX
            textArea->obj.area.width = 109;
#else   // TARGET_STAX
            textArea->obj.area.width = 89;
#endif  // TARGET_STAX
            textArea->text                               = navText;
            textArea->fontId                             = SMALL_REGULAR_FONT;
            textArea->obj.area.height                    = NAV_BUTTON_HEIGHT;
            textArea->textAlignment                      = CENTER;
            textArea->obj.alignment                      = CENTER;
            navContainer->children[PAGE_INDICATOR_INDEX] = (nbgl_obj_t *) textArea;
        }
        if (navConfig->withBackKey) {
            navContainer->children[PREVIOUS_PAGE_INDEX]->alignmentMarginX += 79;
        }
    }

    // configure enabling/disabling of button
    configButtons(navContainer, navConfig->nbPages, navConfig->activePage);

    return;
}

#endif  // HAVE_SE_TOUCH
