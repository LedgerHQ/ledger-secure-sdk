/**
 * @file nbgl_step.h
 * @brief Step construction API of NBGL
 *
 */

#ifndef NBGL_STEP_H
#define NBGL_STEP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "nbgl_layout.h"
#include "nbgl_obj.h"
#include "nbgl_types.h"
#ifdef HAVE_LANGUAGE_PACK
#include "bolos_ux_loc_strings.h"
#endif  // HAVE_LANGUAGE_PACK

/*********************
 *      DEFINES
 *********************/
/**
 * get the "position" of a step within a flow of several steps
 * @param _step step index from which to get the position
 * @param _nb_steps number of steps in the flow
 */
#define GET_POS_OF_STEP(_step, _nb_steps) \
    (_nb_steps < 2)                       \
        ? SINGLE_STEP                     \
        : ((_step == 0) ? FIRST_STEP      \
                        : ((_step == (_nb_steps - 1)) ? LAST_STEP : NEITHER_FIRST_NOR_LAST_STEP))

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief type shared externally
 *
 */
typedef void *nbgl_step_t;

/**
 * @brief prototype of chosen menu list item callback
 * @param choiceIndex index of the menu list item
 */
typedef void (*nbgl_stepMenuListCallback_t)(uint8_t choiceIndex);

/**
 * @brief prototype of function to be called when buttons are touched on a screen
 * @param stepCtx context returned by the nbgl_stepDrawXXX function
 * @param event type of button event
 */
typedef void (*nbgl_stepButtonCallback_t)(nbgl_step_t stepCtx, nbgl_buttonEvent_t event);

/**
 * @brief possible position for a step in a flow
 *
 */
enum {
    SINGLE_STEP,                  ///< single step flow
    FIRST_STEP,                   ///< first in a multiple steps flow
    LAST_STEP,                    ///< last in a multiple steps flow
    NEITHER_FIRST_NOR_LAST_STEP,  ///< neither first nor last in a multiple steps flow
};

///< When the flow is navigated from first to last step
#define FORWARD_DIRECTION      0x00
///< When the flow is navigated from last to first step
#define BACKWARD_DIRECTION     0x08
///< When action callback applies on any button press
#define ACTION_ON_ANY_BUTTON   0x40
///< When action callback applies only on both button press
#define ACTION_ON_BOTH_BUTTONS 0x00
///< mask for the position of the step in nbgl_stepPosition_t
#define STEP_POSITION_MASK     0x03

/**
 * @brief this type is a bitfield containing:
 * - bit[3]: if 1, backward direction, if 0 forward
 * - bit[2]: if 1, action on any button, if 0 action only on double button
 * - bit[1:0]: possible positions for a step in a flow
 *
 */
typedef uint8_t nbgl_stepPosition_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

nbgl_step_t nbgl_stepDrawText(nbgl_stepPosition_t               pos,
                              nbgl_stepButtonCallback_t         onActionCallback,
                              nbgl_screenTickerConfiguration_t *ticker,
                              const char                       *text,
                              const char                       *subText,
                              nbgl_contentCenteredInfoStyle_t   style,
                              bool                              modal);
nbgl_step_t nbgl_stepDrawCenteredInfo(nbgl_stepPosition_t               pos,
                                      nbgl_stepButtonCallback_t         onActionCallback,
                                      nbgl_screenTickerConfiguration_t *ticker,
                                      nbgl_layoutCenteredInfo_t        *info,
                                      bool                              modal);
nbgl_step_t nbgl_stepDrawMenuList(nbgl_stepMenuListCallback_t       onActionCallback,
                                  nbgl_screenTickerConfiguration_t *ticker,
                                  nbgl_layoutMenuList_t            *list,
                                  bool                              modal);
uint8_t     nbgl_stepGetMenuListCurrent(nbgl_step_t step);
nbgl_step_t nbgl_stepDrawSwitch(nbgl_stepPosition_t               pos,
                                nbgl_stepButtonCallback_t         onActionCallback,
                                nbgl_screenTickerConfiguration_t *ticker,
                                nbgl_layoutSwitch_t              *switchInfo,
                                bool                              modal);
int         nbgl_stepRelease(nbgl_step_t step);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_STEP_H */
