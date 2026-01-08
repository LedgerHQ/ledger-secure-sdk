/**
 * @file nbgl_content.h
 * @brief common content for Graphical Library
 *
 */

#ifndef NBGL_CONTENT_H
#define NBGL_CONTENT_H

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
#include "nbgl_obj.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief possible styles for Centered Info Area
 *
 */
typedef enum {
#ifdef HAVE_SE_TOUCH
    LARGE_CASE_INFO,  ///< text in BLACK and large case (INTER 32px), subText in black in Inter24px
    LARGE_CASE_BOLD_INFO,  ///< text in BLACK and large case (INTER 32px), subText in black bold
                           ///< Inter24px, text3 in black Inter24px
    LARGE_CASE_GRAY_INFO,  ///< text in BLACK and large case (INTER 32px), subText in black
                           ///< Inter24px text3 in dark gray Inter24px
    NORMAL_INFO,  ///< Icon in black, a potential text in black bold 24px under it, a potential text
                  ///< in dark gray (24px) under it, a potential text in black (24px) under it
    PLUGIN_INFO   ///< A potential text in black 32px, a potential text in black (24px) under it, a
                 ///< small horizontal line under it, a potential icon under it, a potential text in
                 ///< black (24px) under it
#else   // HAVE_SE_TOUCH
    REGULAR_INFO = 0,  ///< both texts regular (but '\\b' can switch to bold)
    BOLD_TEXT1_INFO,   ///< bold is used for text1 (but '\\b' can switch to regular)
    BUTTON_INFO        ///< bold is used for text1 and text2 as a white button
#endif  // HAVE_SE_TOUCH
} nbgl_contentCenteredInfoStyle_t;

/**
 * @brief This structure contains info to build a centered (vertically and horizontally) area, with
 * a possible Icon, a possible text under it, and a possible sub-text gray under it.
 *
 */
typedef struct {
    const char *text1;  ///< first text (can be null)
    const char *text2;  ///< second text (can be null)
#ifdef HAVE_SE_TOUCH
    const char *text3;                      ///< third text (can be null)
#endif                                      // HAVE_SE_TOUCH
    const nbgl_icon_details_t      *icon;   ///< a buffer containing the 1BPP icon
    const nbgl_icon_details_t      *icon2;   ///< a buffer containing the 1BPP icon
    const nbgl_icon_details_t      *icon3;   ///< a buffer containing the 1BPP icon
    const nbgl_icon_details_t      *icon4;   ///< a buffer containing the 1BPP icon
    bool                            onTop;  ///< if set to true, align only horizontally
    nbgl_contentCenteredInfoStyle_t style;  ///< style to apply to this info
#ifdef HAVE_SE_TOUCH
    int16_t offsetY;  ///< vertical shift to apply to this info (if >0, shift to bottom)
#endif                // HAVE_SE_TOUCH
} nbgl_contentCenteredInfo_t;

/**
 * @brief possible types of illustration, for example in @ref nbgl_contentCenter_t
 *
 */
typedef enum {
    ICON_ILLUSTRATION,  ///< simple icon
    ANIM_ILLUSTRATION,  ///< animation
} nbgl_contentIllustrationType_t;

/**
 * @brief This structure contains info to build a centered (vertically and horizontally) area, with
 * many fields (if NULL, not used):
 *  - an icon (with a possible hug)
 *  - a title in black large case
 *  - a sub-title in black small bold case
 *  - a description in black small regular case
 *  - a sub-text in dark gray regular small case
 *  - a padding on the bottom
 */
typedef struct {
    nbgl_contentIllustrationType_t illustrType;
    const nbgl_icon_details_t     *icon;  ///< the icon (can be null)
    const nbgl_icon_details_t     *icon2;  ///< the icon (can be null)
    const nbgl_icon_details_t     *icon3;  ///< the icon (can be null)
    const nbgl_icon_details_t     *icon4;  ///< the icon (can be null)
    const nbgl_animation_t
        *animation;  ///< the animation (can be null), used if illustrType is @ref ANIM_ILLUSTRATION
    uint16_t animOffsetX;  ///< horizontal offset of animation in icon if integrated (but usually 0)
    uint16_t animOffsetY;  ///< vertical offset of animation in icon if integrated (but usually 0)
    const char *title;     ///< title in black large (can be null)
    const char *smallTitle;   ///< sub-title in black small bold case (can be null)
    const char *description;  ///< description in black small regular case (can be null)
    const char *subText;      ///< sub-text in dark gray regular small case
    uint16_t    iconHug;      ///< vertical margin to apply on top and bottom of the icon
    bool        padding;      ///< if true, apply a padding of 40px at the bottom
} nbgl_contentCenter_t;

/**
 * @brief This structure contains data to build a centered info + long press button content
 */
typedef struct {
    const char                *text;           ///< centered text in large case
    const nbgl_icon_details_t *icon;           ///< a buffer containing the 1BPP icon
    const char                *longPressText;  ///< text of the long press button
    uint8_t longPressToken;  ///< the token used as argument of the onActionCallback when button is
                             ///< long pressed
    tune_index_e
        tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when button is touched
} nbgl_contentInfoLongPress_t;

/**
 * @brief This structure contains data to build a centered info + simple black button content
 */
typedef struct {
    const char                *text;        ///< centered text in large case
    const nbgl_icon_details_t *icon;        ///< a buffer containing the 1BPP icon
    const char                *buttonText;  ///< text of the long press button
    uint8_t buttonToken;  ///< the token used as argument of the onActionCallback when button is
                          ///< long pressed
    tune_index_e
        tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when button is touched
} nbgl_contentInfoButton_t;

/**
 * @brief possible types of value alias
 *
 */
typedef enum {
    NO_ALIAS_TYPE = 0,
    ENS_ALIAS,           ///< alias comes from ENS
    ADDRESS_BOOK_ALIAS,  ///< alias comes from Address Book
    QR_CODE_ALIAS,       ///< alias is an address to be displayed as a QR Code
    INFO_LIST_ALIAS,     ///< alias is list of infos
    TAG_VALUE_LIST_ALIAS
} nbgl_contentValueAliasType_t;

/**
 * @brief This structure contains additions to a tag/value pair,
 *        to be able to build a screen to display these additions (for alias)
 */
typedef struct {
    const char *fullValue;    ///< full string of the value when used as an alias
    const char *explanation;  ///< string displayed in gray, explaing where the alias comes from
                              ///< only used if aliasType is @ref NO_ALIAS_TYPE
    const char *title;  ///< if not NULL and aliasType is @ref QR_CODE_ALIAS, is used as title of
                        ///< the QR Code
    const char
        *backText;  ///< used as title of the popping page, if not NULL, otherwise "item" is used
    union {
        const struct nbgl_contentInfoList_s *infolist;  ///< if aliasType is INFO_LIST_ALIAS
        const struct nbgl_contentTagValueList_s
            *tagValuelist;  ///< if aliasType is TAG_VALUE_LIST_ALIAS
    };
    nbgl_contentValueAliasType_t aliasType;  ///< type of alias
} nbgl_contentValueExt_t;

/**
 * @brief This structure contains a [tag,value] pair and possible extensions
 */
typedef struct {
    const char *item;   ///< string giving the tag name
    const char *value;  ///< string giving the value name
    union {
        const nbgl_icon_details_t
            *valueIcon;  ///< - if alias, a buffer containing the 1BPP icon for icon
                         ///< on right of value, on Stax/Flex (can be NULL)
                         ///< - if centeredInfo, it's the icon of the centered info (can be NULL)
        const nbgl_contentValueExt_t
            *extension;  ///< if not NULL, gives additional info on value field
    };
    int8_t forcePageStart : 1;  ///< if set to 1, the tag will be displayed at the top of a new
                                ///< review page
    int8_t centeredInfo : 1;    ///< if set to 1, the tag will be displayed as a centered info
    int8_t aliasValue : 1;      ///< if set to 1, the value represents an alias
                                ///< - On wallet size, a > icon enables to
                                ///< view the full value (extension field in union)
                                ///< - On Nano, the value is displayed in white
} nbgl_contentTagValue_t;

/**
 * @brief prototype of tag/value pair retrieval callback
 * @param pairIndex index of the tag/value pair to retrieve (from 0 (to nbPairs-1))
 * @return a pointer on a static tag/value pair
 */
typedef nbgl_contentTagValue_t *(*nbgl_contentTagValueCallback_t)(uint8_t pairIndex);

/**
 * @brief prototype of function to be called when an action on a content object occurs
 * @param token integer passed at content object initialization
 * @param index when the object touched is a list of radio buttons, gives the index of the activated
 * @param page index of the current page, can be used to restart the use_case directly at the right
 * page button
 */
typedef void (*nbgl_contentActionCallback_t)(int token, uint8_t index, int page);

/**
 * @brief This structure contains a list of [tag,value] pairs
 */
typedef struct nbgl_contentTagValueList_s {
    const nbgl_contentTagValue_t
        *pairs;  ///< array of [tag,value] pairs (nbPairs items). If NULL, callback is used instead
    nbgl_contentTagValueCallback_t callback;  ///< function to call to retrieve a given pair
    uint8_t nbPairs;  ///< number of pairs in pairs array (or max number of pairs to retrieve with
                      ///< callback)
    uint8_t startIndex;          ///< index of the first pair to get with callback
    bool    hideEndOfLastLine;   ///< if set to true, replace 3 last chars of last line by "..."
    uint8_t nbMaxLinesForValue;  ///< if > 0, set the max number of lines for value field.
    uint8_t token;  ///< the token that will be used as argument of the callback if icon in any
                    ///< tag/value pair is touched (index is the index of the pair in pairs[])
    bool smallCaseForValue;  ///< if set to true, a 24px font is used for value text, otherwise a
                             ///< 32px font is used
    bool wrapping;  ///< if set to true, value text will be wrapped on ' ' to avoid cutting words
    nbgl_contentActionCallback_t
        actionCallback;  ///< called when a valueIcon is touched on a given pair
} nbgl_contentTagValueList_t;

/**
 * @brief This structure contains a [item,value] pair and info about "details" button
 */
typedef struct {
    nbgl_contentTagValueList_t tagValueList;       ///< list of tag/value pairs
    const nbgl_icon_details_t *detailsButtonIcon;  ///< icon to use in details button
    const char                *detailsButtonText;  ///< this text is used for "details" button
    uint8_t detailsButtonToken;  ///< the token used as argument of the actionCallback when the
                                 ///< "details" button is touched
    tune_index_e
        tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when details button is touched
} nbgl_contentTagValueDetails_t;

/**
 * @brief This structure contains [item,value] pair(s) and info about a potential "details" button,
 * but also a black button + footer to confirm/cancel
 */
typedef struct {
    nbgl_contentTagValueList_t tagValueList;       ///< list of tag/value pairs
    const nbgl_icon_details_t *detailsButtonIcon;  ///< icon to use in details button
    const char *detailsButtonText;  ///< this text is used for "details" button (if NULL, no button)
    uint8_t     detailsButtonToken;  ///< the token used as argument of the actionCallback when the
                                     ///< "details" button is touched
    tune_index_e
        tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when details button is touched
    const char
        *confirmationText;  ///< text of the confirmation button, if NULL "It matches" is used
    const char
           *cancelText;  ///< the text used for cancel action, if NULL "It doesn't matches" is used
    uint8_t confirmationToken;  ///< the token used as argument of the onActionCallback
    uint8_t cancelToken;  ///< the token used as argument of the onActionCallback when the cancel
                          ///< button is pressed
} nbgl_contentTagValueConfirm_t;

/**
 * @brief This structure contains info to build a switch (on the right) with a description (on the
 * left), with a potential sub-description (in gray)
 *
 */
typedef struct {
    const char *text;  ///< main text for the switch
    const char
        *subText;  ///< description under main text (NULL terminated, single line, may be null)
    nbgl_state_t initState;  ///< initial state of the switch
    uint8_t token;  ///< the token that will be used as argument of the callback (unused on Nano)
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played
} nbgl_contentSwitch_t;

/**
 * @brief This structure contains data to build a @ref SWITCHES_LIST content
 */
typedef struct nbgl_pageSwitchesList_s {
    const nbgl_contentSwitch_t *switches;    ///< array of switches (nbSwitches items)
    uint8_t                     nbSwitches;  ///< number of elements in switches and tokens array
} nbgl_contentSwitchesList_t;

/**
 * @brief This structure contains data to build a @ref INFOS_LIST content
 */
typedef struct nbgl_contentInfoList_s {
    const char *const *infoTypes;     ///< array of types of infos (in black/bold)
    const char *const *infoContents;  ///< array of contents of infos (in black)
    const nbgl_contentValueExt_t
        *infoExtensions;  ///< if not NULL, gives additional info on type field
                          ///< any {0} element of this array is considered as invalid
    uint8_t nbInfos;      ///< number of elements in infoTypes and infoContents array
    uint8_t token;  ///< token to use with extensions, if withExtensions is true and infoExtensions
                    ///< is not NULL
    bool withExtensions;  /// if set to TRUE and if infoExtensions is not NULL, use this
                          /// infoExtensions field
} nbgl_contentInfoList_t;

/**
 * @brief This structure contains a list of names to build a list of radio
 * buttons (on the right part of screen), with for each a description (names array)
 * The chosen item index is provided is the "index" argument of the callback
 */
typedef struct {
    union {
        const char *const *names;  ///< array of strings giving the choices (nbChoices)
#if defined(HAVE_LANGUAGE_PACK)
        UX_LOC_STRINGS_INDEX *nameIds;  ///< array of string Ids giving the choices (nbChoices)
#endif                                  // HAVE_LANGUAGE_PACK
    };
    bool    localized;   ///< if set to true, use nameIds and not names
    uint8_t nbChoices;   ///< number of choices
    uint8_t initChoice;  ///< index of the current choice
    uint8_t token;       ///< the token that will be used as argument of the callback
    tune_index_e
        tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when selecting a radio button)

} nbgl_contentRadioChoice_t;

/**
 * @brief This structure contains data to build a @ref BARS_LIST content
 */
typedef struct {
    const char *const *barTexts;  ///< array of texts for each bar (nbBars items, in black/bold)
    const uint8_t     *tokens;    ///< array of tokens, one for each bar (nbBars items)
    uint8_t            nbBars;    ///< number of elements in barTexts and tokens array
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when a bar is touched
} nbgl_contentBarsList_t;

/**
 * @brief This structure contains data to build a tip-box, on top of a footer,
 * on bottom of a content center
 */
typedef struct {
    const char                *text;    ///< text of the tip-box
    const nbgl_icon_details_t *icon;    ///< icon of the tip-box
    uint8_t                    token;   ///< token used when tip-box is tapped
    tune_index_e               tuneId;  ///< tune played when tip-box is tapped
} nbgl_contentTipBox_t;

/**
 * @brief This structure contains data to build a @ref EXTENDED_CENTER content
 */
typedef struct {
    nbgl_contentCenter_t contentCenter;  ///< centered content (icon + text(s))
    nbgl_contentTipBox_t tipBox;         ///< if text field is NULL, no tip-box
} nbgl_contentExtendedCenter_t;

/**
 * @brief The different types of predefined contents
 *
 */
typedef enum {
    CENTERED_INFO = 0,  ///< a centered info
    EXTENDED_CENTER,    ///< a centered content and a possible tip-box
    INFO_LONG_PRESS,    ///< a centered info and a long press button
    INFO_BUTTON,        ///< a centered info and a simple black button
    TAG_VALUE_LIST,     ///< list of tag/value pairs
    TAG_VALUE_DETAILS,  ///< a tag/value pair and a small button to get details.
    TAG_VALUE_CONFIRM,  ///< tag/value pairs and a black button/footer to confirm/cancel.
    SWITCHES_LIST,      ///< list of switches with descriptions
    INFOS_LIST,         ///< list of infos with titles
    CHOICES_LIST,       ///< list of choices through radio buttons
    BARS_LIST           ///< list of touchable bars (with > on the right to go to sub-pages)
} nbgl_contentType_t;

/**
 * @brief Union of the different type of contents
 */
typedef union {
    nbgl_contentCenteredInfo_t    centeredInfo;     ///< @ref CENTERED_INFO type
    nbgl_contentExtendedCenter_t  extendedCenter;   ///< @ref EXTENDED_CENTER type
    nbgl_contentInfoLongPress_t   infoLongPress;    ///< @ref INFO_LONG_PRESS type
    nbgl_contentInfoButton_t      infoButton;       ///< @ref INFO_BUTTON type
    nbgl_contentTagValueList_t    tagValueList;     ///< @ref TAG_VALUE_LIST type
    nbgl_contentTagValueDetails_t tagValueDetails;  ///< @ref TAG_VALUE_DETAILS type
    nbgl_contentTagValueConfirm_t tagValueConfirm;  ///< @ref TAG_VALUE_CONFIRM type
    nbgl_contentSwitchesList_t    switchesList;     ///< @ref SWITCHES_LIST type
    nbgl_contentInfoList_t        infosList;        ///< @ref INFOS_LIST type
    nbgl_contentRadioChoice_t     choicesList;      ///< @ref CHOICES_LIST type
    nbgl_contentBarsList_t        barsList;         ///< @ref BARS_LIST type
} nbgl_content_u;

/**
 * @brief This structure contains data to build a content
 */
typedef struct {
    nbgl_contentType_t type;  ///< type of page content in the content union
    nbgl_content_u     content;
    nbgl_contentActionCallback_t
        contentActionCallback;  ///< callback to be called when an action on an object occurs
} nbgl_content_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_CONTENT_H */
