/**
 * @file nbgl_obj.h
 * @brief API to draw all basic graphic objects
 *
 */

#ifndef NBGL_OBJ_H
#define NBGL_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nbgl_types.h"
#include "nbgl_fonts.h"
#include "ux_loc.h"

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
// Keypad special key values
#define BACKSPACE_KEY 8
#define VALIDATE_KEY  '\r'

// for Keyboard
#ifdef NBGL_KEYBOARD
#ifdef SCREEN_SIZE_WALLET
#if defined(TARGET_STAX)
#define KEYBOARD_KEY_HEIGHT 60
#elif defined(TARGET_FLEX)
#define KEYBOARD_KEY_HEIGHT 72
#endif  // TARGETS

// index of keys for keyMask field of nbgl_keyboard_t
#define SHIFT_KEY_INDEX         26
#define DIGITS_SWITCH_KEY_INDEX 27
#define BACKSPACE_KEY_INDEX     28
#define SPACE_KEY_INDEX         29
#define SPECIAL_KEYS_INDEX      30

#else  // SCREEN_SIZE_WALLET
#define KEYBOARD_KEY_WIDTH  14
#define KEYBOARD_KEY_HEIGHT 14
#define KEYBOARD_WIDTH      (5 * KEYBOARD_KEY_WIDTH)
#endif  // SCREEN_SIZE_WALLET
#endif  // NBGL_KEYBOARD

// for Keypad
#ifdef SCREEN_SIZE_WALLET
#if defined(TARGET_STAX)
#define KEYPAD_KEY_HEIGHT 104
#elif defined(TARGET_FLEX)
#define KEYPAD_KEY_HEIGHT 88
#endif  // TARGETS
#else   // SCREEN_SIZE_WALLET
#define KEYPAD_WIDTH  114
#define KEYPAD_HEIGHT 18
#endif  // SCREEN_SIZE_WALLET
#define KEYPAD_MAX_DIGITS 12

#ifdef SCREEN_SIZE_WALLET
// external margin in pixels
#if defined(TARGET_STAX)
#define BORDER_MARGIN        24
#define BOTTOM_BORDER_MARGIN 24
#elif defined(TARGET_FLEX)
#define BORDER_MARGIN        32
#define BOTTOM_BORDER_MARGIN 24
#endif  // TARGETS

// Back button header height
#if defined(TARGET_STAX)
#define BACK_BUTTON_HEADER_HEIGHT 88
#elif defined(TARGET_FLEX)
#define BACK_BUTTON_HEADER_HEIGHT 96
#endif  // TARGETS

// common dimensions for buttons
#if COMMON_RADIUS == 40
#define BUTTON_RADIUS   RADIUS_40_PIXELS
#define BUTTON_DIAMETER (COMMON_RADIUS * 2)
#elif COMMON_RADIUS == 44
#define BUTTON_RADIUS   RADIUS_44_PIXELS
#define BUTTON_DIAMETER (COMMON_RADIUS * 2)
#endif  // COMMON_RADIUS

// width & height for spinner
#if defined(TARGET_STAX)
#define SPINNER_WIDTH  60
#define SPINNER_HEIGHT 44
#elif defined(TARGET_FLEX)
#define SPINNER_WIDTH  64
#define SPINNER_HEIGHT 48
#endif  // TARGETS

// width & height for radio button
#if defined(TARGET_STAX)
#define RADIO_WIDTH  32
#define RADIO_HEIGHT 32
#elif defined(TARGET_FLEX)
#define RADIO_WIDTH  40
#define RADIO_HEIGHT 40
#endif  // TARGETS

// common small icons
#if SMALL_ICON_SIZE == 32
#define SPACE_ICON           C_Space_32px
#define BACKSPACE_ICON       C_Erase_32px
#define SHIFT_ICON           C_Maj_32px
#define SHIFT_LOCKED_ICON    C_Maj_Lock_32px
#define VALIDATE_ICON        C_Check_32px
#define RADIO_OFF_ICON       C_radio_inactive_32px
#define RADIO_ON_ICON        C_radio_active_32px
#define PUSH_ICON            C_Chevron_32px
#define LEFT_ARROW_ICON      C_Back_32px
#define RIGHT_ARROW_ICON     C_Next_32px
#define CHEVRON_BACK_ICON    C_Chevron_Back_32px
#define CHEVRON_NEXT_ICON    C_Chevron_Next_32px
#define CLOSE_ICON           C_Close_32px
#define WHEEL_ICON           C_Settings_32px
#define INFO_I_ICON          C_Info_32px
#define QRCODE_ICON          C_QRCode_32px
#define MINI_PUSH_ICON       C_Mini_Push_32px
#define WARNING_ICON         C_Warning_32px
#define ROUND_WARN_ICON      C_Important_Circle_32px
#define PRIVACY_ICON         C_Privacy_32px
#define EXCLAMATION_ICON     C_Exclamation_32px
#define QUESTION_ICON        C_Question_32px
#define DIGIT_ICON           C_round_24px
#define QUESTION_CIRCLE_ICON C_Question_Mark_Circle_32px
#elif SMALL_ICON_SIZE == 40
#define SPACE_ICON           C_Space_40px
#define BACKSPACE_ICON       C_Erase_40px
#define SHIFT_ICON           C_Maj_40px
#define SHIFT_LOCKED_ICON    C_Maj_Lock_40px
#define VALIDATE_ICON        C_Check_40px
#define RADIO_OFF_ICON       C_radio_inactive_40px
#define RADIO_ON_ICON        C_radio_active_40px
#define PUSH_ICON            C_Chevron_40px
#define LEFT_ARROW_ICON      C_Back_40px
#define RIGHT_ARROW_ICON     C_Next_40px
#define CHEVRON_BACK_ICON    C_Chevron_Back_40px
#define CHEVRON_NEXT_ICON    C_Chevron_Next_40px
#define CLOSE_ICON           C_Close_40px
#define WHEEL_ICON           C_Settings_40px
#define INFO_I_ICON          C_Info_40px
#define QRCODE_ICON          C_QRCode_40px
#define MINI_PUSH_ICON       C_Mini_Push_40px
#define WARNING_ICON         C_Warning_40px
#define ROUND_WARN_ICON      C_Important_Circle_40px
#define PRIVACY_ICON         C_Privacy_40px
#define EXCLAMATION_ICON     C_Exclamation_40px
#define QUESTION_ICON        C_Question_40px
#define DIGIT_ICON           C_pin_24
#define QUESTION_CIRCLE_ICON C_Question_Mark_Circle_40px
#endif  // SMALL_ICON_SIZE

// common large icons
#if LARGE_ICON_SIZE == 64
#define CHECK_CIRCLE_ICON     C_Check_Circle_64px
#define DENIED_CIRCLE_ICON    C_Denied_Circle_64px
#define IMPORTANT_CIRCLE_ICON C_Important_Circle_64px
#define LARGE_WARNING_ICON    C_Warning_64px
#else  // LARGE_ICON_SIZE
#error Undefined LARGE_ICON_SIZE
#endif  // LARGE_ICON_SIZE

// For backward compatibility, to be remove later
#define C_warning64px        _Pragma("GCC warning \"Deprecated constant!\"") C_Warning_64px
#define C_round_warning_64px _Pragma("GCC warning \"Deprecated constant!\"") C_Important_Circle_64px
#define C_round_check_64px   _Pragma("GCC warning \"Deprecated constant!\"") C_Check_Circle_64px
#define C_Message_64px       _Pragma("GCC warning \"Deprecated constant!\"") C_Review_64px
#define C_leftArrow32px      _Pragma("GCC warning \"Deprecated constant!\"") C_Back_32px
#define C_Next32px           _Pragma("GCC warning \"Deprecated constant!\"") C_Next_32px
#define C_round_cross_64px   _Pragma("GCC warning \"Deprecated constant!\"") C_Denied_Circle_64px

// max number of pages when nbgl_page_indicator_t uses dashes (above, it uses n / nb_pages)
#define NB_MAX_PAGES_WITH_DASHES 6

// number of spinner positions
#define NB_SPINNER_POSITIONS 4
#endif  // SCREEN_SIZE_WALLET

/**********************
 *      TYPEDEFS
 **********************/

#define SWIPE_MASK \
    ((1 << SWIPED_UP) | (1 << SWIPED_DOWN) | (1 << SWIPED_LEFT) | (1 << SWIPED_RIGHT))

/**
 * @brief The different pressed buttons
 *
 */
#define LEFT_BUTTON   0x01  ///< Left button event
#define RIGHT_BUTTON  0x02  ///< Right button event
#define BOTH_BUTTONS  0x03  ///< Both buttons event
#define RELEASED_MASK 0x80  ///< released (see LSB bits to know what buttons are released)
#define CONTINUOUS_MASK \
    0x40  ///< if set, means that the button(s) is continuously pressed (this event is sent every
          ///< 300ms after the first 800ms)

typedef enum {
    BUTTON_LEFT_PRESSED = 0,          ///< Sent when Left button is released
    BUTTON_RIGHT_PRESSED,             ///< Send when Right button is released
    BUTTON_LEFT_CONTINUOUS_PRESSED,   ///< Send when Left button is continuouly pressed (sent every
                                      ///< 300ms after the first 800ms)
    BUTTON_RIGHT_CONTINUOUS_PRESSED,  ///< Send when Right button is continuouly pressed (sent every
                                      ///< 300ms after the first 800ms)
    BUTTON_BOTH_PRESSED,              ///< Sent when both buttons are released
    BUTTON_BOTH_TOUCHED,              ///< Sent when both buttons are touched
    INVALID_BUTTON_EVENT
} nbgl_buttonEvent_t;

/**
 * @brief prototype of function to be called when a button event is received by an object (TODO:
 * change to screen?)
 * @param obj the concerned object
 * @param buttonEvent event on buttons
 */
typedef void (*nbgl_buttonCallback_t)(void *obj, nbgl_buttonEvent_t buttonEvent);

/**
 * @brief The low level Touchscreen event, coming from driver
 *
 */
typedef struct {
    nbgl_touchState_t state;  ///< state of the touch event, e.g @ref PRESSED or @ref RELEASED
#ifdef HAVE_HW_TOUCH_SWIPE
    nbgl_hardwareSwipe_t swipe;  ///< potential reported swipe
#endif                           // HAVE_HW_TOUCH_SWIPE
    int16_t
        x;  ///< horizontal position of the touch (or for a @ref RELEASED the last touched point)
    int16_t y;  ///< vertical position of the touch (or for a @ref RELEASED the last touched point)
} nbgl_touchStatePosition_t;

/**
 * @brief prototype of function to be called when a touch event is received by an object
 * @param obj the concerned object
 * @param eventType type of touch event
 */
typedef void (*nbgl_touchCallback_t)(void *obj, nbgl_touchType_t eventType);

/**
 * @brief Common structure for all graphical objects
 *
 * @note this type must never be instantiated
 */
typedef struct PACKED__ nbgl_obj_s {
    nbgl_area_t area;  ///< absolute position, backGround color and size of the object. DO NOT MOVE
                       ///< THIS FIELD
    int16_t
        rel_x0;  ///< horizontal position of top-left corner relative to parent's top-left corner
    int16_t rel_y0;  ///< vertical position of top-left corner relative to parent's top-left corner,
                     ///< must be multiple of 4
    struct nbgl_obj_s *parent;            ///< parent of this object
    struct nbgl_obj_s *alignTo;           ///< object to align to (parent by default)
    nbgl_aligment_t    alignment;         ///< type of alignment
    int16_t            alignmentMarginX;  ///< horizontal margin when aligning
    int16_t            alignmentMarginY;  ///< vertical margin when aligning
    nbgl_obj_type_t    type;              ///< type of the graphical object, must be explicitly set
    uint16_t touchMask;  ///< bit mask to tell engine which touch events are handled by this object
    uint8_t  touchId;  ///< a unique identifier (by screen) to be used by external test environment
                       ///< (TTYT or Screenshots)
} nbgl_obj_t;

/**
 * @brief struct to represent a container (@ref CONTAINER type)
 *
 * @note the main screen is a kind of container
 *
 */
typedef struct PACKED__ nbgl_container_s {
    nbgl_obj_t          obj;     ///< common part
    nbgl_direction_t    layout;  ///< layout of children inside this object
    uint8_t             nbChildren;
    bool                forceClean;  ///< if set to true, a paint will be done with background color
    struct nbgl_obj_s **children;    ///< children of this object (nbChildren size)
} nbgl_container_t;

/**
 * @brief struct to represent a vertical or horizontal line
 *
 */
typedef struct PACKED__ nbgl_line_s {
    nbgl_obj_t       obj;        ///<  common part
    nbgl_direction_t direction;  ///< direction of the line, e.g @ref VERTICAL or @ref HORIZONTAL
    color_t          lineColor;  ///< color of the line
    uint8_t thickness;  ///< thickness of the line in pixel, maybe different from height for
                        ///< horizontal line
    uint8_t offset;  ///< the object height being always 4, with a y0 multiple of 4, this offset is
                     ///< use to move the line within these 4 pixels
} nbgl_line_t;

/**
 * @brief prototype of function to be called when a @ref IMAGE object is drawned, and no buffer was
 * provided
 * @param token provided token in @ref IMAGE object
 * @return the icn details to be drawned in image object
 */
typedef nbgl_icon_details_t *(*onImageDrawCallback_t)(uint8_t token);

/**
 * @brief  struct to represent an image (@ref IMAGE type)
 *
 */
typedef struct PACKED__ nbgl_image_s {
    nbgl_obj_t obj;           // common part
    color_t foregroundColor;  ///< color set to '1' bits, for 1PBB images. '0' are set to background
                              ///< color.
    nbgl_transformation_t      transformation;  ///< usually NO_TRANSFORMATION
    const nbgl_icon_details_t *buffer;     ///< buffer containing bitmap, with exact same size as
                                           ///< object (width*height*bpp/8 bytes)
    onImageDrawCallback_t onDrawCallback;  ///< function called if buffer is NULL, with above token
                                           ///< as parameter. Can be NULL
    uint8_t token;                         ///< token to use as param of onDrawCallback
} nbgl_image_t;

/**
 * @brief  struct to represent an image file object (@ref IMAGE_FILE type)
 * The source of the data is an image file with header. width and height are given in this header
 *
 */
typedef struct PACKED__ nbgl_image_file_s {
    nbgl_obj_t     obj;     // common part
    const uint8_t *buffer;  ///< buffer containing image file
} nbgl_image_file_t;

/**
 * @brief  struct to represent a QR code (@ref QR_CODE type), whose size is fixed
 *
 */
typedef struct PACKED__ nbgl_qrcode_s {
    nbgl_obj_t obj;           // common part
    color_t foregroundColor;  ///< color set to '1' bits, for 1PBB images. '0' are set to background
                              ///< color.
    nbgl_qrcode_version_t version;  ///< requested version, if V10, size will be fixed to 228*228,
                                    ///< if V4, size will be fixed to 132*132
    const char *text;               ///< text single line (NULL terminated)
} nbgl_qrcode_t;

/**
 * @brief struct to represent a radio button (@ref RADIO_BUTTON type)
 *
 * @note size is fixed
 *
 */
typedef struct PACKED__ nbgl_radio_s {
    nbgl_obj_t   obj;          // common part
    color_t      activeColor;  ///< color set to to inner circle, when active.
    color_t      borderColor;  ///< color set to border.
    nbgl_state_t state;        ///< state of the radio button. Active is when state == @ref ON_STATE
} nbgl_radio_t;

/**
 * @brief struct to represent a switch (size is fixed) (@ref SWITCH type)
 *
 */
typedef struct PACKED__ nbgl_switch_s {
    nbgl_obj_t   obj;       // common part
    color_t      onColor;   ///< color set to border and knob, when ON (knob on the right).
    color_t      offColor;  ///< color set to border and knob, when OFF (knob on the left).
    nbgl_state_t state;     ///< state of the switch.
} nbgl_switch_t;

/**
 * @brief  struct to represent a progress bar (@ref PROGRESS_BAR type)
 * @note if withBorder, the stroke of the border is fixed (3 pixels)
 */
typedef struct PACKED__ nbgl_progress_bar_s {
    nbgl_obj_t obj;  // common part
    uint16_t   previousWidth;
    uint8_t    state;           ///< state of the progress, in % (from 0 to 100).
    uint8_t partialRedraw : 1;  ///< set to true to redraw only partially the object (update state).
    uint8_t withBorder : 1;     ///< if set to true, a border in black surround the whole object
    uint8_t resetIfOverriden : 1;  ///< if set to true, the state is reset to 0 if the screen is
                                   ///< covered by another screen
    color_t foregroundColor;       ///< color of the inner progress bar and border (if applicable)
} nbgl_progress_bar_t;

/**
 * @brief Style to apply to @ref nbgl_page_indicator_t
 *
 */
typedef enum {
    PROGRESSIVE_INDICATOR = 0,  ///< all dashes before active page are black
    CURRENT_INDICATOR           ///< only current page dash is black
} nbgl_page_indicator_style_t;

/**
 * @brief  struct to represent a navigation bar (@ref PAGE_INDICATOR type)
 * There can be up to 5 page indicators, whose shape is fixed.
 * If there are more than 5 pages, the middle indicator will be "..."
 *
 * @note height is fixed
 */
typedef struct PACKED__ nbgl_navigation_bar_s {
    nbgl_obj_t                  obj;         ///< common part
    uint8_t                     nbPages;     ///< number of pages.
    uint8_t                     activePage;  ///< index of active page (from 0 to nbPages-1).
    nbgl_page_indicator_style_t style;       ///< Style to apply
} nbgl_page_indicator_t;

/**
 * @brief prototype of function to be called when a @ref TEXT_AREA object is drawned, and no text
 * was provided
 * @param token provided token in @ref TEXT_AREA object
 * @return an ASCII string (null terminated) to be drawned in text area
 */
typedef char *(*onTextDrawCallback_t)(uint8_t token);

/**
 * @brief struct to represent a button (@ref BUTTON type)
 * that can contain a text and/or an icon
 * @note border width is fixed (2 pixels)
 *
 */
typedef struct PACKED__ nbgl_button_s {
    nbgl_obj_t obj;                  ///< common part
    color_t    innerColor;           ///< color set inside of the button
    color_t    borderColor;          ///< color set to button's border
    color_t    foregroundColor;      ///< color set to '1' bits in icon, and text. '0' are set to
                                     ///< innerColor color.
    nbgl_radius_t        radius;     ///< radius of the corners, must be a multiple of 4.
    nbgl_font_id_e       fontId;     ///< id of the font to use, if any
    bool                 localized;  ///< unused, kept for compatibility
    const char          *text;       ///< single line UTF-8 text (NULL terminated)
    onTextDrawCallback_t onDrawCallback;  ///< function called if not NULL, with above token as
                                          ///< parameter to get the text of the button
    uint8_t                    token;     ///< token to use as param of onDrawCallback
    const nbgl_icon_details_t *icon;  ///< buffer containing icons bitmap. Set to NULL when no icon
} nbgl_button_t;

/**
 * @brief struct to represent a text area (@ref TEXT_AREA type)
 *
 */
typedef struct PACKED__ nbgl_text_area_s {
    nbgl_obj_t obj;        ///< common part
    color_t    textColor;  ///< color set to '1' bits in text. '0' are set to backgroundColor color.
    nbgl_aligment_t textAlignment;  ///< alignment of text within the area
    nbgl_style_t    style;          ///< to define the style of border
    nbgl_font_id_e  fontId;         ///< id of the font to use
    bool autoHideLongLine;  ///< if set to true, replace beginning of line by ... to keep it single
                            ///< line
    bool    wrapping;       ///< if set to true, break lines on ' ' when possible
    uint8_t nbMaxLines;  ///< if >0, replace end (3 last chars) of line (nbMaxLines-1) by "..." and
                         ///< stop display here
    const char *text;    ///< ASCII text to draw (NULL terminated). Can be NULL.
    uint16_t    len;     ///< number of bytes to write (if 0, max number of chars or strlen is used)
    onTextDrawCallback_t
            onDrawCallback;  ///< function called if not NULL to get the text of the text area
    uint8_t token;           ///< token to use as param of onDrawCallback
} nbgl_text_area_t;

/**
 * @brief struct to represent a text entry area (@ref TEXT_ENTRY type)
 *
 */
typedef struct PACKED__ nbgl_text_entry_s {
    nbgl_obj_t     obj;      ///< common part
    nbgl_font_id_e fontId;   ///< id of the font to use
    uint8_t        nbChars;  ///< number of char placeholders to display (8 or 9 chars).
    const char    *text;     ///< text to display (up to nbChars chars).
} nbgl_text_entry_t;

typedef struct PACKED__ nbgl_mask_control_s {
    nbgl_obj_t obj;            ///< common part
    bool       enableMasking;  ///< true: Enable masking of area / false: Disable masking of area
    uint8_t    maskIndex;      ///< index of mask
} nbgl_mask_control_t;

/**
 * @brief struct to represent a "spinner", represented by the Ledger corners, in gray, with one of
 * the corners in black (@ref SPINNER type)
 *
 */
typedef struct PACKED__ nbgl_spinner_s {
    nbgl_obj_t obj;       ///< common part
    uint8_t    position;  ///< position of the spinner (from 0 to 3). If set to 0xFF, the spinner is
                          ///< entirely black
} nbgl_spinner_t;

/**
 * @brief prototype of function to be called when a valid key is pressed on keyboard
 * Backspace is equal to 0x8 (ASCII code), Validate (for Keypad) is equal to 15 ('\\r')
 * @param touchedKey char typed on keyboard
 */
typedef void (*keyboardCallback_t)(char touchedKey);

/**
 * @brief Mode in which to open/set the keyboard
 *
 */
typedef enum {
#ifdef HAVE_SE_TOUCH
    MODE_LETTERS = 0,  ///< letters mode
    MODE_DIGITS,       ///< digits and some special characters mode
    MODE_SPECIAL       ///< extended special characters mode
#else                  // HAVE_SE_TOUCH
    MODE_LOWER_LETTERS,        ///< lower case letters mode
    MODE_UPPER_LETTERS,        ///< upper case letters mode
    MODE_DIGITS_AND_SPECIALS,  ///< digits and some special characters mode
    MODE_NONE                  ///< no mode defined (only for Nanos)
#endif                 // HAVE_SE_TOUCH
} keyboardMode_t;

/**
 * @brief Letters casing in which to open/set the keyboard
 *
 */
typedef enum {
    LOWER_CASE = 0,    ///< lower case mode
    UPPER_CASE,        ///< upper case mode for one character
    LOCKED_UPPER_CASE  ///< locked upper case mode
} keyboardCase_t;

/**
 * @brief struct to represent a keyboard (@ref KEYBOARD type)
 *
 */
typedef struct PACKED__ nbgl_keyboard_s {
    nbgl_obj_t obj;          ///< common part
    color_t    textColor;    ///< color set to letters.
    color_t    borderColor;  ///< color set to key borders
    bool       lettersOnly;  ///< if true, only display letter keys and Backspace
#ifdef SCREEN_SIZE_WALLET
    bool needsRefresh;  ///< if true, means that the keyboard has been redrawn and needs a refresh
    keyboardCase_t casing;  ///< keyboard casing mode (lower, upper once or upper locked)
#else                       // SCREEN_SIZE_WALLET
    bool    enableBackspace;   ///< if true, Backspace key is enabled
    bool    enableValidate;    ///< if true, Validate key is enabled
    uint8_t selectedCharIndex;
#endif                      // SCREEN_SIZE_WALLET
    keyboardMode_t mode;    ///< keyboard mode to start with
    uint32_t keyMask;  ///< mask used to disable some keys in letters only mod. The 26 LSB bits of
                       ///< mask are used, for the 26 letters of a QWERTY keyboard. Bit[0] for Q,
                       ///< Bit[1] for W and so on
    keyboardCallback_t callback;  ///< function called when an active key is pressed
} nbgl_keyboard_t;

/**
 * @brief struct to represent a keypad (@ref KEYPAD type)
 *
 */
typedef struct PACKED__ nbgl_keypad_s {
    nbgl_obj_t obj;  ///< common part
#ifdef SCREEN_SIZE_WALLET
    color_t borderColor;                 ///< color set to key borders
    bool    softValidation;              ///< if true, the "check icon" is replaced by an arrow
    bool    enableDigits;                ///< if true, Digit keys are enabled
    bool    partial;                     ///< if true, means that only some keys have changed
    uint8_t digitIndexes[5];             ///< array of digits indexes, 4 bits per digit
#else                                    // SCREEN_SIZE_WALLET
    uint8_t selectedKey;  ///< selected key position
#endif                                   // SCREEN_SIZE_WALLET
    bool               enableBackspace;  ///< if true, Backspace key is enabled
    bool               enableValidate;   ///< if true, Validate key is enabled
    bool               shuffled;         ///< if true, Digit keys are shuffled
    keyboardCallback_t callback;         ///< function called when an active key is pressed
} nbgl_keypad_t;

/**
 * @brief ids of touchable objects, for external stimulus (by Testing environment)
 *
 */
enum {
    BOTTOM_BUTTON_ID = 1,
    LEFT_BUTTON_ID,
    RIGHT_BUTTON_ID,
    WHOLE_SCREEN_ID,
    TOP_RIGHT_BUTTON_ID,
    BACK_BUTTON_ID,
    SINGLE_BUTTON_ID,
    EXTRA_BUTTON_ID,
    CHOICE_1_ID,
    CHOICE_2_ID,
    CHOICE_3_ID,
    KEYPAD_ID,
    KEYBOARD_ID,
    ENTERED_TEXT_ID,
    VALUE_BUTTON_1_ID,
    VALUE_BUTTON_2_ID,
    VALUE_BUTTON_3_ID,
    LONG_PRESS_BUTTON_ID,
    TIP_BOX_ID,
    CONTROLS_ID,  // when multiple controls in the same pages (buttons, switches, radios)
    NB_CONTROL_IDS
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void nbgl_refresh(void);
void nbgl_refreshSpecial(nbgl_refresh_mode_t mode);
void nbgl_refreshSpecialWithPostRefresh(nbgl_refresh_mode_t mode, nbgl_post_refresh_t post_refresh);
bool nbgl_refreshIsNeeded(void);
void nbgl_refreshReset(void);

void     nbgl_objInit(void);
void     nbgl_objDraw(nbgl_obj_t *obj);
void     nbgl_objAllowDrawing(bool enable);
uint8_t *nbgl_objGetRAMBuffer(void);
bool     nbgl_objIsUx(nbgl_obj_t *obj);

void         nbgl_objPoolRelease(uint8_t layer);
nbgl_obj_t  *nbgl_objPoolGet(nbgl_obj_type_t type, uint8_t layer);
nbgl_obj_t  *nbgl_objPoolGetPrevious(nbgl_obj_t *obj, uint8_t layer);
uint8_t      nbgl_objPoolGetId(nbgl_obj_t *obj);
int          nbgl_objPoolGetArray(nbgl_obj_type_t type,
                                  uint8_t         nbObjs,
                                  uint8_t         layer,
                                  nbgl_obj_t    **objArray);
uint8_t      nbgl_objPoolGetNbUsed(uint8_t layer);
void         nbgl_containerPoolRelease(uint8_t layer);
nbgl_obj_t **nbgl_containerPoolGet(uint8_t nbObjs, uint8_t layer);
uint8_t      nbgl_containerPoolGetNbUsed(uint8_t layer);

// for internal use
void nbgl_objDrawKeyboard(nbgl_keyboard_t *kbd);
void nbgl_objDrawKeypad(nbgl_keypad_t *kbd);
#ifdef HAVE_SE_TOUCH
void nbgl_keyboardTouchCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType);
void nbgl_keypadTouchCallback(nbgl_obj_t *obj, nbgl_touchType_t eventType);

bool nbgl_keyboardGetPosition(nbgl_keyboard_t *kbd, char index, uint16_t *x, uint16_t *y);
bool nbgl_keypadGetPosition(nbgl_keypad_t *kbd, char index, uint16_t *x, uint16_t *y);
#else   // HAVE_SE_TOUCH
void nbgl_keyboardCallback(nbgl_obj_t *obj, nbgl_buttonEvent_t buttonEvent);
void nbgl_keypadCallback(nbgl_obj_t *obj, nbgl_buttonEvent_t buttonEvent);
#endif  // HAVE_SE_TOUCH

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_OBJ_H */
