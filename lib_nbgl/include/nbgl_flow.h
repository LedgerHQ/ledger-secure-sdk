/**
 * @file nbgl_flow.h
 * @brief Types shared by the Nano use cases (the flow API itself is implemented by the OS)
 *
 */

#ifndef NBGL_FLOW_H
#define NBGL_FLOW_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "nbgl_step.h"
#include "nbgl_obj.h"
#include "nbgl_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief prototype of function to be called when a step is using a callback on "double-key" action
 */
typedef void (*nbgl_stepCallback_t)(void);

/**
 * @brief This structure contains data to build a page in multi-pages mode (@ref
 * nbgl_pageDrawGenericContent)
 */
typedef struct nbgl_pageContent_s {
    nbgl_contentType_t type;  ///< type of page content in the following union
    union {
        nbgl_contentCenteredInfo_t    centeredInfo;     ///< @ref CENTERED_INFO type
        nbgl_contentInfoButton_t      infoButton;       ///< @ref INFO_BUTTON type
        nbgl_contentTagValueList_t    tagValueList;     ///< @ref TAG_VALUE_LIST type
        nbgl_contentTagValueConfirm_t tagValueConfirm;  ///< @ref TAG_VALUE_CONFIRM type
        nbgl_contentSwitchesList_t    switchesList;     ///< @ref SWITCHES_LIST type
        nbgl_contentInfoList_t        infosList;        ///< @ref INFOS_LIST type
        nbgl_contentRadioChoice_t     choicesList;      ///< @ref CHOICES_LIST type
        nbgl_contentBarsList_t        barsList;         ///< @ref BARS_LIST type
    };
} nbgl_pageContent_t;

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NBGL_FLOW_H */
