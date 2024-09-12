/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    U2F_MEDIA_NONE,
    U2F_MEDIA_USB,
    U2F_MEDIA_NFC,
    U2F_MEDIA_BLE
} u2f_transport_media_t;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    u2f_transport_media_t media;
} u2f_service_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
