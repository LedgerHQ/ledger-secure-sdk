/**
 * @file nbgl_layout.h
 * @brief API of the Advanced BOLOS Graphical Library, for predefined layouts
 *
 */

#ifndef NBGL_LAYOUT_H
#define NBGL_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nbgl_obj.h"
#include "nbgl_screen.h"
#include "nbgl_types.h"
#include "nbgl_content.h"
#ifdef HAVE_PIEZO_SOUND
#include "os_io_seproxyhal.h"
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
#define NBGL_NO_TUNE               NB_TUNES
#define NBGL_NO_PROGRESS_INDICATOR 0xFF

///< To be used when a control token shall not be used
#define NBGL_INVALID_TOKEN 0xFF

#ifdef HAVE_SE_TOUCH
///< special code used as index of action callback to inform when Exit key (X) is
///< pressed in the navigation bar
#define EXIT_PAGE 0xFF

#ifdef TARGET_STAX
#define NB_MAX_SUGGESTION_BUTTONS         12
// only 4 buttons are visible at the same time on Stax
#define NB_MAX_VISIBLE_SUGGESTION_BUTTONS 4
#define TOUCHABLE_HEADER_BAR_HEIGHT       88
#define TOUCHABLE_MAIN_BAR_HEIGHT         96
#define TOUCHABLE_BAR_HEIGHT              96
#define SMALL_FOOTER_HEIGHT               88
#define SIMPLE_FOOTER_HEIGHT              92
#define SMALL_CENTERING_HEADER            32
#define MEDIUM_CENTERING_HEADER           56
#define LONG_PRESS_BUTTON_HEIGHT          128
#else  // TARGET_STAX
#define NB_MAX_SUGGESTION_BUTTONS         8
// only 2 buttons are visible at the same time on Flex
#define NB_MAX_VISIBLE_SUGGESTION_BUTTONS 2
#define TOUCHABLE_HEADER_BAR_HEIGHT       96
#define TOUCHABLE_MAIN_BAR_HEIGHT         100
#define TOUCHABLE_BAR_HEIGHT              92
#define SMALL_FOOTER_HEIGHT               96
#define SIMPLE_FOOTER_HEIGHT              96
#define SMALL_CENTERING_HEADER            40
#define MEDIUM_CENTERING_HEADER           64
#define LONG_PRESS_BUTTON_HEIGHT          152
#endif  // TARGET_STAX

#define AVAILABLE_WIDTH (SCREEN_WIDTH - 2 * BORDER_MARGIN)

#define NB_MAX_LINES NB_MAX_LINES_IN_DETAILS

#else  // HAVE_SE_TOUCH
// 7 pixels on each side
#define AVAILABLE_WIDTH (SCREEN_WIDTH - 2 * 7)
// maximum number of lines in screen
#define NB_MAX_LINES    4

#endif  // HAVE_SE_TOUCH

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief type shared externally
 *
 */
typedef void *nbgl_layout_t;

/**
 * @brief prototype of function to be called when an object is touched
 * @param token integer passed when registering callback
 * @param index when the object touched is a list of radio buttons, gives the index of the activated
 * button
 */
typedef void (*nbgl_layoutTouchCallback_t)(int token, uint8_t index);

/**
 * @brief prototype of function to be called when buttons are touched on a screen
 * @param layout layout concerned by the event
 * @param event type of button event
 */
typedef void (*nbgl_layoutButtonCallback_t)(nbgl_layout_t *layout, nbgl_buttonEvent_t event);

/**
 * @brief This structure contains info to build a navigation bar at the bottom of the screen
 * @note this widget is incompatible with a footer.
 *
 */
typedef struct {
    uint8_t token;               ///< the token that will be used as argument of the callback
    uint8_t nbPages;             ///< number of pages. (if 0, no navigation)
    uint8_t activePage;          ///< index of active page (from 0 to nbPages-1).
    bool    withExitKey;         ///< if set to true, an exit button is drawn (X on the left)
    bool    withBackKey;         ///< if set to true, the "back" key is drawn
    bool    withSeparationLine;  ///< if set to true, an horizontal line is drawn on top of bar in
                                 ///< light gray
    bool withPageIndicator;  ///< on Flex, a "page on nb_pages" text can be added between back and
                             ///< forward keys
    bool visibleIndicator;   ///< on Flex, the page indicator can be visible or not.
                             ///< if withPageIndicator is true and this boolean false, the back key
                             ///< is placed as if there was an indicator
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when pressing keys)
#endif                    // HAVE_PIEZO_SOUND
} nbgl_layoutNavigationBar_t;

/**
 * @brief possible directions for Navigation arrows
 *
 */
typedef enum {
    HORIZONTAL_NAV,  ///< '<' and '>' are displayed, to navigate between pages and steps
    VERTICAL_NAV     ///< '\/' and '/\' are displayed, to navigate in a list (vertical scrolling)
} nbgl_layoutNavDirection_t;

/**
 * @brief possible styles for Navigation arrows (it's a bit field)
 *
 */
typedef enum {
    NO_ARROWS = 0,
    LEFT_ARROW,   ///< left arrow is used
    RIGHT_ARROW,  ///< right arrow is used
} nbgl_layoutNavIndication_t;

/**
 * @brief This structure contains info to build a navigation bar at the bottom of the screen
 * @note this widget is incompatible with a footer.
 *
 */
typedef struct {
    nbgl_layoutNavDirection_t  direction;   ///< vertical or horizontal navigation
    nbgl_layoutNavIndication_t indication;  ///< specifies which arrows to use (left or right)
} nbgl_layoutNavigation_t;

/**
 * @brief Structure containing all information when creating a layout. This structure must be passed
 * as argument to @ref nbgl_layoutGet
 * @note It shall not be used
 *
 */
typedef struct nbgl_layoutDescription_s {
    bool modal;  ///< if true, puts the layout on top of screen stack (modal). Otherwise puts on
                 ///< background (for apps)
#ifdef HAVE_SE_TOUCH
    bool withLeftBorder;  ///< if true, draws a light gray left border on the whole height of the
                          ///< screen
    const char *tapActionText;  ///< Light gray text used when main container is "tapable"
    uint8_t tapActionToken;     ///< the token that will be used as argument of the onActionCallback
                                ///< when main container is "tapped"
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tapTuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when tapping on
                             ///< main container
#endif                       // HAVE_PIEZO_SOUND
    nbgl_layoutTouchCallback_t
        onActionCallback;  ///< the callback to be called on any action on the layout
#else                      // HAVE_SE_TOUCH
    nbgl_layoutButtonCallback_t
            onActionCallback;  ///< the callback to be called on any action on the layout
#endif                     // HAVE_SE_TOUCH
    nbgl_screenTickerConfiguration_t ticker;  // configuration of ticker (timeout)
} nbgl_layoutDescription_t;

/**
 * @brief This structure contains info to build a clickable "bar" with a text and an icon
 *
 */
typedef struct {
    const nbgl_icon_details_t
               *iconLeft;  ///< a buffer containing the 1BPP icon for icon on left (can be NULL)
    const char *text;      ///< text (can be NULL)
    const nbgl_icon_details_t *iconRight;  ///< a buffer containing the 1BPP icon for icon 2 (can be
                                           ///< NULL). Dimensions must be the same as iconLeft
    const char *subText;                   ///< sub text (can be NULL)
    bool        large;                     ///< set to true only for the main level of OS settings
    uint8_t     token;     ///< the token that will be used as argument of the callback
    bool        inactive;  ///< if set to true, the bar is grayed-out and cannot be touched
    bool        centered;  ///< DEPRECATED, not used
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
#endif                    // HAVE_PIEZO_SOUND
} nbgl_layoutBar_t;

/**
 * @brief Deprecated, kept for retro compatibility
 *
 */
typedef nbgl_contentSwitch_t nbgl_layoutSwitch_t;

/**
 * @brief Deprecated, kept for retro compatibility
 */
typedef nbgl_contentRadioChoice_t nbgl_layoutRadioChoice_t;

/**
 * @brief prototype of menu list item retrieval callback
 * @param choiceIndex index of the menu list item to retrieve (from 0 (to nbChoices-1))
 * @return a pointer on a string
 */
typedef const char *(*nbgl_menuListCallback_t)(uint8_t choiceIndex);

/**
 * @brief This structure contains a list of names to build a menu list on Nanos, with for each item
 * a description (names array)
 */
typedef struct {
    nbgl_menuListCallback_t callback;        ///< function to call to retrieve a menu list item text
    uint8_t                 nbChoices;       ///< total number of choices in the menu list
    uint8_t                 selectedChoice;  ///< index of the selected choice (centered, in bold)

} nbgl_layoutMenuList_t;

/**
 * @brief Deprecated, kept for retro compatibility
 */
typedef nbgl_contentTagValue_t nbgl_layoutTagValue_t;

/**
 * @brief Deprecated, kept for retro compatibility
 */
typedef nbgl_contentTagValueList_t nbgl_layoutTagValueList_t;

/**
 * @brief Deprecated, kept for retro compatibility
 *
 */
typedef nbgl_contentCenteredInfo_t nbgl_layoutCenteredInfo_t;

#ifdef HAVE_SE_TOUCH

/**
 * @brief This structure contains info to build a centered (vertically and horizontally) area, with
 * a QR Code, a possible text (black, bold) under it, and a possible sub-text (black, regular) under
 * it.
 *
 */
typedef struct {
    const char *url;         ///< URL for QR code
    const char *text1;       ///< first text (can be null)
    const char *text2;       ///< second text (can be null)
    int16_t     offsetY;     ///< vertical shift to apply to this info (if > 0, shift to bottom)
    bool        centered;    ///< if set to true, center vertically
    bool        largeText1;  ///< if set to true, use 32px font for text1
} nbgl_layoutQRCode_t;

/**
 * @brief The different styles for a pair of buttons
 *
 */
typedef enum {
    ROUNDED_AND_FOOTER_STYLE
        = 0,            ///< A rounded black background full width button on top of a footer
    BOTH_ROUNDED_STYLE  ///< A rounded black background full width button on top of a rounded white
                        ///< background full width button
} nbgl_layoutChoiceButtonsStyle_t;

/**
 * @brief This structure contains info to build a pair of buttons, one on top of the other.
 *
 * @note the pair of button is automatically put on bottom of screen, in the footer
 */
typedef struct {
    const char *topText;     ///< up-button text (index 0)
    const char *bottomText;  ///< bottom-button text (index 1)
    uint8_t     token;       ///< the token that will be used as argument of the callback
    nbgl_layoutChoiceButtonsStyle_t style;  ///< the style of the pair
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
#endif                    // HAVE_PIEZO_SOUND
} nbgl_layoutChoiceButtons_t;

/**
 * @brief This structure contains info to build a pair of buttons, the small one, with icon, on the
 * left  of the other.
 *
 * @note the pair of button is automatically put on bottom of main container
 */
typedef struct {
    const nbgl_icon_details_t *leftIcon;    ///< a buffer containing the 1BPP icon for left button
    const char                *rightText;   ///< right-button text
    uint8_t                    leftToken;   ///< the token used when left button is pressed
    uint8_t                    rightToken;  ///< the token used when right button is pressed
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
#endif                    // HAVE_PIEZO_SOUND
} nbgl_layoutHorizontalButtons_t;

/**
 * @brief The different styles for a button
 *
 */
typedef enum {
    BLACK_BACKGROUND
        = 0,           ///< rounded bordered button, with text/icon in white, on black background
    WHITE_BACKGROUND,  ///< rounded bordered button, with text/icon in black, on white background
    NO_BORDER,         ///< simple clickable text, in black
    LONG_PRESS         ///< long press button, with progress indicator
} nbgl_layoutButtonStyle_t;

/**
 * @brief This structure contains info to build a single button
 */
typedef struct {
    const char                *text;   ///< button text
    const nbgl_icon_details_t *icon;   ///< a buffer containing the 1BPP icon for button
    uint8_t                    token;  ///< the token that will be used as argument of the callback
    nbgl_layoutButtonStyle_t   style;
    bool fittingContent;  ///< if set to true, fit the width of button to text, otherwise full width
    bool onBottom;        ///< if set to true, align on bottom of page, otherwise put on bottom of
                          ///< previous object
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
#endif                    // HAVE_PIEZO_SOUND
} nbgl_layoutButton_t;

/**
 * @brief The different types of keyboard contents
 *
 */
typedef enum {
    KEYBOARD_WITH_SUGGESTIONS,  ///< text entry area + suggestion buttons
    KEYBOARD_WITH_BUTTON,       ///< text entry area + confirmation button
    NB_KEYBOARD_CONTENT_TYPES
} nbgl_layoutKeyboardContentType_t;

/**
 * @brief This structure contains info to build suggestion buttons
 */
typedef struct {
    const char **buttons;  ///< array of 4 strings for buttons (last ones can be NULL)
    int firstButtonToken;  ///< first token used for buttons, provided in onActionCallback (the next
                           ///< 3 values will be used for other buttons)
    uint8_t nbUsedButtons;  ///< the number of actually used buttons
} nbgl_layoutSuggestionButtons_t;

/**
 * @brief This structure contains info to build a confirmation button
 */
typedef struct {
    const char *text;    ///< text of the button
    int         token;   ///< token of the button
    bool        active;  ///< if true, button is active, otherwise inactive (grayed-out)
} nbgl_layoutConfirmationButton_t;

/**
 * @brief This structure contains info to build a keyboard content (controls that are linked to
 * keyboard)
 */
typedef struct {
    nbgl_layoutKeyboardContentType_t type;   ///< type of content
    const char                      *title;  ///< centered title explaining the screen
    const char                      *text;   ///< already entered text
    bool    numbered;   ///< if set to true, the text is preceded on the left by 'number.'
    uint8_t number;     ///< if numbered is true, number used to build 'number.' text
    bool    grayedOut;  ///< if true, the text is grayed out (but not the potential number)
    int     textToken;  ///< the token that will be used as argument of the callback when text is
                        ///< touched
    union {
        nbgl_layoutSuggestionButtons_t
            suggestionButtons;  /// used if type is @ref KEYBOARD_WITH_SUGGESTIONS
        nbgl_layoutConfirmationButton_t
            confirmationButton;  /// used if type is @ref KEYBOARD_WITH_BUTTON
    };
#ifdef HAVE_PIEZO_SOUND
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
#endif                    // HAVE_PIEZO_SOUND
} nbgl_layoutKeyboardContent_t;

/**
 * @brief The different types of extended header
 *
 */
typedef enum {
    HEADER_EMPTY = 0,      ///< empty space, to have a better vertical centering of centered info
    HEADER_BACK_AND_TEXT,  ///< back key and optional text
    HEADER_BACK_AND_PROGRESS,  ///< optional back key and progress indicator (only on Stax)
    HEADER_TITLE,              ///< simple centered text
    HEADER_EXTENDED_BACK,      ///< back key, centered text and touchable key on the right
    HEADER_RIGHT_TEXT,         ///< touchable text on the right, with a vertical separation line
    NB_HEADER_TYPES
} nbgl_layoutHeaderType_t;

/**
 * @brief This structure contains info to build a header.
 *
 */
typedef struct {
    nbgl_layoutHeaderType_t type;  ///< type of header
    bool separationLine;  ///< if true, a separation line is added at the bottom of this control
    union {
        struct {
            uint16_t height;
        } emptySpace;  ///< if type is @ref HEADER_EMPTY
        struct {
            const char  *text;    ///< can be NULL if no text
            uint8_t      token;   ///< when back key is pressed
            tune_index_e tuneId;  ///< when back key is pressed
        } backAndText;            ///< if type is @ref HEADER_BACK_AND_TEXT
        struct {
            const nbgl_icon_details_t *actionIcon;  ///< right button icon
            uint8_t                    activePage;
            uint8_t                    nbPages;
            bool                       withBack;
            uint8_t                    token;        ///< when optional back key is pressed
            uint8_t                    actionToken;  ///< when optional right button is pressed
            tune_index_e               tuneId;       ///< when optional back key is pressed
        } progressAndBack;                           ///< if type is @ref HEADER_BACK_AND_PROGRESS
        struct {
            const char *text;
        } title;  ///< if type is @ref HEADER_TITLE
        struct {
            const nbgl_icon_details_t *actionIcon;   ///< right button icon
            const char                *text;         ///< centered text (can be NULL if no text)
            uint8_t                    textToken;    ///< when text is touched
            uint8_t                    backToken;    ///< when back key is pressed
            uint8_t                    actionToken;  ///< when right key is pressed
            tune_index_e               tuneId;       ///< when back key is pressed
        } extendedBack;                              ///< if type is @ref HEADER_EXTENDED_BACK
        struct {
            const char  *text;    ///< touchable text on the right
            uint8_t      token;   ///< when text is pressed
            tune_index_e tuneId;  ///< when text is pressed
        } rightText;              ///< if type is @ref HEADER_RIGHT_TEXT
    };
} nbgl_layoutHeader_t;

/**
 * @brief The different types of extended footer
 *
 */
typedef enum {
    FOOTER_EMPTY = 0,    ///< empty space, to have a better vertical centering of centered info
    FOOTER_SIMPLE_TEXT,  ///< simple touchable text in bold
    FOOTER_DOUBLE_TEXT,  ///< 2 touchable texts in bold, separated by a vertical line (only on Stax)
    FOOTER_TEXT_AND_NAV,   ///< touchable text in bold on the left, navigation on the right (only on
                           ///< Flex)
    FOOTER_NAV,            ///< navigation bar
    FOOTER_SIMPLE_BUTTON,  ///< simple black or white button (see @ref nbgl_layoutButtonStyle_t)
    FOOTER_CHOICE_BUTTONS,  ///< double buttons (see @ref nbgl_layoutChoiceButtonsStyle_t)
    NB_FOOTER_TYPES
} nbgl_layoutFooterType_t;

/**
 * @brief This structure contains info to build an extended footer.
 *
 */
typedef struct {
    nbgl_layoutFooterType_t type;  ///< type of footer
    bool separationLine;  ///< if true, a separation line is added at the top of this control
    union {
        struct {
            uint16_t height;
        } emptySpace;  ///< if type is @ref FOOTER_EMPTY
        struct {
            const char  *text;
            bool         mutedOut;  ///< if true, text is displayed in gray
            uint8_t      token;
            tune_index_e tuneId;
        } simpleText;  ///< if type is @ref FOOTER_SIMPLE_TEXT
        struct {
            const char  *leftText;
            const char  *rightText;
            uint8_t      leftToken;
            uint8_t      rightToken;
            tune_index_e tuneId;
        } doubleText;  ///< if type is @ref FOOTER_DOUBLE_TEXT
        struct {
            nbgl_layoutNavigationBar_t navigation;
            const char                *text;
            uint8_t                    token;
            tune_index_e               tuneId;
        } textAndNav;                              ///< if type is @ref FOOTER_TEXT_AND_NAV
        nbgl_layoutNavigationBar_t navigation;     ///< if type is @ref FOOTER_NAV
        nbgl_layoutButton_t        button;         ///< if type is @ref FOOTER_SIMPLE_BUTTON
        nbgl_layoutChoiceButtons_t choiceButtons;  ///< if type is @ref FOOTER_SIMPLE_BUTTON
    };
} nbgl_layoutFooter_t;

/**
 * @brief The different types of area on top of footer
 *
 */
typedef enum {
    UP_FOOTER_LONG_PRESS = 0,      ///< long-press button
    UP_FOOTER_BUTTON,              ///< simple button
    UP_FOOTER_HORIZONTAL_BUTTONS,  ///< 2 buttons, on the same line
    UP_FOOTER_TIP_BOX,             ///< Tip-box
    NB_UP_FOOTER_TYPES
} nbgl_layoutUpFooterType_t;

/**
 * @brief This structure contains info to build an up-footer (area on top of footer).
 *
 */
typedef struct {
    nbgl_layoutUpFooterType_t type;  ///< type of up-footer
    union {
        struct {
            const char  *text;       ///< text in the long-press button
            uint8_t      token;      ///< token used when button is long-pressed
            tune_index_e tuneId;     ///< tune played when button is long-pressed
        } longPress;                 ///< if type is @ref UP_FOOTER_LONG_PRESS
        nbgl_layoutButton_t button;  ///< if type is @ref UP_FOOTER_BUTTON
        nbgl_layoutHorizontalButtons_t
                             horizontalButtons;  ///< if type is @ref UP_FOOTER_HORIZONTAL_BUTTONS
        nbgl_contentTipBox_t tipBox;             ///< if type is @ref UP_FOOTER_TIP_BOX
    };
} nbgl_layoutUpFooter_t;
#endif  // HAVE_SE_TOUCH

/**
 * @brief This structure contains info to build a progress bar with info. The progress bar itself is
 * 120px width * 12px height
 *
 */
typedef struct {
    uint8_t     percentage;  ///< percentage of completion, from 0 to 100.
    const char *text;        ///< text in black, on top of progress bar
    const char *subText;     ///< text in gray, under progress bar
} nbgl_layoutProgressBar_t;

/**
 * @brief This structure contains info to build a keyboard with @ref nbgl_layoutAddKeyboard()
 *
 */
typedef struct {
    uint32_t keyMask;  ///< mask used to disable some keys in letters only mod. The 26 LSB bits of
                       ///< mask are used, for the 26 letters of a QWERTY keyboard. Bit[0] for Q,
                       ///< Bit[1] for W and so on
    keyboardCallback_t callback;     ///< function called when an active key is pressed
    bool               lettersOnly;  ///< if true, only display letter keys and Backspace
    keyboardMode_t     mode;         ///< keyboard mode to start with
#ifdef HAVE_SE_TOUCH
    keyboardCase_t casing;  ///< keyboard casing mode (lower, upper once or upper locked)
#else                       // HAVE_SE_TOUCH
    bool    enableBackspace;   ///< if true, Backspace key is enabled
    bool    enableValidate;    ///< if true, Validate key is enabled
    uint8_t selectedCharIndex;
#endif                      // HAVE_SE_TOUCH
} nbgl_layoutKbd_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
nbgl_layout_t *nbgl_layoutGet(const nbgl_layoutDescription_t *description);
int nbgl_layoutAddCenteredInfo(nbgl_layout_t *layout, const nbgl_layoutCenteredInfo_t *info);
int nbgl_layoutAddContentCenter(nbgl_layout_t *layout, const nbgl_contentCenter_t *info);
int nbgl_layoutAddProgressBar(nbgl_layout_t *layout, const nbgl_layoutProgressBar_t *barLayout);

#ifdef HAVE_SE_TOUCH
int nbgl_layoutAddTopRightButton(nbgl_layout_t             *layout,
                                 const nbgl_icon_details_t *icon,
                                 uint8_t                    token,
                                 tune_index_e               tuneId);
int nbgl_layoutAddTouchableBar(nbgl_layout_t *layout, const nbgl_layoutBar_t *barLayout);
int nbgl_layoutAddSwitch(nbgl_layout_t *layout, const nbgl_layoutSwitch_t *switchLayout);
int nbgl_layoutAddText(nbgl_layout_t *layout, const char *text, const char *subText);
int nbgl_layoutAddSubHeaderText(nbgl_layout_t *layout, const char *text);
int nbgl_layoutAddRadioChoice(nbgl_layout_t *layout, const nbgl_layoutRadioChoice_t *choices);
int nbgl_layoutAddQRCode(nbgl_layout_t *layout, const nbgl_layoutQRCode_t *info);
int nbgl_layoutAddChoiceButtons(nbgl_layout_t *layout, const nbgl_layoutChoiceButtons_t *info);
int nbgl_layoutAddHorizontalButtons(nbgl_layout_t                        *layout,
                                    const nbgl_layoutHorizontalButtons_t *info);
int nbgl_layoutAddTagValueList(nbgl_layout_t *layout, const nbgl_layoutTagValueList_t *list);
int nbgl_layoutAddLargeCaseText(nbgl_layout_t *layout, const char *text);
int nbgl_layoutAddTextContent(nbgl_layout_t *layout,
                              const char    *title,
                              const char    *description,
                              const char    *info);
int nbgl_layoutAddSeparationLine(nbgl_layout_t *layout);

int nbgl_layoutAddButton(nbgl_layout_t *layout, const nbgl_layoutButton_t *buttonInfo);
int nbgl_layoutAddLongPressButton(nbgl_layout_t *layout,
                                  const char    *text,
                                  uint8_t        token,
                                  tune_index_e   tuneId);
int nbgl_layoutAddFooter(nbgl_layout_t *layout,
                         const char    *text,
                         uint8_t        token,
                         tune_index_e   tuneId);
int nbgl_layoutAddSplitFooter(nbgl_layout_t *layout,
                              const char    *leftText,
                              uint8_t        leftToken,
                              const char    *rightText,
                              uint8_t        rightToken,
                              tune_index_e   tuneId);
int nbgl_layoutAddHeader(nbgl_layout_t *layout, const nbgl_layoutHeader_t *headerDesc);
int nbgl_layoutAddExtendedFooter(nbgl_layout_t *layout, const nbgl_layoutFooter_t *footerDesc);
int nbgl_layoutAddUpFooter(nbgl_layout_t *layout, const nbgl_layoutUpFooter_t *upFooterDesc);
int nbgl_layoutAddNavigationBar(nbgl_layout_t *layout, const nbgl_layoutNavigationBar_t *info);
int nbgl_layoutAddBottomButton(nbgl_layout_t             *layout,
                               const nbgl_icon_details_t *icon,
                               uint8_t                    token,
                               bool                       separationLine,
                               tune_index_e               tuneId);
int nbgl_layoutAddProgressIndicator(nbgl_layout_t *layout,
                                    uint8_t        activePage,
                                    uint8_t        nbPages,
                                    bool           withBack,
                                    uint8_t        backToken,
                                    tune_index_e   tuneId);
int nbgl_layoutAddSpinner(nbgl_layout_t *layout, const char *text, bool fixed);
int nbgl_layoutAddSwipe(nbgl_layout_t *layout,
                        uint16_t       swipesMask,
                        const char    *text,
                        uint8_t        token,
                        tune_index_e   tuneId);
#else   // HAVE_SE_TOUCH
int nbgl_layoutAddText(nbgl_layout_t                  *layout,
                       const char                     *text,
                       const char                     *subText,
                       nbgl_contentCenteredInfoStyle_t style);
int nbgl_layoutAddNavigation(nbgl_layout_t *layout, nbgl_layoutNavigation_t *info);
int nbgl_layoutAddMenuList(nbgl_layout_t *layout, nbgl_layoutMenuList_t *list);
#endif  // HAVE_SE_TOUCH

#ifdef NBGL_KEYBOARD
/* layout objects for page with keyboard */
int nbgl_layoutAddKeyboard(nbgl_layout_t *layout, const nbgl_layoutKbd_t *kbdInfo);
#ifdef HAVE_SE_TOUCH
int            nbgl_layoutUpdateKeyboard(nbgl_layout_t *layout,
                                         uint8_t        index,
                                         uint32_t       keyMask,
                                         bool           updateCasing,
                                         keyboardCase_t casing);
bool           nbgl_layoutKeyboardNeedsRefresh(nbgl_layout_t *layout, uint8_t index);
DEPRECATED int nbgl_layoutAddSuggestionButtons(nbgl_layout_t *layout,
                                               uint8_t        nbUsedButtons,
                                               const char  *buttonTexts[NB_MAX_SUGGESTION_BUTTONS],
                                               int          firstButtonToken,
                                               tune_index_e tuneId);
DEPRECATED int nbgl_layoutUpdateSuggestionButtons(
    nbgl_layout_t *layout,
    uint8_t        index,
    uint8_t        nbUsedButtons,
    const char    *buttonTexts[NB_MAX_SUGGESTION_BUTTONS]);
DEPRECATED int nbgl_layoutAddEnteredText(nbgl_layout_t *layout,
                                         bool           numbered,
                                         uint8_t        number,
                                         const char    *text,
                                         bool           grayedOut,
                                         int            offsetY,
                                         int            token);
DEPRECATED int nbgl_layoutUpdateEnteredText(nbgl_layout_t *layout,
                                            uint8_t        index,
                                            bool           numbered,
                                            uint8_t        number,
                                            const char    *text,
                                            bool           grayedOut);
DEPRECATED int nbgl_layoutAddConfirmationButton(nbgl_layout_t *layout,
                                                bool           active,
                                                const char    *text,
                                                int            token,
                                                tune_index_e   tuneId);
DEPRECATED int nbgl_layoutUpdateConfirmationButton(nbgl_layout_t *layout,
                                                   uint8_t        index,
                                                   bool           active,
                                                   const char    *text);
int nbgl_layoutAddKeyboardContent(nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content);
int nbgl_layoutUpdateKeyboardContent(nbgl_layout_t *layout, nbgl_layoutKeyboardContent_t *content);
#else   // HAVE_SE_TOUCH
int nbgl_layoutUpdateKeyboard(nbgl_layout_t *layout, uint8_t index, uint32_t keyMask);
int nbgl_layoutAddEnteredText(nbgl_layout_t *layout, const char *text, bool lettersOnly);
int nbgl_layoutUpdateEnteredText(nbgl_layout_t *layout, uint8_t index, const char *text);
#endif  // HAVE_SE_TOUCH
#endif  // NBGL_KEYBOARD

#ifdef NBGL_KEYPAD
#ifdef HAVE_SE_TOUCH
/* layout objects for page with keypad (Stax) */
int nbgl_layoutAddKeypad(nbgl_layout_t *layout, keyboardCallback_t callback, bool shuffled);
int nbgl_layoutUpdateKeypad(nbgl_layout_t *layout,
                            uint8_t        index,
                            bool           enableValidate,
                            bool           enableBackspace,
                            bool           enableDigits);
DEPRECATED int nbgl_layoutAddHiddenDigits(nbgl_layout_t *layout, uint8_t nbDigits);
DEPRECATED int nbgl_layoutUpdateHiddenDigits(nbgl_layout_t *layout,
                                             uint8_t        index,
                                             uint8_t        nbActive);
int            nbgl_layoutAddKeypadContent(nbgl_layout_t *layout,
                                           const char    *title,
                                           bool           hidden,
                                           uint8_t        nbDigits,
                                           const char    *text);
int            nbgl_layoutUpdateKeypadContent(nbgl_layout_t *layout,
                                              bool           hidden,
                                              uint8_t        nbActiveDigits,
                                              const char    *text);

#else   // HAVE_SE_TOUCH
/* layout objects for pages with keypad (nanos) */
int nbgl_layoutAddKeypad(nbgl_layout_t     *layout,
                         keyboardCallback_t callback,
                         const char        *text,
                         bool               shuffled);
int nbgl_layoutUpdateKeypad(nbgl_layout_t *layout,
                            uint8_t        index,
                            bool           enableValidate,
                            bool           enableBackspace);
int nbgl_layoutAddHiddenDigits(nbgl_layout_t *layout, uint8_t nbDigits);
int nbgl_layoutUpdateHiddenDigits(nbgl_layout_t *layout, uint8_t index, uint8_t nbActive);
#endif  // HAVE_SE_TOUCH
#endif  // NBGL_KEYPAD

/* generic functions */
int nbgl_layoutDraw(nbgl_layout_t *layout);
int nbgl_layoutRelease(nbgl_layout_t *layout);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_LAYOUT_H */
