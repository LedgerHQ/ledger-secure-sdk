
/**
 * @file nbgl_obj_keyboard.c
 * @brief The construction and touch management of a keyboard object
 *
 */

#ifdef NBGL_KEYBOARD
#ifdef HAVE_SE_TOUCH

/*********************
 *      INCLUDES
 *********************/
#include "nbgl_debug.h"
#include "nbgl_front.h"
#include "nbgl_draw.h"
#include "nbgl_draw_text.h"
#include "nbgl_obj.h"
#include "nbgl_fonts.h"
#include "nbgl_touch.h"
#include "glyphs.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"

/*********************
 *      DEFINES
 *********************/

#define FIRST_LINE_CHAR_COUNT  10
#define SECOND_LINE_CHAR_COUNT 9

#if (SCREEN_WIDTH == 400)
#define NORMAL_KEY_WIDTH 40
#define LETTER_OFFSET_Y  ((KEYBOARD_KEY_HEIGHT - 32) / 2)
#elif (SCREEN_WIDTH == 480)
#define NORMAL_KEY_WIDTH 48
#define LETTER_OFFSET_Y  ((KEYBOARD_KEY_HEIGHT - 36) / 2)
#elif (SCREEN_WIDTH == 300)
#define NORMAL_KEY_WIDTH 30
#define LETTER_OFFSET_Y  ((KEYBOARD_KEY_HEIGHT - 22) / 2)
#endif
#define SHIFT_KEY_WIDTH    (NORMAL_KEY_WIDTH + SECOND_LINE_OFFSET)
#define SECOND_LINE_OFFSET ((SCREEN_WIDTH - (SECOND_LINE_CHAR_COUNT * NORMAL_KEY_WIDTH)) / 2)
#if defined(TARGET_STAX)
#define SPACE_KEY_WIDTH 276
#elif defined(TARGET_FLEX)
#define SPACE_KEY_WIDTH 336
#elif defined(TARGET_APEX)
#define SPACE_KEY_WIDTH 209
#endif  // TARGETS
#define SWITCH_KEY_WIDTH                 (SCREEN_WIDTH - SPACE_KEY_WIDTH)
#define SPECIAL_CHARS_KEY_WIDTH          (NORMAL_KEY_WIDTH * 2 + SECOND_LINE_OFFSET)
#define BACKSPACE_KEY_WIDTH_FULL         SHIFT_KEY_WIDTH
#define BACKSPACE_KEY_WIDTH_DIGITS       SPECIAL_CHARS_KEY_WIDTH
#define BACKSPACE_KEY_WIDTH_LETTERS_ONLY (SCREEN_WIDTH - 7 * NORMAL_KEY_WIDTH)

#define IS_KEY_MASKED(_index) (keyboard->keyMask & (1 << _index))

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char kbd_chars[]       = "qwertyuiopasdfghjklzxcvbnm";
static const char kbd_chars_upper[] = "QWERTYUIOPASDFGHJKLZXCVBNM";
const char        kbd_digits[]      = "1234567890-/:;()&@\".,?!\'";
static const char kbd_specials[]    = "[]{}#%^*+=_\\|~<>$`\".,?!\'";

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/
static uint8_t getKeyboardIndex(nbgl_keyboard_t *keyboard, nbgl_touchStatePosition_t *position)
{
    uint8_t i = 0;
    // get index of key pressed
    if (position->y < KEYBOARD_KEY_HEIGHT) {
        // 1st row:
        i = position->x / NORMAL_KEY_WIDTH;
    }
    else if (position->y < (2 * KEYBOARD_KEY_HEIGHT)) {
        // 2nd row:
        i = FIRST_LINE_CHAR_COUNT + (position->x - SECOND_LINE_OFFSET) / NORMAL_KEY_WIDTH;
        if (i >= FIRST_LINE_CHAR_COUNT + SECOND_LINE_CHAR_COUNT) {
            i = FIRST_LINE_CHAR_COUNT + SECOND_LINE_CHAR_COUNT - 1;
        }
    }
    else if (position->y < (3 * KEYBOARD_KEY_HEIGHT)) {
        // 3rd row:
        if (keyboard->mode == MODE_LETTERS) {
            // shift does not exist in letters only mode
            if (!keyboard->lettersOnly) {
                if (position->x < SHIFT_KEY_WIDTH) {
                    i = SHIFT_KEY_INDEX;
                }
                else {
                    i = FIRST_LINE_CHAR_COUNT + SECOND_LINE_CHAR_COUNT
                        + (position->x - SHIFT_KEY_WIDTH) / NORMAL_KEY_WIDTH;
                    // Backspace key is a bit larger...
                    if (i >= 26) {
                        i = BACKSPACE_KEY_INDEX;
                    }
                }
            }
            else {
                i = FIRST_LINE_CHAR_COUNT + SECOND_LINE_CHAR_COUNT + position->x / NORMAL_KEY_WIDTH;
                // Backspace key is larger...
                if (i >= 26) {
                    i = BACKSPACE_KEY_INDEX;
                }
            }
        }
        else {
            if (position->x < SPECIAL_CHARS_KEY_WIDTH) {
                i = SPECIAL_KEYS_INDEX;
            }
            else {
                i = FIRST_LINE_CHAR_COUNT + SECOND_LINE_CHAR_COUNT
                    + (position->x - SPECIAL_CHARS_KEY_WIDTH) / NORMAL_KEY_WIDTH;
                // Backspace key is a bit larger...
                if (i >= 24) {
                    i = BACKSPACE_KEY_INDEX;
                }
            }
        }
    }
    else if (!keyboard->lettersOnly && (position->y < (4 * KEYBOARD_KEY_HEIGHT))) {
        // 4th row
        if (position->x < SWITCH_KEY_WIDTH) {
            i = DIGITS_SWITCH_KEY_INDEX;
        }
        else {
            i = SPACE_KEY_INDEX;
        }
    }
    return i;
}

// draw parts common to all modes of keyboard
static void keyboardDrawCommonLines(nbgl_keyboard_t *keyboard)
{
    nbgl_area_t rectArea;

    // clean full area
    rectArea.backgroundColor = keyboard->obj.area.backgroundColor;
    rectArea.x0              = keyboard->obj.area.x0;
    rectArea.y0              = keyboard->obj.area.y0;
    rectArea.width           = keyboard->obj.area.width;
    rectArea.height          = keyboard->obj.area.height;
    nbgl_frontDrawRect(&rectArea);

    /// draw horizontal lines
    rectArea.backgroundColor = keyboard->obj.area.backgroundColor;
    rectArea.x0              = keyboard->obj.area.x0;
    rectArea.y0              = keyboard->obj.area.y0;
    rectArea.width           = keyboard->obj.area.width;
    rectArea.height          = 1;
    nbgl_frontDrawLine(&rectArea, 1, keyboard->borderColor);  // 1st line (top)
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    nbgl_frontDrawLine(&rectArea, 1, keyboard->borderColor);  // 2nd line
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    nbgl_frontDrawLine(&rectArea, 1, keyboard->borderColor);  // 3rd line
    // in letter only mode, only draw the last line if not at bottom of screen
    if ((keyboard->obj.alignmentMarginY > 0) || (!keyboard->lettersOnly)) {
        rectArea.y0 += KEYBOARD_KEY_HEIGHT;
        nbgl_frontDrawLine(&rectArea, 1, keyboard->borderColor);  // 4th line
    }
    // in non letter only mode, only draw the last line if not at bottom of screen
    if ((keyboard->obj.alignmentMarginY > 0) && (!keyboard->lettersOnly)) {
        rectArea.y0 += KEYBOARD_KEY_HEIGHT;
        nbgl_frontDrawLine(&rectArea, 1, keyboard->borderColor);  // 5th line
    }
#ifdef TARGET_STAX
    /// then draw vertical line
    rectArea.x0     = keyboard->obj.area.x0;
    rectArea.y0     = keyboard->obj.area.y0;
    rectArea.width  = 1;
    rectArea.height = KEYBOARD_KEY_HEIGHT * 3;
    if (!keyboard->lettersOnly) {
        rectArea.height += KEYBOARD_KEY_HEIGHT;
    }
    nbgl_frontDrawLine(&rectArea, 0, keyboard->borderColor);  // 1st full line, on the left
#endif                                                        // TARGET_STAX
}

// draw full grid
static void keyboardDrawGrid(nbgl_keyboard_t *keyboard)
{
    nbgl_area_t rectArea;
    uint8_t     i;

    /// draw common lines
    keyboardDrawCommonLines(keyboard);

    // then all vertical lines separating keys
    rectArea.backgroundColor = keyboard->obj.area.backgroundColor;
    rectArea.x0              = keyboard->obj.area.x0;
    rectArea.y0              = keyboard->obj.area.y0;
    rectArea.width           = 1;
    rectArea.height          = KEYBOARD_KEY_HEIGHT;
#ifdef TARGET_APEX
    // On Apex, we start all lines 1px under the horizontal one
    rectArea.y0++;
    rectArea.height--;
#endif

    // First row of keys: 10 letters (qwertyuiop) or digits, so 9 separations
    for (i = 0; i < 9; i++) {
        rectArea.x0 += NORMAL_KEY_WIDTH;
        nbgl_frontDrawLine(&rectArea, 2, keyboard->borderColor);
    }

    // Second row: 9 letters (asdfghjkl) or digits
    rectArea.x0 = keyboard->obj.area.x0 + SECOND_LINE_OFFSET;
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    nbgl_frontDrawLine(&rectArea, 2, keyboard->borderColor);
    for (i = 10; i < 19; i++) {
        rectArea.x0 += NORMAL_KEY_WIDTH;
        nbgl_frontDrawLine(&rectArea, 2, keyboard->borderColor);
    }

    // Third row, it depends of the mode:
    // - 9 keys: Shift, 7 letters (zxcvbnm) and backspace in normal mode
    // - 8 keys: 7 letters (zxcvbnm) and backspace in letters only mode
    // - 7 keys: Special char key, 5 keys and backspace in digits mode
    uint8_t nbLines, firstShift;
    if (keyboard->mode == MODE_LETTERS) {
        if (keyboard->lettersOnly) {
            nbLines    = 7;
            firstShift = NORMAL_KEY_WIDTH;
        }
        else {
            nbLines    = 8;
            firstShift = SHIFT_KEY_WIDTH;
        }
    }
    else {
        nbLines    = 6;
        firstShift = SPECIAL_CHARS_KEY_WIDTH;
    }
    rectArea.x0 = keyboard->obj.area.x0 + firstShift;
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    for (i = 0; i < nbLines; i++) {
        nbgl_frontDrawLine(&rectArea, 2, keyboard->borderColor);
        rectArea.x0 += NORMAL_KEY_WIDTH;
    }

    // 4th row, only in Full mode
    if (!keyboard->lettersOnly || (keyboard->mode != MODE_LETTERS)) {
        rectArea.x0 = keyboard->obj.area.x0 + SWITCH_KEY_WIDTH;
        rectArea.y0 += KEYBOARD_KEY_HEIGHT;
#ifdef TARGET_APEX
        // On Apex, we start the last line 2px under the horizontal one
        rectArea.y0 += 2;
        rectArea.height -= 2;
#endif
        nbgl_frontDrawLine(&rectArea, 0, keyboard->borderColor);
    }
}

// draw letters for letters mode
static void keyboardDrawLetters(nbgl_keyboard_t *keyboard)
{
    uint8_t     i;
    nbgl_area_t rectArea;
    const char *keys;

    if (keyboard->casing != LOWER_CASE) {
        keys = kbd_chars_upper;
    }
    else {
        keys = kbd_chars;
    }

    rectArea.backgroundColor = keyboard->obj.area.backgroundColor;
    rectArea.y0              = keyboard->obj.area.y0 + LETTER_OFFSET_Y;
    rectArea.width           = 1;
    rectArea.height          = KEYBOARD_KEY_HEIGHT * 3;
    rectArea.x0              = keyboard->obj.area.x0;

    // First row of keys: 10 letters (qwertyuiop)
    for (i = 0; i < 10; i++) {
        rectArea.x0 = keyboard->obj.area.x0 + i * NORMAL_KEY_WIDTH;

        rectArea.x0
            += (NORMAL_KEY_WIDTH - nbgl_getCharWidth(SMALL_REGULAR_1BPP_FONT, &keys[i])) / 2;
        nbgl_drawText(
            &rectArea, &keys[i], 1, SMALL_REGULAR_1BPP_FONT, (IS_KEY_MASKED(i)) ? WHITE : BLACK);
    }
    // Second row: 9 letters (asdfghjkl)
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    for (i = 10; i < 19; i++) {
        rectArea.x0 = keyboard->obj.area.x0 + SECOND_LINE_OFFSET + (i - 10) * NORMAL_KEY_WIDTH;
        rectArea.x0
            += (NORMAL_KEY_WIDTH - nbgl_getCharWidth(SMALL_REGULAR_1BPP_FONT, &keys[i])) / 2;
        nbgl_drawText(
            &rectArea, &keys[i], 1, SMALL_REGULAR_1BPP_FONT, (IS_KEY_MASKED(i)) ? WHITE : BLACK);
    }
    // Third row: Shift key, 7 letters (zxcvbnm) and backspace
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    uint16_t offsetX;
    if (!keyboard->lettersOnly) {
        // draw background rectangle
#if defined(TARGET_STAX)
        rectArea.width  = SHIFT_KEY_WIDTH - 1;
        rectArea.height = KEYBOARD_KEY_HEIGHT;
        rectArea.x0     = 1;
#elif defined(TARGET_FLEX)
        rectArea.width  = SHIFT_KEY_WIDTH;
        rectArea.height = KEYBOARD_KEY_HEIGHT;
        rectArea.x0     = 0;
#elif defined(TARGET_APEX)
        if (keyboard->casing == LOWER_CASE) {
            rectArea.width = SHIFT_KEY_WIDTH;
        }
        else {
            rectArea.width = SHIFT_KEY_WIDTH + 1;
        }
        rectArea.height = KEYBOARD_KEY_HEIGHT + 1;
        rectArea.x0     = 0;
#endif
        rectArea.bpp             = NBGL_BPP_1;
        rectArea.y0              = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2;
        rectArea.backgroundColor = (keyboard->casing != LOWER_CASE) ? BLACK : WHITE;
        nbgl_frontDrawRect(&rectArea);

        // draw top & bottom horizontal lines (on Apex, only if in lower case)
#ifdef TARGET_APEX
        if (keyboard->casing == LOWER_CASE)
#endif
        {
            rectArea.width           = SHIFT_KEY_WIDTH - 1;
            rectArea.height          = 1;
            rectArea.x0              = 1;
            rectArea.y0              = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2;
            rectArea.backgroundColor = (keyboard->casing != LOWER_CASE) ? BLACK : WHITE;
            nbgl_frontDrawLine(&rectArea, 0, keyboard->borderColor);
            rectArea.y0 += KEYBOARD_KEY_HEIGHT;
            rectArea.backgroundColor = WHITE;
            nbgl_frontDrawLine(&rectArea, 0, keyboard->borderColor);
        }
        // draw Shift key
        rectArea.width  = SHIFT_ICON.width;
        rectArea.height = SHIFT_ICON.height;
        rectArea.bpp    = NBGL_BPP_1;
        rectArea.y0     = (keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2
                       + (KEYBOARD_KEY_HEIGHT - rectArea.height) / 2);
        rectArea.x0     = (SHIFT_KEY_WIDTH - rectArea.width) / 2;
        if (IS_KEY_MASKED(SHIFT_KEY_INDEX)) {
            rectArea.backgroundColor = WHITE;
            nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, WHITE, &SHIFT_LOCKED_ICON);
        }
        else {
            rectArea.backgroundColor = (keyboard->casing != LOWER_CASE) ? BLACK : WHITE;
            nbgl_drawIcon(
                &rectArea,
                NO_TRANSFORMATION,
                (keyboard->casing != LOWER_CASE) ? WHITE : BLACK,
                (keyboard->casing == LOCKED_UPPER_CASE) ? (&SHIFT_LOCKED_ICON) : (&SHIFT_ICON));
            rectArea.backgroundColor = WHITE;
        }
        offsetX = keyboard->obj.area.x0 + SHIFT_KEY_WIDTH;
    }
    else {
        offsetX = 0;
    }
    rectArea.y0 = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2 + LETTER_OFFSET_Y;
    for (i = 19; i < 26; i++) {
        rectArea.x0 = offsetX + (i - 19) * NORMAL_KEY_WIDTH;
        rectArea.x0
            += (NORMAL_KEY_WIDTH - nbgl_getCharWidth(SMALL_REGULAR_1BPP_FONT, &keys[i])) / 2;
        nbgl_drawText(
            &rectArea, &keys[i], 1, SMALL_REGULAR_1BPP_FONT, (IS_KEY_MASKED(i)) ? WHITE : BLACK);
    }
    // draw backspace
    rectArea.width  = BACKSPACE_ICON.width;
    rectArea.height = BACKSPACE_ICON.height;
    rectArea.bpp    = NBGL_BPP_1;
    rectArea.x0     = offsetX + 7 * NORMAL_KEY_WIDTH;
    rectArea.y0     = (keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2
                   + (KEYBOARD_KEY_HEIGHT - rectArea.height) / 2);
    if (!keyboard->lettersOnly) {
        rectArea.x0 += (BACKSPACE_KEY_WIDTH_FULL - rectArea.width) / 2;
    }
    else {
        rectArea.x0 += (BACKSPACE_KEY_WIDTH_LETTERS_ONLY - rectArea.width) / 2;
    }
    nbgl_drawIcon(&rectArea,
                  NO_TRANSFORMATION,
                  (IS_KEY_MASKED(BACKSPACE_KEY_INDEX)) ? WHITE : BLACK,
                  &BACKSPACE_ICON);

    // 4th row, only in Full mode
    if (!keyboard->lettersOnly) {
        rectArea.x0 = (SWITCH_KEY_WIDTH - nbgl_getTextWidth(SMALL_REGULAR_1BPP_FONT, ".?123")) / 2;
        rectArea.y0 = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 3 + LETTER_OFFSET_Y;
        nbgl_drawText(&rectArea,
                      ".?123",
                      5,
                      SMALL_REGULAR_1BPP_FONT,
                      (IS_KEY_MASKED(DIGITS_SWITCH_KEY_INDEX)) ? WHITE : BLACK);

        rectArea.x0 = SWITCH_KEY_WIDTH + (SPACE_KEY_WIDTH - SPACE_ICON.width) / 2;
        nbgl_drawIcon(&rectArea,
                      NO_TRANSFORMATION,
                      (IS_KEY_MASKED(SPACE_KEY_INDEX)) ? WHITE : BLACK,
                      &SPACE_ICON);
    }
}

// draw digits/special chars for digits/special mode
static void keyboardDrawDigits(nbgl_keyboard_t *keyboard)
{
    uint8_t     i;
    nbgl_area_t rectArea;
    const char *keys;

    if (keyboard->mode == MODE_DIGITS) {
        keys = kbd_digits;
    }
    else {
        keys = kbd_specials;
    }

    rectArea.backgroundColor = keyboard->obj.area.backgroundColor;
    rectArea.y0              = keyboard->obj.area.y0 + LETTER_OFFSET_Y;
    rectArea.width           = 1;
    rectArea.height          = KEYBOARD_KEY_HEIGHT * 3;
    rectArea.x0              = keyboard->obj.area.x0;

    // First row of keys: 10 digits (1234567890)
    for (i = 0; i < 10; i++) {
        rectArea.x0 = keyboard->obj.area.x0 + i * NORMAL_KEY_WIDTH;
        rectArea.x0
            += (NORMAL_KEY_WIDTH - nbgl_getCharWidth(SMALL_REGULAR_1BPP_FONT, &keys[i])) / 2;
        nbgl_drawText(
            &rectArea, &keys[i], 1, SMALL_REGULAR_1BPP_FONT, (IS_KEY_MASKED(i)) ? WHITE : BLACK);
    }
    // Second row: 9 keys ()
    rectArea.y0 += KEYBOARD_KEY_HEIGHT;
    for (i = 10; i < 19; i++) {
        rectArea.x0 = keyboard->obj.area.x0 + (i - 10) * NORMAL_KEY_WIDTH + SECOND_LINE_OFFSET;
        rectArea.x0
            += (NORMAL_KEY_WIDTH - nbgl_getCharWidth(SMALL_REGULAR_1BPP_FONT, &keys[i])) / 2;
        nbgl_drawText(
            &rectArea, &keys[i], 1, SMALL_REGULAR_1BPP_FONT, (IS_KEY_MASKED(i)) ? WHITE : BLACK);
    }
    // Third row: special key, 5 keys and backspace

    // draw "#+=" key
    if (keyboard->mode == MODE_DIGITS) {
        rectArea.x0
            = (SPECIAL_CHARS_KEY_WIDTH - nbgl_getTextWidth(SMALL_REGULAR_1BPP_FONT, "#+=")) / 2;
        rectArea.y0 = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2 + LETTER_OFFSET_Y;
        nbgl_drawText(&rectArea, "#+=", 3, SMALL_REGULAR_1BPP_FONT, BLACK);
    }
    else {
        rectArea.x0
            = (SPECIAL_CHARS_KEY_WIDTH - nbgl_getTextWidth(SMALL_REGULAR_1BPP_FONT, "123")) / 2;
        rectArea.y0 = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2 + LETTER_OFFSET_Y;
        nbgl_drawText(&rectArea, "123", 3, SMALL_REGULAR_1BPP_FONT, BLACK);
    }

    for (i = 19; i < 24; i++) {
        rectArea.x0 = SPECIAL_CHARS_KEY_WIDTH + (i - 19) * NORMAL_KEY_WIDTH;
        rectArea.x0
            += (NORMAL_KEY_WIDTH - nbgl_getCharWidth(SMALL_REGULAR_1BPP_FONT, &keys[i])) / 2;
        nbgl_drawText(
            &rectArea, &keys[i], 1, SMALL_REGULAR_1BPP_FONT, (IS_KEY_MASKED(i)) ? WHITE : BLACK);
    }
    // draw backspace
    rectArea.width  = BACKSPACE_ICON.width;
    rectArea.height = BACKSPACE_ICON.height;
    rectArea.bpp    = NBGL_BPP_1;
    rectArea.x0     = SPECIAL_CHARS_KEY_WIDTH + 5 * NORMAL_KEY_WIDTH;
    rectArea.y0     = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 2
                  + ((KEYBOARD_KEY_HEIGHT - rectArea.height) / 2);
    rectArea.x0 += (BACKSPACE_KEY_WIDTH_DIGITS - rectArea.width) / 2;
    nbgl_drawIcon(&rectArea, NO_TRANSFORMATION, BLACK, &BACKSPACE_ICON);

    // 4th row
    rectArea.x0 = (SWITCH_KEY_WIDTH - nbgl_getTextWidth(SMALL_REGULAR_1BPP_FONT, "ABC")) / 2;
    rectArea.y0 = keyboard->obj.area.y0 + KEYBOARD_KEY_HEIGHT * 3 + LETTER_OFFSET_Y;
    nbgl_drawText(&rectArea, "ABC", 3, SMALL_REGULAR_1BPP_FONT, BLACK);

    rectArea.x0 = SWITCH_KEY_WIDTH + (SPACE_KEY_WIDTH - SPACE_ICON.width) / 2;
    nbgl_drawIcon(&rectArea,
                  NO_TRANSFORMATION,
                  (IS_KEY_MASKED(SPACE_KEY_INDEX)) ? WHITE : BLACK,
                  &SPACE_ICON);
}

static void keyboardDraw(nbgl_keyboard_t *keyboard)
{
    // At first, draw grid
    keyboardDrawGrid(keyboard);
    if (keyboard->mode == MODE_LETTERS) {
        // then draw key content
        keyboardDrawLetters(keyboard);
    }
    else {
        ////// then draw key content //////
        keyboardDrawDigits(keyboard);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief function to be called when the keyboard object is touched
 *
 * @param obj touched object (keyboard)
 * @param eventType type of touch (only TOUCHED is accepted)
 * @return none
 */
void nbgl_keyboardTouchCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType)
{
    uint8_t                    firstIndex, lastIndex;
    nbgl_touchStatePosition_t *firstPosition, *lastPosition;
    nbgl_keyboard_t           *keyboard = (nbgl_keyboard_t *) obj;

    LOG_DEBUG(MISC_LOGGER, "keyboardTouchCallback(): eventType = %d\n", eventType);
    if (eventType != TOUCHED) {
        return;
    }
    if (nbgl_touchGetTouchedPosition(obj, &firstPosition, &lastPosition) == false) {
        return;
    }
    // modify positions with keyboard position
    firstPosition->x -= obj->area.x0;
    firstPosition->y -= obj->area.y0;
    lastPosition->x -= obj->area.x0;
    lastPosition->y -= obj->area.y0;

    firstIndex = getKeyboardIndex(keyboard, firstPosition);
    if (firstIndex > SPECIAL_KEYS_INDEX) {
        return;
    }
    lastIndex = getKeyboardIndex(keyboard, lastPosition);
    if (lastIndex > SPECIAL_KEYS_INDEX) {
        return;
    }
    // if position of finger has moved durinng press to another "key", drop it
    if (lastIndex != firstIndex) {
        return;
    }

    if (keyboard->mode == MODE_LETTERS) {
        keyboardCase_t cur_casing = keyboard->casing;
        // if the casing mode was upper (not-locked), go back to lower case
        if ((keyboard->casing == UPPER_CASE) && (firstIndex != SHIFT_KEY_INDEX)
            && ((IS_KEY_MASKED(firstIndex)) == 0)) {
            keyboard->casing = LOWER_CASE;
            // just redraw, refresh will be done by client (user of keyboard)
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            keyboard->needsRefresh = true;
        }
        if ((firstIndex < 26) && ((IS_KEY_MASKED(firstIndex)) == 0)) {
            keyboard->callback((cur_casing != LOWER_CASE) ? kbd_chars_upper[firstIndex]
                                                          : kbd_chars[firstIndex]);
        }
        else if (firstIndex == SHIFT_KEY_INDEX) {
            switch (keyboard->casing) {
                case LOWER_CASE:
                    keyboard->casing = UPPER_CASE;
                    break;
                case UPPER_CASE:
                    keyboard->casing = LOCKED_UPPER_CASE;
                    break;
                case LOCKED_UPPER_CASE:
                    keyboard->casing = LOWER_CASE;
                    break;
            }
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_REFRESH,
                                               POST_REFRESH_FORCE_POWER_ON);
        }
        else if (firstIndex == DIGITS_SWITCH_KEY_INDEX) {  // switch to digits
            keyboard->mode = MODE_DIGITS;
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            nbgl_refreshSpecialWithPostRefresh(FULL_COLOR_REFRESH, POST_REFRESH_FORCE_POWER_ON);
        }
    }
    else if (keyboard->mode == MODE_DIGITS) {
        if (firstIndex < 26) {
            keyboard->callback(kbd_digits[firstIndex]);
        }
        else if (firstIndex == SPECIAL_KEYS_INDEX) {
            keyboard->mode = MODE_SPECIAL;
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_REFRESH,
                                               POST_REFRESH_FORCE_POWER_ON);
        }
        else if (firstIndex == DIGITS_SWITCH_KEY_INDEX) {  // switch to letters
            keyboard->mode = MODE_LETTERS;
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            nbgl_refreshSpecialWithPostRefresh(FULL_COLOR_REFRESH, POST_REFRESH_FORCE_POWER_ON);
        }
    }
    else if (keyboard->mode == MODE_SPECIAL) {
        if (firstIndex < 26) {
            keyboard->callback(kbd_specials[firstIndex]);
        }
        else if (firstIndex == SPECIAL_KEYS_INDEX) {
            keyboard->mode = MODE_DIGITS;
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            nbgl_refreshSpecialWithPostRefresh(BLACK_AND_WHITE_REFRESH,
                                               POST_REFRESH_FORCE_POWER_ON);
        }
        else if (firstIndex == DIGITS_SWITCH_KEY_INDEX) {  // switch to letters
            keyboard->mode = MODE_LETTERS;
            nbgl_objDraw((nbgl_obj_t *) keyboard);
            nbgl_refreshSpecialWithPostRefresh(FULL_COLOR_REFRESH, POST_REFRESH_FORCE_POWER_ON);
        }
    }
    if (firstIndex == BACKSPACE_KEY_INDEX) {  // backspace
        keyboard->callback(BACKSPACE_KEY);
    }
    else if ((firstIndex == SPACE_KEY_INDEX) && ((IS_KEY_MASKED(SPACE_KEY_INDEX)) == 0)) {  // space
        keyboard->callback(' ');
    }
}

/**
 * @brief This function gets the position (top-left corner) of the key at the
 * given index. (to be used for Testing purpose)
 *
 * @param kbd the object to be drawned
 * @param index ascii character (in lower-case)
 * @param x [out] the top-left position
 * @param y [out] the top-left position
 * @return true if found, false otherwise
 */
bool nbgl_keyboardGetPosition(nbgl_keyboard_t *kbd, char index, uint16_t *x, uint16_t *y)
{
    uint8_t charIndex = 0;

    while (charIndex < 26) {
        if (index == kbd_chars[charIndex]) {
            break;
        }
        charIndex++;
    }

    // if in first line
    if (charIndex < FIRST_LINE_CHAR_COUNT) {
        *x = kbd->obj.area.x0 + charIndex * NORMAL_KEY_WIDTH;
        *y = kbd->obj.area.y0;
    }
    else if (charIndex < (FIRST_LINE_CHAR_COUNT + SECOND_LINE_CHAR_COUNT)) {
        *x = kbd->obj.area.x0 + (charIndex - FIRST_LINE_CHAR_COUNT) * NORMAL_KEY_WIDTH
             + SECOND_LINE_OFFSET;
        *y = kbd->obj.area.y0 + KEYBOARD_KEY_HEIGHT;
    }
    else if (charIndex < sizeof(kbd_chars)) {
        if (kbd->mode == MODE_LETTERS) {
            *x = kbd->obj.area.x0
                 + (charIndex - FIRST_LINE_CHAR_COUNT - SECOND_LINE_CHAR_COUNT) * NORMAL_KEY_WIDTH;
            // shift does not exist in letters only mode
            if (!kbd->lettersOnly) {
                *x = *x + SHIFT_KEY_WIDTH;
            }
        }
        else {
            *x = kbd->obj.area.x0
                 + (charIndex - FIRST_LINE_CHAR_COUNT - SECOND_LINE_CHAR_COUNT) * NORMAL_KEY_WIDTH
                 + SPECIAL_CHARS_KEY_WIDTH;
        }
        *y = kbd->obj.area.y0 + 2 * KEYBOARD_KEY_HEIGHT;
    }
    else {
        return false;
    }
    return true;
}

/**
 * @brief This function draws a keyboard object
 *
 * @param kbd the object to be drawned
 */
void nbgl_objDrawKeyboard(nbgl_keyboard_t *kbd)
{
    kbd->obj.touchMask = (1 << TOUCHED);
    kbd->obj.touchId   = KEYBOARD_ID;
    kbd->needsRefresh  = false;

    keyboardDraw(kbd);

#ifdef TARGET_STAX
    // If a keyboard in the screen, exclude nothing from touch, to avoid missing touch on
    // left keys
    touch_exclude_borders(0);
#endif  // TARGET_STAX
}
#endif  // HAVE_SE_TOUCH
#endif  // NBGL_KEYBOARD
