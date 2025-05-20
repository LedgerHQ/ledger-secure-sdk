/**
 * @file nbgl_touch.c
 * Implementation of touchscreen management in new BAGL
 */

#include "app_config.h"

#ifdef HAVE_SE_TOUCH
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include "nbgl_obj.h"
#include "nbgl_debug.h"
#include "nbgl_touch.h"
#include "nbgl_screen.h"
#include "os_pic.h"

/*********************
 *      DEFINES
 *********************/
enum {
    UX_CTX = 0,
    APP_CTX,
    NB_CTXS
};

/**********************
 *      TYPEDEFS
 **********************/
typedef struct nbgl_touchCtx_s {
    nbgl_touchState_t         lastState;
    uint32_t                  lastPressedTime;
    uint32_t                  lastCurrentTime;
    nbgl_obj_t               *lastPressedObj;
    nbgl_touchStatePosition_t firstTouchedPosition;
    nbgl_touchStatePosition_t lastTouchedPosition;
} nbgl_touchCtx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static nbgl_touchCtx_t touchCtxs[NB_CTXS];

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * @brief This function applies the touch event of given type on the given graphic object
 *
 * @param obj object on which the event is applied
 * @param eventType type of touchscreen event
 */
static void applytouchStatePosition(nbgl_obj_t *obj, nbgl_touchType_t eventType)
{
    nbgl_screen_t *screen = (nbgl_screen_t *) nbgl_screenGetTop();
    if (!obj) {
        return;
    }
    LOG_DEBUG(TOUCH_LOGGER, "Apply event %d on object of type %d\n", eventType, obj->type);
    /* the first action is the one provided by the application */
    if ((obj->touchMask & (1 << eventType)) != 0) {
        // for some specific objects, call directly a specific callback
        switch (obj->type) {
#ifdef NBGL_KEYBOARD
            case KEYBOARD:
                nbgl_keyboardTouchCallback(obj, eventType);
                break;
#endif  // NBGL_KEYBOARD
#ifdef NBGL_KEYPAD
            case KEYPAD:
                nbgl_keypadTouchCallback(obj, eventType);
                break;
#endif  // NBGL_KEYPAD
            default:
                if (screen->touchCallback != NULL) {
                    ((nbgl_touchCallback_t) PIC(screen->touchCallback))((void *) obj, eventType);
                }
                break;
        }
    }
}

/**
 * @brief if the given obj contains the coordinates of the given event, parse
 * all its children with the same criterion.
 * If no children or none concerned, check whether this object can process the event or not
 *
 * @param obj
 * @param event
 * @return the concerned object or NULL if not found
 */
static nbgl_obj_t *getTouchedObject(nbgl_obj_t *obj, nbgl_touchStatePosition_t *event)
{
    if (obj == NULL) {
        return NULL;
    }
    /* check coordinates
       no need to go further if the touched point is not within the object
       And because the children are also within the object, no need to check them either */
    if ((event->x < obj->area.x0) || (event->x >= (obj->area.x0 + obj->area.width))
        || (event->y < obj->area.y0) || (event->y >= (obj->area.y0 + obj->area.height))) {
        return NULL;
    }
    if ((obj->type == SCREEN) || (obj->type == CONTAINER)) {
        nbgl_container_t *container = (nbgl_container_t *) obj;
        // parse the children, if any
        if (container->children != NULL) {
            int8_t i = container->nbChildren - 1;
            // parse children from the latest, because they are drawn from the first
            while (i >= 0) {
                nbgl_obj_t *current = container->children[i];
                if (current != NULL) {
                    current = getTouchedObject(current, event);
                    if (current != NULL) {
                        return current;
                    }
                }
                i--;
            }
        }
    }
    /* now see if the object is interested by touch events (any of them) */
    if (obj->touchMask != 0) {
        // LOG_DEBUG(TOUCH_LOGGER,"%d %d \n",clickableObjectTypes ,(1<<obj->type));
        return obj;
    }
    else {
        return NULL;
    }
}

/**
 * @brief Get the deepest container that matches with input position and swipe
 * @param obj Root object
 * @param pos Position we are looking the container at
 * @param detectedSwipe Swipe gesture we are looking for
 * @return Pointer to obj if object exists, NULL otherwise
 */
static nbgl_obj_t *getSwipableObjectAtPos(nbgl_obj_t                *obj,
                                          nbgl_touchStatePosition_t *pos,
                                          nbgl_touchType_t           detectedSwipe)
{
    if (obj == NULL) {
        return NULL;
    }

    // Check if pos position belongs to obj
    if ((pos->x < obj->area.x0) || (pos->x >= (obj->area.x0 + obj->area.width))
        || (pos->y < obj->area.y0) || (pos->y >= (obj->area.y0 + obj->area.height))) {
        return NULL;
    }

    if ((obj->type == SCREEN) || (obj->type == CONTAINER)) {
        nbgl_container_t *container = (nbgl_container_t *) obj;
        for (uint8_t i = 0; i < container->nbChildren; i++) {
            nbgl_obj_t *current = container->children[i];
            if (current != NULL) {
                nbgl_obj_t *child = getSwipableObjectAtPos(current, pos, detectedSwipe);
                if (child) {
                    return child;
                }
            }
        }
    }
    if (obj->touchMask & (1 << detectedSwipe)) {
        return obj;
    }
    return NULL;
}

/**
 * @brief Get the deepest container that matches with a swipe
 *
 * The returned container matches with:
 * - First position in the swipe move
 * - Last position in the swipe move
 * - Detected swipe gesture
 *
 * @param first First position in the swipe move
 * @param last Last position in the swipe move
 * @param detectedSwipe Detected swipe gesture
 * @return Pointer to obj if object exists, NULL otherwise
 */

static nbgl_obj_t *getSwipableObject(nbgl_obj_t                *obj,
                                     nbgl_touchStatePosition_t *first,
                                     nbgl_touchStatePosition_t *last,
                                     nbgl_touchType_t           detectedSwipe)
{
    if (obj == NULL) {
        return NULL;
    }

    nbgl_obj_t *first_obj = getSwipableObjectAtPos(obj, first, detectedSwipe);

    if (first_obj == NULL) {
        return NULL;
    }

    nbgl_obj_t *last_obj = getSwipableObjectAtPos(obj, last, detectedSwipe);

    // Swipable objects match
    if (first_obj == last_obj) {
        return first_obj;
    }

    return NULL;
}

// Swipe detection

#ifndef HAVE_HW_TOUCH_SWIPE
#define SWIPE_THRESHOLD_X 10
#define SWIPE_THRESHOLD_Y 20
#else
// Mapping between nbgl_hardwareSwipe_t and nbgl_touchEvent_t
const nbgl_touchType_t SWIPE_GESTURES[] = {[HARDWARE_SWIPE_UP]    = SWIPED_UP,
                                           [HARDWARE_SWIPE_DOWN]  = SWIPED_DOWN,
                                           [HARDWARE_SWIPE_LEFT]  = SWIPED_LEFT,
                                           [HARDWARE_SWIPE_RIGHT] = SWIPED_RIGHT};
#endif  // HAVE_HW_TOUCH_SWIPE

static nbgl_touchType_t nbgl_detectSwipe(nbgl_touchStatePosition_t *last,
                                         nbgl_touchStatePosition_t *first)
{
#ifdef HAVE_HW_TOUCH_SWIPE
    // Swipe is detected by hardware
    (void) first;

    if (last->swipe >= NO_HARDWARE_SWIPE) {
        return NB_TOUCH_TYPES;
    }

    return SWIPE_GESTURES[last->swipe];

#else
    // Swipe is detected by software
    nbgl_touchType_t detected_swipe = NB_TOUCH_TYPES;
    if ((last->x - first->x) >= SWIPE_THRESHOLD_X) {
        detected_swipe = SWIPED_RIGHT;
    }
    else if ((first->x - last->x) >= SWIPE_THRESHOLD_X) {
        detected_swipe = SWIPED_LEFT;
    }
    else if ((last->y - first->y) >= SWIPE_THRESHOLD_Y) {
        detected_swipe = SWIPED_DOWN;
    }
    else if ((first->y - last->y) >= SWIPE_THRESHOLD_Y) {
        detected_swipe = SWIPED_UP;
    }

    return detected_swipe;
#endif  // HAVE_HW_TOUCH_SWIPE
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
/**
 * @brief Function to initialize the touch context
 * @param fromUx if true, means to initialize the UX context, otherwise App one
 */
void nbgl_touchInit(bool fromUx)
{
    nbgl_touchCtx_t *ctx = fromUx ? &touchCtxs[UX_CTX] : &touchCtxs[APP_CTX];
    memset(ctx, 0, sizeof(nbgl_touchCtx_t));
}

/**
 * @brief Function to be called periodically to check touchscreen state
 * and coordinates
 * @param fromUx if true, means this is called from the UX, not the App
 * @param touchStatePosition state and position read from touch screen
 * @param currentTime current time in ms
 */
void nbgl_touchHandler(bool                       fromUx,
                       nbgl_touchStatePosition_t *touchStatePosition,
                       uint32_t                   currentTime)
{
    nbgl_obj_t      *foundObj;
    nbgl_touchCtx_t *ctx = fromUx ? &touchCtxs[UX_CTX] : &touchCtxs[APP_CTX];

    // save last received currentTime
    ctx->lastCurrentTime = currentTime;

    if (ctx->lastState == RELEASED) {
        // filter out not realistic cases (successive RELEASE events)
        if (RELEASED == touchStatePosition->state) {
            ctx->lastState = touchStatePosition->state;
            return;
        }
        // memorize first touched position
        memcpy(&ctx->firstTouchedPosition, touchStatePosition, sizeof(nbgl_touchStatePosition_t));
    }
    // LOG_DEBUG(TOUCH_LOGGER,"state = %s, x = %d, y=%d\n",(touchStatePosition->state ==
    // RELEASED)?"RELEASED":"PRESSED",touchStatePosition->x,touchStatePosition->y);

    // parse the whole screen to find proper object
    foundObj = getTouchedObject(nbgl_screenGetTop(), touchStatePosition);

    // LOG_DEBUG(TOUCH_LOGGER,"nbgl_touchHandler: found obj %p, type = %d\n",foundObj,
    // foundObj->type);
    if (foundObj == NULL) {
        LOG_DEBUG(TOUCH_LOGGER, "nbgl_touchHandler: no found obj\n");
        if ((touchStatePosition->state == PRESSED) && (ctx->lastState == PRESSED)
            && (ctx->lastPressedObj != NULL)) {
            // finger has moved out of an object
            // make sure lastPressedObj still belongs to current screen before warning it
            if (nbgl_screenContainsObj(ctx->lastPressedObj)) {
                applytouchStatePosition(ctx->lastPressedObj, OUT_OF_TOUCH);
            }
        }
        // Released event has been handled, forget lastPressedObj
        ctx->lastPressedObj = NULL;
    }

    // memorize last touched position
    memcpy(&ctx->lastTouchedPosition, touchStatePosition, sizeof(nbgl_touchStatePosition_t));

    if (touchStatePosition->state == RELEASED) {
        nbgl_touchType_t swipe = nbgl_detectSwipe(touchStatePosition, &ctx->firstTouchedPosition);
        bool             consumed = false;

        ctx->lastState = touchStatePosition->state;
        if (swipe != NB_TOUCH_TYPES) {
            // Swipe detected
            nbgl_obj_t *swipedObj = getSwipableObject(
                nbgl_screenGetTop(), &ctx->firstTouchedPosition, &ctx->lastTouchedPosition, swipe);
            // if a swipable object has been found
            if (swipedObj) {
                applytouchStatePosition(swipedObj, swipe);
                consumed = true;
            }
        }
        if (!consumed && (ctx->lastPressedObj != NULL)
            && ((foundObj == ctx->lastPressedObj)
                || (nbgl_screenContainsObj(ctx->lastPressedObj)))) {
            // very strange if lastPressedObj != foundObj, let's consider that it's a normal release
            // on lastPressedObj make sure lastPressedObj still belongs to current screen before
            // "releasing" it
            applytouchStatePosition(ctx->lastPressedObj, TOUCH_RELEASED);
            if (currentTime >= (ctx->lastPressedTime + LONG_TOUCH_DURATION)) {
                applytouchStatePosition(ctx->lastPressedObj, LONG_TOUCHED);
            }
            else if (currentTime >= (ctx->lastPressedTime + SHORT_TOUCH_DURATION)) {
                applytouchStatePosition(ctx->lastPressedObj, TOUCHED);
            }
        }
        // Released event has been handled, forget lastPressedObj
        ctx->lastPressedObj = NULL;
    }
    else {  // PRESSED
        if ((ctx->lastState == PRESSED) && (ctx->lastPressedObj != NULL)) {
            ctx->lastState = touchStatePosition->state;
            if (foundObj != ctx->lastPressedObj) {
                // finger has moved out of an object
                // make sure lastPressedObj still belongs to current screen before warning it
                if (nbgl_screenContainsObj(ctx->lastPressedObj)) {
                    applytouchStatePosition(ctx->lastPressedObj, OUT_OF_TOUCH);
                }
                ctx->lastPressedObj = NULL;
            }
            else {
                // warn the concerned object that it is still touched
                applytouchStatePosition(foundObj, TOUCHING);
            }
        }
        else if (ctx->lastState == RELEASED) {
            ctx->lastState = touchStatePosition->state;
            // newly touched object
            ctx->lastPressedObj  = foundObj;
            ctx->lastPressedTime = currentTime;
            applytouchStatePosition(foundObj, TOUCH_PRESSED);
            applytouchStatePosition(foundObj, TOUCHING);
        }
    }
}

bool nbgl_touchGetTouchedPosition(nbgl_obj_t                 *obj,
                                  nbgl_touchStatePosition_t **firstPos,
                                  nbgl_touchStatePosition_t **lastPos)
{
    nbgl_touchCtx_t *ctx = nbgl_objIsUx(obj) ? &touchCtxs[UX_CTX] : &touchCtxs[APP_CTX];
    LOG_DEBUG(TOUCH_LOGGER, "nbgl_touchGetTouchedPosition: %p %p\n", obj, ctx->lastPressedObj);
    if (obj == ctx->lastPressedObj) {
        *firstPos = &ctx->firstTouchedPosition;
        *lastPos  = &ctx->lastTouchedPosition;
        return true;
    }
    return false;
}

uint32_t nbgl_touchGetTouchDuration(nbgl_obj_t *obj)
{
    nbgl_touchCtx_t *ctx = nbgl_objIsUx(obj) ? &touchCtxs[UX_CTX] : &touchCtxs[APP_CTX];
    if (obj == ctx->lastPressedObj) {
        return (ctx->lastCurrentTime - ctx->lastPressedTime);
    }
    return 0;
}

/**
 * @brief parse all the children of the given object, recursively, until an object with the given
 * touch if is found.
 *
 * @param obj parent of the touched object
 * @param id id of the touched object to find
 * @return the concerned object or NULL if not found
 */
nbgl_obj_t *nbgl_touchGetObjectFromId(nbgl_obj_t *obj, uint8_t id)
{
    if (obj == NULL) {
        return NULL;
    }
    if ((obj->type == SCREEN) || (obj->type == CONTAINER)) {
        nbgl_container_t *container = (nbgl_container_t *) obj;
        // parse the children, if any
        if (container->children != NULL) {
            uint8_t i;
            for (i = 0; i < container->nbChildren; i++) {
                nbgl_obj_t *current = container->children[i];
                if (current != NULL) {
                    current = nbgl_touchGetObjectFromId(current, id);
                    if (current != NULL) {
                        return current;
                    }
                }
            }
        }
    }
    /* now see if the object is interested by touch events (any of them) */
    if ((obj->touchMask != 0) && (obj->touchId == id)) {
        LOG_DEBUG(TOUCH_LOGGER, "found %p: id = %d, type = %d \n", obj, obj->touchId, obj->type);
        return obj;
    }
    else {
        return NULL;
    }
}
#endif  // HAVE_SE_TOUCH
