/* NBGL runtime mocks.
 * Provides lightweight object, screen, font, input, and draw stubs.
 */

#include "nbgl_types.h"
#include "nbgl_obj.h"
#include "nbgl_screen.h"
#include "nbgl_fonts.h"
#include "nbgl_touch.h"
#include "nbgl_buttons.h"
#include "nbgl_draw.h"

#include <stddef.h>
#include <string.h>

/* refresh */

void nbgl_refresh(void) {}

void nbgl_refreshSpecial(nbgl_refresh_mode_t mode)
{
    (void) mode;
}

void nbgl_refreshSpecialWithPostRefresh(nbgl_refresh_mode_t mode, nbgl_post_refresh_t post_refresh)
{
    (void) mode;
    (void) post_refresh;
}

bool nbgl_refreshIsNeeded(void)
{
    return false;
}

void nbgl_refreshReset(void) {}

/* obj */

void nbgl_objInit(void) {}

void nbgl_objDraw(nbgl_obj_t *obj)
{
    (void) obj;
}

void nbgl_objAllowDrawing(bool enable)
{
    (void) enable;
}

static uint8_t ram_buffer[512];

uint8_t *nbgl_objGetRAMBuffer(void)
{
    return ram_buffer;
}

/* obj pool */

typedef union {
    nbgl_obj_t            base;
    nbgl_container_t      container;
    nbgl_line_t           line;
    nbgl_image_t          image;
    nbgl_image_file_t     image_file;
    nbgl_qrcode_t         qrcode;
    nbgl_radio_t          radio;
    nbgl_switch_t         sw;
    nbgl_progress_bar_t   progress;
    nbgl_page_indicator_t page_ind;
    nbgl_button_t         button;
    nbgl_text_area_t      text_area;
    nbgl_text_entry_t     text_entry;
    nbgl_mask_control_t   mask;
    nbgl_spinner_t        spinner;
    nbgl_keyboard_t       keyboard;
    nbgl_keypad_t         keypad;
} nbgl_any_obj_t;

#define OBJ_POOL_SIZE 512
static nbgl_any_obj_t obj_pool[OBJ_POOL_SIZE];
static uint16_t       obj_pool_idx;

#define CONTAINER_PTR_POOL_SIZE 2048
static nbgl_obj_t *container_ptr_pool[CONTAINER_PTR_POOL_SIZE];
static uint16_t    container_ptr_pool_idx;

nbgl_obj_t *nbgl_objPoolGet(nbgl_obj_type_t type, uint8_t layer)
{
    (void) layer;
    if (obj_pool_idx >= OBJ_POOL_SIZE) {
        obj_pool_idx = 0;
    }
    nbgl_any_obj_t *slot = &obj_pool[obj_pool_idx++];
    memset(slot, 0, sizeof(*slot));
    slot->base.type = type;
    return &slot->base;
}

uint8_t nbgl_objPoolGetId(nbgl_obj_t *obj)
{
    (void) obj;
    return 0;
}

int nbgl_objPoolGetArray(nbgl_obj_type_t type, uint8_t nbObjs, uint8_t layer, nbgl_obj_t **objArray)
{
    for (uint8_t i = 0; i < nbObjs; i++) {
        objArray[i] = nbgl_objPoolGet(type, layer);
    }
    return 0;
}

uint8_t nbgl_objPoolGetNbUsed(uint8_t layer)
{
    (void) layer;
    return (uint8_t) obj_pool_idx;
}

void nbgl_objPoolRelease(uint8_t layer)
{
    (void) layer;
    obj_pool_idx = 0;
}

nbgl_obj_t **nbgl_containerPoolGet(uint8_t nbObjs, uint8_t layer)
{
    (void) layer;
    if (container_ptr_pool_idx + nbObjs > CONTAINER_PTR_POOL_SIZE) {
        container_ptr_pool_idx = 0;
    }
    nbgl_obj_t **slot = &container_ptr_pool[container_ptr_pool_idx];
    memset(slot, 0, nbObjs * sizeof(nbgl_obj_t *));
    container_ptr_pool_idx += nbObjs;
    return slot;
}

uint8_t nbgl_containerPoolGetNbUsed(uint8_t layer)
{
    (void) layer;
    return (uint8_t) container_ptr_pool_idx;
}

void nbgl_containerPoolRelease(uint8_t layer)
{
    (void) layer;
    container_ptr_pool_idx = 0;
}

/* screen */

#define SCREEN_STACK_SIZE   4
#define MAX_SCREEN_CHILDREN 8
static nbgl_obj_t *screen_elements[SCREEN_STACK_SIZE][MAX_SCREEN_CHILDREN];
static uint8_t     screen_stack_idx;

nbgl_obj_t *nbgl_screenContainsObjType(nbgl_screen_t *screen, nbgl_obj_type_t type)
{
    (void) screen;
    (void) type;
    return NULL;
}

#ifdef HAVE_SE_TOUCH
int nbgl_screenSet(nbgl_obj_t                           ***elements,
                   uint8_t                                 nbElements,
                   const nbgl_screenTickerConfiguration_t *ticker,
                   nbgl_touchCallback_t                    touchCallback)
{
    (void) nbElements;
    (void) ticker;
    (void) touchCallback;
    memset(screen_elements[0], 0, sizeof(screen_elements[0]));
    *elements = screen_elements[0];
    return 0;
}

int nbgl_screenPush(nbgl_obj_t                           ***elements,
                    uint8_t                                 nbElements,
                    const nbgl_screenTickerConfiguration_t *ticker,
                    nbgl_touchCallback_t                    touchCallback)
{
    (void) nbElements;
    (void) ticker;
    (void) touchCallback;
    screen_stack_idx++;
    if (screen_stack_idx >= SCREEN_STACK_SIZE) {
        screen_stack_idx = 1;
    }
    memset(screen_elements[screen_stack_idx], 0, sizeof(screen_elements[screen_stack_idx]));
    *elements = screen_elements[screen_stack_idx];
    return screen_stack_idx;
}
#else
int nbgl_screenSet(nbgl_obj_t                           ***elements,
                   uint8_t                                 nbElements,
                   const nbgl_screenTickerConfiguration_t *ticker,
                   nbgl_buttonCallback_t                   buttonCallback)
{
    (void) nbElements;
    (void) ticker;
    (void) buttonCallback;
    memset(screen_elements[0], 0, sizeof(screen_elements[0]));
    *elements = screen_elements[0];
    return 0;
}

int nbgl_screenPush(nbgl_obj_t                           ***elements,
                    uint8_t                                 nbElements,
                    const nbgl_screenTickerConfiguration_t *ticker,
                    nbgl_buttonCallback_t                   buttonCallback)
{
    (void) nbElements;
    (void) ticker;
    (void) buttonCallback;
    screen_stack_idx++;
    if (screen_stack_idx >= SCREEN_STACK_SIZE) {
        screen_stack_idx = 1;
    }
    memset(screen_elements[screen_stack_idx], 0, sizeof(screen_elements[screen_stack_idx]));
    *elements = screen_elements[screen_stack_idx];
    return screen_stack_idx;
}
#endif

void nbgl_screenRedraw(void) {}

int nbgl_screenPop(uint8_t screenIndex)
{
    (void) screenIndex;
    if (screen_stack_idx > 0) {
        screen_stack_idx--;
    }
    return 0;
}

nbgl_obj_t **nbgl_screenGetElements(uint8_t screenIndex)
{
    if (screenIndex < SCREEN_STACK_SIZE) {
        return screen_elements[screenIndex];
    }
    return screen_elements[0];
}

uint8_t nbgl_screenGetCurrentStackSize(void)
{
    return screen_stack_idx;
}

uint8_t nbgl_screenGetUxStackSize(void)
{
    return 0;
}

nbgl_obj_t *nbgl_screenGetAt(uint8_t screenIndex)
{
    (void) screenIndex;
    return NULL;
}

nbgl_obj_t *nbgl_screenGetTop(void)
{
    return NULL;
}

int nbgl_screenUpdateTicker(uint8_t screenIndex, const nbgl_screenTickerConfiguration_t *ticker)
{
    (void) screenIndex;
    (void) ticker;
    return 0;
}

int nbgl_screenUpdateBackgroundColor(uint8_t screenIndex, color_t color)
{
    (void) screenIndex;
    (void) color;
    return 0;
}

int nbgl_screenReset(void)
{
    screen_stack_idx = 0;
    return 0;
}

void nbgl_screenHandler(uint32_t intervaleMs)
{
    (void) intervaleMs;
}

/* fonts */

static const nbgl_font_t mock_font = {
    .bitmap_len   = 0,
    .font_id      = 0,
    .bpp          = 1,
    .height       = 12,
    .line_height  = 14,
    .char_kerning = 0,
    .crop         = 0,
    .y_min        = 0,
    .first_char   = 0x20,
    .last_char    = 0x7E,
    .characters   = NULL,
    .bitmap       = NULL,
};

uint8_t nbgl_getCharWidth(nbgl_font_id_e fontId, const char *text)
{
    (void) fontId;
    (void) text;
    return 8;
}

const nbgl_font_t *nbgl_getFont(nbgl_font_id_e fontId)
{
    (void) fontId;
    return &mock_font;
}

uint8_t nbgl_getFontHeight(nbgl_font_id_e fontId)
{
    (void) fontId;
    return 12;
}

uint8_t nbgl_getFontLineHeight(nbgl_font_id_e fontId)
{
    (void) fontId;
    return 14;
}

uint16_t nbgl_getSingleLineTextWidth(nbgl_font_id_e fontId, const char *text)
{
    (void) fontId;
    if (text == NULL) {
        return 0;
    }
    return (uint16_t) (strlen(text) * 8);
}

uint16_t nbgl_getSingleLineTextWidthInLen(nbgl_font_id_e fontId, const char *text, uint16_t maxLen)
{
    (void) fontId;
    if (text == NULL) {
        return 0;
    }
    uint16_t len = (uint16_t) strlen(text);
    if (len > maxLen) {
        len = maxLen;
    }
    return len * 8;
}

uint16_t nbgl_getTextHeight(nbgl_font_id_e fontId, const char *text)
{
    (void) fontId;
    (void) text;
    return 14;
}

uint16_t nbgl_getTextHeightInWidth(nbgl_font_id_e fontId,
                                   const char    *text,
                                   uint16_t       maxWidth,
                                   bool           wrapping)
{
    (void) fontId;
    (void) text;
    (void) maxWidth;
    (void) wrapping;
    return 14;
}

void nbgl_getTextMaxLenAndWidth(nbgl_font_id_e fontId,
                                const char    *text,
                                uint16_t       maxWidth,
                                uint16_t      *len,
                                uint16_t      *width,
                                bool           wrapping)
{
    (void) fontId;
    (void) text;
    (void) maxWidth;
    (void) wrapping;
    if (len) {
        *len = (text != NULL) ? (uint16_t) strlen(text) : 0;
    }
    if (width) {
        *width = (text != NULL) ? (uint16_t) (strlen(text) * 8) : 0;
    }
}

uint16_t nbgl_getTextNbLinesInWidth(nbgl_font_id_e fontId,
                                    const char    *text,
                                    uint16_t       maxWidth,
                                    bool           wrapping)
{
    (void) fontId;
    (void) text;
    (void) maxWidth;
    (void) wrapping;
    return 1;
}

uint8_t nbgl_getTextNbPagesInWidth(nbgl_font_id_e fontId,
                                   const char    *text,
                                   uint8_t        nbLinesPerPage,
                                   uint16_t       maxWidth)
{
    (void) fontId;
    (void) text;
    (void) nbLinesPerPage;
    (void) maxWidth;
    return 1;
}

uint16_t nbgl_getTextWidth(nbgl_font_id_e fontId, const char *text)
{
    (void) fontId;
    if (text == NULL) {
        return 0;
    }
    return (uint16_t) (strlen(text) * 8);
}

bool nbgl_getTextMaxLenInNbLines(nbgl_font_id_e fontId,
                                 const char    *text,
                                 uint16_t       maxWidth,
                                 uint16_t       maxNbLines,
                                 uint16_t      *len,
                                 bool           wrapping)
{
    (void) fontId;
    (void) maxWidth;
    (void) maxNbLines;
    (void) wrapping;
    if (len) {
        *len = (text != NULL) ? (uint16_t) strlen(text) : 0;
    }
    return false;
}

void nbgl_textReduceOnNbLines(nbgl_font_id_e fontId,
                              const char    *origText,
                              uint16_t       maxWidth,
                              uint8_t        nbLines,
                              char          *reducedText,
                              uint16_t       reducedTextLen)
{
    (void) fontId;
    (void) maxWidth;
    (void) nbLines;
    if (reducedText && reducedTextLen > 0 && origText) {
        strncpy(reducedText, origText, reducedTextLen - 1);
        reducedText[reducedTextLen - 1] = '\0';
    }
}

void nbgl_textWrapOnNbLines(nbgl_font_id_e fontId, char *text, uint16_t maxWidth, uint8_t nbLines)
{
    (void) fontId;
    (void) text;
    (void) maxWidth;
    (void) nbLines;
}

void nbgl_refreshUnicodeFont(const LANGUAGE_PACK *lp)
{
    (void) lp;
}

/* touch */

void nbgl_touchHandler(bool fromUx, nbgl_touchStatePosition_t *touchEvent, uint32_t currentTimeMs)
{
    (void) fromUx;
    (void) touchEvent;
    (void) currentTimeMs;
}

uint32_t nbgl_touchGetTouchDuration(nbgl_obj_t *obj)
{
    (void) obj;
    return 0;
}

bool nbgl_touchGetTouchedPosition(nbgl_obj_t                 *obj,
                                  nbgl_touchStatePosition_t **firstPos,
                                  nbgl_touchStatePosition_t **lastPos)
{
    (void) obj;
    (void) firstPos;
    (void) lastPos;
    return false;
}

/* buttons */

void nbgl_buttonsHandler(uint8_t buttonState, uint32_t currentTimeMs)
{
    (void) buttonState;
    (void) currentTimeMs;
}

void nbgl_buttonsReset(void) {}

/* keyboard / keypad */

#ifndef HAVE_SE_TOUCH
void nbgl_keyboardCallback(nbgl_obj_t *obj, nbgl_buttonEvent_t buttonEvent)
{
    (void) obj;
    (void) buttonEvent;
}

void nbgl_keypadCallback(nbgl_obj_t *obj, nbgl_buttonEvent_t buttonEvent)
{
    (void) obj;
    (void) buttonEvent;
}
#endif

/* draw */

nbgl_font_id_e nbgl_drawText(const nbgl_area_t *area,
                             const char        *text,
                             uint16_t           textLen,
                             nbgl_font_id_e     fontId,
                             color_t            fontColor)
{
    (void) area;
    (void) text;
    (void) textLen;
    (void) fontColor;
    return fontId;
}
