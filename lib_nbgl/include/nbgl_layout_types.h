/**
 * @file nbgl_layout_types.h
 * @brief Types of NBGL layouts also used by the use-case API
 *
 */

#ifndef NBGL_LAYOUT_TYPES_H
#define NBGL_LAYOUT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

#include "os_io_seph_cmd.h"

#include "nbgl_types.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief prototype of function to be called when an object is touched
 * @param token integer passed when registering callback
 * @param index when the object touched is a list of radio buttons, gives the index of the activated
 * button
 */
typedef void (*nbgl_layoutTouchCallback_t)(int token, uint8_t index);

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
    uint8_t  nbUsedButtons;  ///< the number of actually used (displayed) buttons
    uint16_t nbCandidates;   ///< total number of matching candidates, which may be greater than
                             ///< @ref nbUsedButtons when the displayed buttons are capped. On Nano
                             ///< it is used to decide when the whole candidate list fits in the
                             ///< selection page. If left to 0, @ref nbUsedButtons is used instead
                             ///< (legacy behaviour).
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
    bool                             obfuscated;
    bool    numbered;   ///< if set to true, the text is preceded on the left by 'number.'
    uint8_t number;     ///< if numbered is true, number used to build 'number.' text
    bool    grayedOut;  ///< (unused, kept for compatibility)
    int     textToken;  ///< the token that will be used as argument of the callback when "cross"
                        ///< button is touched (when text is not empty)
    union {
        nbgl_layoutSuggestionButtons_t
            suggestionButtons;  /// used if type is @ref KEYBOARD_WITH_SUGGESTIONS
        nbgl_layoutConfirmationButton_t
            confirmationButton;  /// used if type is @ref KEYBOARD_WITH_BUTTON
    };
#ifdef HAVE_SE_TOUCH
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
#endif
} nbgl_layoutKeyboardContent_t;

/**
 * @brief This structure contains info to build a left content area.
 *
 */
typedef struct {
    uint8_t                     nbRows;    ///< number of rows in the area
    const char                 *title;     ///< title of area in bold
    const char                **rowTexts;  ///< array of nbRows texts (displayed in regular)
    const nbgl_icon_details_t **rowIcons;  ///< array of nbRows icon
} nbgl_layoutLeftContent_t;

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

#endif  // HAVE_SE_TOUCH

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_LAYOUT_TYPES_H */
