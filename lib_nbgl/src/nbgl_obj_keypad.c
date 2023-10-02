
/**
 * @file nbgl_obj_keypad.c
 * @brief The construction and touch management of a keypad object
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_front.h"
#include "nbgl_draw.h"
#include "nbgl_obj.h"
#include "nbgl_fonts.h"
#include "nbgl_touch.h"
#include "glyphs.h"

/*********************
 *      DEFINES
 *********************/
#define KEY_WIDTH            (SCREEN_WIDTH/3)
#define DIGIT_OFFSET_Y       (((KEYPAD_KEY_HEIGHT-32)/2)%0xFFC)

#define BACKSPACE_KEY_INDEX  10
#define VALIDATE_KEY_INDEX   11

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
 *      MACROS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/
static uint8_t getKeypadIndex(nbgl_touchStatePosition_t *position) {
  uint8_t i=0;
  // get index of key pressed
  if (position->y < KEYPAD_KEY_HEIGHT) {
    // 1st line:
    i = position->x/KEY_WIDTH;
  }
  else if (position->y < (2*KEYPAD_KEY_HEIGHT)) {
    // 2nd line:
    i = 3 + position->x/KEY_WIDTH;
  }
  else if (position->y < (3*KEYPAD_KEY_HEIGHT)) {
    // 3rd line:
    i = 6 + position->x/KEY_WIDTH;
  }
  else if (position->y < (4*KEYPAD_KEY_HEIGHT)) {
    // 4th line
    if (position->x<KEY_WIDTH) {
      i = BACKSPACE_KEY_INDEX;
    }
    else if (position->x<(2*KEY_WIDTH)) {
      i = 0;
    }
    else {
      i = VALIDATE_KEY_INDEX;
    }
  }
  return i;
}

static void keypadTouchCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType) {
  uint8_t firstIndex, lastIndex;
  nbgl_touchStatePosition_t *firstPosition, *lastPosition;
  nbgl_keypad_t *keypad = (nbgl_keypad_t *)obj;

  LOG_DEBUG(MISC_LOGGER,"keypadTouchCallback(): eventType = %d\n",eventType);
  if (eventType != TOUCHED) {
    return;
  }
  if (nbgl_touchGetTouchedPosition(obj,&firstPosition,&lastPosition) == false) {
    return;
  }
  // modify positions with keypad position
  firstPosition->x -= obj->x0;
  firstPosition->y -= obj->y0;
  lastPosition->x -= obj->x0;
  lastPosition->y -= obj->y0;

  firstIndex = getKeypadIndex(firstPosition);
  if (firstIndex > VALIDATE_KEY_INDEX)
    return;
  lastIndex = getKeypadIndex(lastPosition);
  if (lastIndex > VALIDATE_KEY_INDEX)
    return;
  // if position of finger has moved durinng press to another "key", drop it
  if (lastIndex != firstIndex)
    return;

  if ((firstIndex<10)&&(keypad->enableDigits)) {
    keypad->callback(0x30+firstIndex+1);
  }
  if ((firstIndex == BACKSPACE_KEY_INDEX)&&(keypad->enableBackspace)) { // backspace
    keypad->callback(BACKSPACE_KEY);
  }
  else if ((firstIndex == VALIDATE_KEY_INDEX)&&(keypad->enableValidate)) { // validate
    keypad->callback(VALIDATE_KEY);
  }
}

static void keypadDrawGrid(nbgl_keypad_t *keypad) {
  nbgl_area_t rectArea;

  // clean full area
  rectArea.backgroundColor = keypad->backgroundColor;
  rectArea.x0 = keypad->x0;
  rectArea.y0 = keypad->y0;
  rectArea.width = keypad->width;
  rectArea.height = keypad->height;
  nbgl_frontDrawRect(&rectArea);

  /// draw horizontal lines
  rectArea.backgroundColor = keypad->backgroundColor;
  rectArea.x0 = keypad->x0;
  rectArea.y0 = keypad->y0;
  rectArea.width = keypad->width;
  rectArea.height = 4;
  nbgl_frontDrawHorizontalLine(&rectArea, 0x1, keypad->borderColor); // 1st line (top)
  rectArea.y0 += KEYPAD_KEY_HEIGHT;
  nbgl_frontDrawHorizontalLine(&rectArea, 0x1, keypad->borderColor); // 2nd line
  rectArea.y0 += KEYPAD_KEY_HEIGHT;
  nbgl_frontDrawHorizontalLine(&rectArea, 0x1, keypad->borderColor); // 3rd line
  rectArea.y0 += KEYPAD_KEY_HEIGHT;
  nbgl_frontDrawHorizontalLine(&rectArea, 0x1, keypad->borderColor); // 4th line

  /// then draw 3 vertical lines
  rectArea.backgroundColor = keypad->borderColor;
  rectArea.x0 = keypad->x0;
  rectArea.y0 = keypad->y0;
  rectArea.width = 1;
  rectArea.height = KEYPAD_KEY_HEIGHT*4;
  nbgl_frontDrawRect(&rectArea); // 1st full line, on the left
  rectArea.x0 += KEY_WIDTH;
  nbgl_frontDrawRect(&rectArea); // 2nd line
  rectArea.x0 += KEY_WIDTH;
  nbgl_frontDrawRect(&rectArea); // 3rd line
}


static void keypadDrawDigits(nbgl_keypad_t *keypad) {
  uint8_t i;
  nbgl_area_t rectArea;
  char key_value;

  rectArea.backgroundColor = keypad->backgroundColor;
  rectArea.y0 = keypad->y0 + DIGIT_OFFSET_Y;

  // First row of keys: 1 2 3
  for (i=0;i<3;i++) {
    key_value = i + 0x30 + 1;
    rectArea.x0 = keypad->x0 + i*KEY_WIDTH;
    rectArea.x0 += (KEY_WIDTH-nbgl_getCharWidth(BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,&key_value))/2;
    nbgl_drawText(&rectArea, &key_value,
                  1, BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,
                  keypad->enableDigits?BLACK:LIGHT_GRAY);
  }
  // Second row: 4 5 6
  rectArea.y0 += KEYPAD_KEY_HEIGHT;
  for (;i<6;i++) {
    key_value = i + 0x30 + 1;
    rectArea.x0 = keypad->x0 + (i-3)*KEY_WIDTH;
    rectArea.x0 += (KEY_WIDTH-nbgl_getCharWidth(BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,&key_value))/2;
    nbgl_drawText(&rectArea, &key_value,
                  1, BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,
                  keypad->enableDigits?BLACK:LIGHT_GRAY);
  }
  // Third row: 7 8 9
  rectArea.y0 += KEYPAD_KEY_HEIGHT;
  for (;i<9;i++) {
    key_value = i + 0x30 + 1;
    rectArea.x0 = keypad->x0 + (i-6)*KEY_WIDTH;
    rectArea.x0 += (KEY_WIDTH-nbgl_getCharWidth(BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,&key_value))/2;
    nbgl_drawText(&rectArea, &key_value,
                  1, BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,
                  keypad->enableDigits?BLACK:LIGHT_GRAY);
  }
  // 4th raw, Backspace, 0 and Validate
  // draw backspace
  rectArea.width = C_backspace32px.width;
  rectArea.height = C_backspace32px.height;
  rectArea.bpp = NBGL_BPP_1;
  rectArea.x0 = keypad->x0 + (KEY_WIDTH-rectArea.width)/2;
  rectArea.y0 = keypad->y0 + KEYPAD_KEY_HEIGHT*3 + (KEYPAD_KEY_HEIGHT-rectArea.height)/2;
  nbgl_frontDrawImage(&rectArea,(uint8_t*)C_backspace32px.bitmap,NO_TRANSFORMATION, keypad->enableBackspace?BLACK:WHITE);

  // draw 0
  key_value = 0x30;
  rectArea.x0 = keypad->x0 + KEY_WIDTH;
  rectArea.x0 += (KEY_WIDTH-nbgl_getCharWidth(BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,&key_value))/2;
  rectArea.y0 = keypad->y0 + KEYPAD_KEY_HEIGHT*3 + DIGIT_OFFSET_Y;
  nbgl_drawText(&rectArea, &key_value,
                1, BAGL_FONT_HM_ALPHA_MONO_MEDIUM_32px,
                keypad->enableDigits?BLACK:LIGHT_GRAY);

  // draw validate on gray with white background if not enabled
  if (!keypad->enableValidate) {
    rectArea.width = C_check32px.width;
    rectArea.height = C_check32px.height;
    rectArea.bpp = NBGL_BPP_1;
    rectArea.x0 = keypad->x0 + 2*KEY_WIDTH + (KEY_WIDTH-rectArea.width)/2;
    rectArea.y0 = keypad->y0 + KEYPAD_KEY_HEIGHT*3 + (KEYPAD_KEY_HEIGHT-rectArea.height)/2;
    rectArea.backgroundColor = WHITE;
    nbgl_frontDrawRect(&rectArea);
  }
  else {
    // if enabled, draw icon in white on a black background
    rectArea.backgroundColor = BLACK;
    rectArea.x0 = keypad->x0 + 2*KEY_WIDTH;
    rectArea.y0 = keypad->y0 + KEYPAD_KEY_HEIGHT*3;
    rectArea.width = KEY_WIDTH;
    rectArea.height = KEYPAD_KEY_HEIGHT;
    nbgl_frontDrawRect(&rectArea);
    rectArea.width = C_check32px.width;
    rectArea.height = C_check32px.height;
    rectArea.bpp = NBGL_BPP_1;
    rectArea.x0 = keypad->x0 + 2*KEY_WIDTH + (KEY_WIDTH-rectArea.width)/2;
    rectArea.y0 = keypad->y0 + KEYPAD_KEY_HEIGHT*3 + (KEYPAD_KEY_HEIGHT-rectArea.height)/2;
    nbgl_frontDrawImage(&rectArea,(uint8_t*)C_check32px.bitmap,NO_TRANSFORMATION, WHITE);
  }
}

static void keypadDraw(nbgl_keypad_t *keypad) {
  // At first, draw grid
  keypadDrawGrid(keypad);

  // then draw key content
  keypadDrawDigits(keypad);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief This function draws a keypad object
 *
 * @param kpd keypad object to draw
 * @return the keypad keypad object
 */
void nbgl_objDrawKeypad(nbgl_keypad_t *kpd) {
  kpd->touchMask = (1 << TOUCHED);
  kpd->touchCallback = (nbgl_touchCallback_t)&keypadTouchCallback;

  keypadDraw(kpd);
}
