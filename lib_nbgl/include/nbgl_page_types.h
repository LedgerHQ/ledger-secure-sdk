/**
 * @file nbgl_page_types.h
 * @brief Types of NBGL pages also used by the use-case API
 *
 */

#ifndef NBGL_PAGE_TYPES_H
#define NBGL_PAGE_TYPES_H

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
#include "nbgl_content.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief Deprecated, kept for retro compatibility
 */
typedef nbgl_contentInfoLongPress_t nbgl_pageInfoLongPress_t;

/**
 * @brief This structure contains data to build a page in multi-pages mode (@ref
 * nbgl_pageDrawGenericContent)
 */
typedef struct nbgl_pageContent_s {
    const char *title;             ///< text for the title of the page (if NULL, no title)
    bool        isTouchableTitle;  ///< if set to true, the title is preceded by <- arrow to go back
    uint8_t titleToken;   ///< if isTouchableTitle set to true, this is the token used when touching
                          ///< title
    tune_index_e tuneId;  ///< if not @ref NBGL_NO_TUNE, a tune will be played when title is touched
    uint8_t      topRightToken;  ///< token used when top-right button (if not NULL) is touched
    const nbgl_icon_details_t *topRightIcon;  ///< a buffer containing the 32px/40px icon for icon
                                              ///< on top-right of screen (no button if NULL)
    nbgl_contentType_t type;                  ///< type of page content in the following union
    union {
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
    };
} nbgl_pageContent_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_PAGE_TYPES_H */
