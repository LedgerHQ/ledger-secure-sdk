/**
  ******************************************************************************
  * @file    usbd_req.c
  * @author  MCD Application Team
  * @version V2.4.1
  * @date    19-June-2015 
  * @brief   This file provides the standard USB requests following chapter 9.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

#include "os_helpers.h"
#include "os_io.h"
#include "os_pic.h"

/* Includes ------------------------------------------------------------------*/
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void               USBD_GetDescriptor(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void               USBD_SetAddress(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static USBD_StatusTypeDef USBD_SetConfig(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void               USBD_GetConfig(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void               USBD_GetStatus(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void               USBD_SetFeature(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void               USBD_ClrFeature(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t            USBD_GetLen(uint8_t *buf);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
* @brief  USBD_StdDevReq
*         Handle standard usb device requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
USBD_StatusTypeDef USBD_StdDevReq(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
        case USB_REQ_TYPE_VENDOR:
            ret = (USBD_StatusTypeDef) ((USB_Setup_t) PIC(pdev->pClass->Setup))(pdev, req);
            break;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                case USB_REQ_GET_DESCRIPTOR:
                    USBD_GetDescriptor(pdev, req);
                    break;

                case USB_REQ_SET_ADDRESS:
                    USBD_SetAddress(pdev, req);
                    break;

                case USB_REQ_SET_CONFIGURATION:
                    ret = USBD_SetConfig(pdev, req);
                    break;

                case USB_REQ_GET_CONFIGURATION:
                    USBD_GetConfig(pdev, req);
                    break;

                case USB_REQ_GET_STATUS:
                    USBD_GetStatus(pdev, req);
                    break;

                case USB_REQ_SET_FEATURE:
                    USBD_SetFeature(pdev, req);
                    break;

                case USB_REQ_CLEAR_FEATURE:
                    USBD_ClrFeature(pdev, req);
                    break;

                default:
                    ret = (USBD_StatusTypeDef) ((USB_Setup_t) PIC(pdev->pClass->Setup))(pdev, req);
                    break;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            break;
    }

    return ret;
}

/**
* @brief  USBD_StdItfReq
*         Handle standard usb interface requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
USBD_StatusTypeDef USBD_StdItfReq(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
        case USB_REQ_TYPE_VENDOR:
        case USB_REQ_TYPE_STANDARD:
            switch (pdev->dev_state) {
                case USBD_STATE_DEFAULT:
                case USBD_STATE_ADDRESSED:
                case USBD_STATE_CONFIGURED:
                    if (LOBYTE(req->wIndex) <= USBD_MAX_NUM_INTERFACES) {
                        ret = (USBD_StatusTypeDef) ((USB_Setup_t) PIC(pdev->pClass->Setup))(pdev,
                                                                                            req);
                        if ((req->wLength == 0U) && (ret == USBD_OK)) {
                            (void) USBD_CtlSendStatus(pdev);
                        }
                    }
                    else {
                        USBD_CtlError(pdev, req);
                    }
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    break;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            break;
    }

    return ret;
}

/**
* @brief  USBD_StdEPReq
*         Handle standard usb endpoint requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
USBD_StatusTypeDef USBD_StdEPReq(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_StatusTypeDef    ret     = USBD_OK;
    USBD_EndpointTypeDef *pep     = NULL;
    uint8_t               ep_addr = LOBYTE(req->wIndex);

    if ((ep_addr & 0x7FU) >= IO_USB_MAX_ENDPOINTS) {
        return USBD_FAIL;
    }

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
        case USB_REQ_TYPE_VENDOR:
            ret = (USBD_StatusTypeDef) ((USB_Setup_t) PIC(pdev->pClass->Setup))(pdev, req);
            break;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                case USB_REQ_SET_FEATURE:
                    switch (pdev->dev_state) {
                        case USBD_STATE_ADDRESSED:
                            if ((ep_addr != 0x00U) && (ep_addr != 0x80U)) {
                                (void) USBD_LL_StallEP(pdev, ep_addr);
                                (void) USBD_LL_StallEP(pdev, 0x80U);
                            }
                            else {
                                USBD_CtlError(pdev, req);
                            }
                            break;

                        case USBD_STATE_CONFIGURED:
                            if (req->wValue == USB_FEATURE_EP_HALT) {
                                if ((ep_addr != 0x00U) && (ep_addr != 0x80U)
                                    && (req->wLength == 0x00U)) {
                                    (void) USBD_LL_StallEP(pdev, ep_addr);
                                }
                            }
                            (void) USBD_CtlSendStatus(pdev);
                            break;

                        default:
                            USBD_CtlError(pdev, req);
                            break;
                    }
                    break;

                case USB_REQ_CLEAR_FEATURE:
                    switch (pdev->dev_state) {
                        case USBD_STATE_ADDRESSED:
                            if ((ep_addr != 0x00U) && (ep_addr != 0x80U)) {
                                (void) USBD_LL_StallEP(pdev, ep_addr);
                                (void) USBD_LL_StallEP(pdev, 0x80U);
                            }
                            else {
                                USBD_CtlError(pdev, req);
                            }
                            break;

                        case USBD_STATE_CONFIGURED:
                            if (req->wValue == USB_FEATURE_EP_HALT) {
                                if ((ep_addr & 0x7FU) != 0x00U) {
                                    (void) USBD_LL_ClearStallEP(pdev, ep_addr);
                                }
                                (void) USBD_CtlSendStatus(pdev);
                                ret = (USBD_StatusTypeDef) pdev->pClass->Setup(pdev, req);
                            }
                            break;

                        default:
                            USBD_CtlError(pdev, req);
                            break;
                    }
                    break;

                case USB_REQ_GET_STATUS:
                    switch (pdev->dev_state) {
                        case USBD_STATE_ADDRESSED:
                            if ((ep_addr != 0x00U) && (ep_addr != 0x80U)) {
                                USBD_CtlError(pdev, req);
                                break;
                            }
                            pep = ((ep_addr & 0x80U) == 0x80U) ? &pdev->ep_in[ep_addr & 0x7FU]
                                                               : &pdev->ep_out[ep_addr & 0x7FU];

                            pep->status = 0x0000U;

                            (void) USBD_CtlSendData(pdev, (uint8_t *) &pep->status, 2U);
                            break;

                        case USBD_STATE_CONFIGURED:
                            if ((ep_addr & 0x80U) == 0x80U) {
                                if (pdev->ep_in[ep_addr & 0xFU].is_used == 0U) {
                                    USBD_CtlError(pdev, req);
                                    break;
                                }
                            }
                            else {
                                if (pdev->ep_out[ep_addr & 0xFU].is_used == 0U) {
                                    USBD_CtlError(pdev, req);
                                    break;
                                }
                            }

                            pep = ((ep_addr & 0x80U) == 0x80U) ? &pdev->ep_in[ep_addr & 0x7FU]
                                                               : &pdev->ep_out[ep_addr & 0x7FU];
                            if ((ep_addr & 0x7fU) >= IO_USB_MAX_ENDPOINTS) {
                                break;
                            }

                            if ((ep_addr == 0x00U) || (ep_addr == 0x80U)) {
                                pep->status = 0x0000U;
                            }
                            else if (USBD_LL_IsStallEP(pdev, ep_addr) != 0U) {
                                pep->status = 0x0001U;
                            }
                            else {
                                pep->status = 0x0000U;
                            }

                            (void) USBD_CtlSendData(pdev, (uint8_t *) &pep->status, 2U);
                            break;

                        default:
                            USBD_CtlError(pdev, req);
                            break;
                    }
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    break;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            break;
    }

    return ret;
}

/**
* @brief  USBD_GetDescriptor
*         Handle Get Descriptor requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void USBD_GetDescriptor(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint16_t len  = 0U;
    uint8_t *pbuf = NULL;
    uint8_t  err  = 0U;

    if (pdev->pDesc == NULL) {
        USBD_CtlError(pdev, req);
        return;
    }
    switch (req->wValue >> 8) {
        case USB_DESC_TYPE_BOS:
            if (pdev->pDesc->GetBOSDescriptor != NULL) {
                pbuf = ((USB_GetBOSDescriptor_t) PIC(pdev->pDesc->GetBOSDescriptor))(
                    pdev->dev_speed, &len);
            }
            else {
                USBD_CtlError(pdev, req);
                err++;
            }
            break;

        case USB_DESC_TYPE_DEVICE:
            pbuf = ((USB_GetDeviceDescriptor_t) PIC(pdev->pDesc->GetDeviceDescriptor))(
                pdev->dev_speed, &len);
            break;

        case USB_DESC_TYPE_CONFIGURATION:
            if (pdev->dev_speed == USBD_SPEED_HIGH) {
                pbuf = ((USB_GetHSConfigDescriptor_t) PIC(pdev->pClass->GetHSConfigDescriptor))(
                    &len);
            }
            else {
                pbuf = ((USB_GetFSConfigDescriptor_t) PIC(pdev->pClass->GetFSConfigDescriptor))(
                    &len);
            }
            break;

        case USB_DESC_TYPE_STRING:
            switch ((uint8_t) (req->wValue)) {
                case USBD_IDX_LANGID_STR:
                    if (pdev->pDesc->GetLangIDStrDescriptor != NULL) {
                        pbuf = ((USB_GetLangIDStrDescriptor_t) PIC(
                            pdev->pDesc->GetLangIDStrDescriptor))(pdev->dev_speed, &len);
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        err++;
                    }
                    break;

                case USBD_IDX_MFC_STR:
                    if (pdev->pDesc->GetManufacturerStrDescriptor != NULL) {
                        pbuf = ((USB_GetManufacturerStrDescriptor_t) PIC(
                            pdev->pDesc->GetManufacturerStrDescriptor))(pdev->dev_speed, &len);
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        err++;
                    }
                    break;

                case USBD_IDX_PRODUCT_STR:
                    if (pdev->pDesc->GetProductStrDescriptor != NULL) {
                        pbuf = ((USB_GetProductStrDescriptor_t) PIC(
                            pdev->pDesc->GetProductStrDescriptor))(pdev->dev_speed, &len);
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        err++;
                    }
                    break;

                case USBD_IDX_SERIAL_STR:
                    if (pdev->pDesc->GetSerialStrDescriptor != NULL) {
                        pbuf = ((USB_GetSerialStrDescriptor_t) PIC(
                            pdev->pDesc->GetSerialStrDescriptor))(pdev->dev_speed, &len);
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        err++;
                    }
                    break;

                case USBD_IDX_CONFIG_STR:
                    if (pdev->pDesc->GetConfigurationStrDescriptor != NULL) {
                        pbuf = ((USB_GetConfigurationStrDescriptor_t) PIC(
                            pdev->pDesc->GetConfigurationStrDescriptor))(pdev->dev_speed, &len);
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        err++;
                    }
                    break;

                case USBD_IDX_INTERFACE_STR:
                    if (pdev->pDesc->GetInterfaceStrDescriptor != NULL) {
                        pbuf = ((USB_GetInterfaceStrDescriptor_t) PIC(
                            pdev->pDesc->GetInterfaceStrDescriptor))(pdev->dev_speed, &len);
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        err++;
                    }
                    break;

                case USBD_IDX_WINUSB_STR:
                    // TODO_IO : Could we process this?
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    err++;
                    break;
            }
            break;

        case USB_DESC_TYPE_DEVICE_QUALIFIER:
            if (pdev->dev_speed == USBD_SPEED_HIGH) {
                pbuf = ((USB_GetDeviceQualifierDescriptor_t) PIC(
                    pdev->pClass->GetDeviceQualifierDescriptor))(&len);
            }
            else {
                USBD_CtlError(pdev, req);
                err++;
            }
            break;

        case USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
            if (pdev->dev_speed == USBD_SPEED_HIGH) {
                pbuf    = ((USB_GetOtherSpeedConfigDescriptor_t) PIC(
                    pdev->pClass->GetOtherSpeedConfigDescriptor))(&len);
                pbuf[1] = USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION;
            }
            else {
                USBD_CtlError(pdev, req);
                err++;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            err++;
            break;
    }

    if (err != 0U) {
        return;
    }

    if (req->wLength != 0U) {
        if (len != 0U) {
            len = MIN(len, req->wLength);
            (void) USBD_CtlSendData(pdev, pbuf, len);
        }
        else {
            USBD_CtlError(pdev, req);
        }
    }
    else {
        (void) USBD_CtlSendStatus(pdev);
    }
}

/**
* @brief  USBD_SetAddress
*         Set device address
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void USBD_SetAddress(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint8_t dev_addr;

    if ((req->wIndex == 0U) && (req->wLength == 0U) && (req->wValue < 128U)) {
        dev_addr = (uint8_t) (req->wValue) & 0x7FU;

        if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            USBD_CtlError(pdev, req);
        }
        else {
            pdev->dev_address = dev_addr;
            (void) USBD_LL_SetUSBAddress(pdev, dev_addr);
            (void) USBD_CtlSendStatus(pdev);

            if (dev_addr != 0U) {
                pdev->dev_state = USBD_STATE_ADDRESSED;
            }
            else {
                pdev->dev_state = USBD_STATE_DEFAULT;
            }
        }
    }
    else {
        USBD_CtlError(pdev, req);
    }
}

/**
* @brief  USBD_SetConfig
*         Handle Set device configuration request
* @param  pdev: device instanceUSBD_SetConfig
* @param  req: usb request
* @retval status
*/
static USBD_StatusTypeDef USBD_SetConfig(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_StatusTypeDef ret    = USBD_OK;
    static uint8_t     cfgidx = 0;

    cfgidx = (uint8_t) (req->wValue);

    if (cfgidx > USBD_MAX_NUM_CONFIGURATION) {
        USBD_CtlError(pdev, req);
        return USBD_FAIL;
    }

    switch (pdev->dev_state) {
        case USBD_STATE_ADDRESSED:
            if (cfgidx != 0U) {
                pdev->dev_config = cfgidx;
                ret              = USBD_SetClassConfig(pdev, cfgidx);
                if (ret != USBD_OK) {
                    USBD_CtlError(pdev, req);
                }
                else {
                    (void) USBD_CtlSendStatus(pdev);
                    pdev->dev_state = USBD_STATE_CONFIGURED;
                }
            }
            else {
                (void) USBD_CtlSendStatus(pdev);
            }
            break;

        case USBD_STATE_CONFIGURED:
            if (cfgidx == 0U) {
                pdev->dev_state  = USBD_STATE_ADDRESSED;
                pdev->dev_config = cfgidx;
                (void) USBD_ClrClassConfig(pdev, cfgidx);
                (void) USBD_CtlSendStatus(pdev);
            }
            else if (cfgidx != pdev->dev_config) {
                // Clear old configuration
                (void) USBD_ClrClassConfig(pdev, (uint8_t) pdev->dev_config);

                // set new configuration
                pdev->dev_config = cfgidx;

                ret = USBD_SetClassConfig(pdev, cfgidx);
                if (ret != USBD_OK) {
                    USBD_CtlError(pdev, req);
                    (void) USBD_ClrClassConfig(pdev, (uint8_t) pdev->dev_config);
                    pdev->dev_state = USBD_STATE_ADDRESSED;
                }
                else {
                    (void) USBD_CtlSendStatus(pdev);
                }
            }
            else {
                (void) USBD_CtlSendStatus(pdev);
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            (void) USBD_ClrClassConfig(pdev, cfgidx);
            ret = USBD_FAIL;
            break;
    }

    return ret;
}

/**
* @brief  USBD_GetConfig
*         Handle Get device configuration request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void USBD_GetConfig(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    if (req->wLength != 1U) {
        USBD_CtlError(pdev, req);
    }
    else {
        switch (pdev->dev_state) {
            case USBD_STATE_DEFAULT:
            case USBD_STATE_ADDRESSED:
                pdev->dev_default_config = 0U;
                (void) USBD_CtlSendData(pdev, (uint8_t *) &pdev->dev_default_config, 1U);
                break;

            case USBD_STATE_CONFIGURED:
                (void) USBD_CtlSendData(pdev, (uint8_t *) &pdev->dev_config, 1U);
                break;

            default:
                USBD_CtlError(pdev, req);
                break;
        }
    }
}

/**
* @brief  USBD_GetStatus
*         Handle Get Status request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void USBD_GetStatus(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    switch (pdev->dev_state) {
        case USBD_STATE_DEFAULT:
        case USBD_STATE_ADDRESSED:
        case USBD_STATE_CONFIGURED:
            if (req->wLength != 0x2U) {
                USBD_CtlError(pdev, req);
                break;
            }

            pdev->dev_config_status = USB_CONFIG_SELF_POWERED;
            if (pdev->dev_remote_wakeup != 0U) {
                pdev->dev_config_status |= USB_CONFIG_REMOTE_WAKEUP;
            }

            (void) USBD_CtlSendData(pdev, (uint8_t *) &pdev->dev_config_status, 2U);
            break;

        default:
            USBD_CtlError(pdev, req);
            break;
    }
}

/**
* @brief  USBD_SetFeature
*         Handle Set device feature request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void USBD_SetFeature(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
        pdev->dev_remote_wakeup = 1U;
        (void) USBD_CtlSendStatus(pdev);
    }
}

/**
 * @brief  USBD_ClrFeature
 *         Handle clear device feature request
 * @param  pdev: device instance
 * @param  req: usb request
 * @retval status
 */
static void USBD_ClrFeature(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    switch (pdev->dev_state) {
        case USBD_STATE_DEFAULT:
        case USBD_STATE_ADDRESSED:
        case USBD_STATE_CONFIGURED:
            if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
                pdev->dev_remote_wakeup = 0U;
                (void) USBD_CtlSendStatus(pdev);
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            break;
    }
}

/**
* @brief  USBD_ParseSetupRequest 
*         Copy buffer into setup structure
* @param  pdev: device instance
* @param  req: usb request
* @retval None
*/
void USBD_ParseSetupRequest(USBD_SetupReqTypedef *req, uint8_t *pdata)
{
    uint8_t *pbuff = pdata;

    req->bmRequest = *(uint8_t *) (pbuff);
    pbuff += 1;
    req->bRequest = *(uint8_t *) (pbuff);
    pbuff += 1;
    req->wValue = SWAPBYTE(pbuff);
    pbuff += 2;
    req->wIndex = SWAPBYTE(pbuff);
    pbuff += 2;
    req->wLength = SWAPBYTE(pbuff);
}

void USBD_CtlError(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    UNUSED(req);

    (void) USBD_LL_StallEP(pdev, 0x80U);
    (void) USBD_LL_StallEP(pdev, 0U);
}

/**
  * @brief  USBD_GetString
  *         Convert Ascii string into unicode one
  * @param  desc : descriptor buffer
  * @param  unicode : Formatted string buffer (unicode)
  * @param  len : descriptor length
  * @retval None
  */
void USBD_GetString(uint8_t *desc, uint8_t *unicode, uint16_t *len)
{
    uint8_t  idx   = 0U;
    uint8_t *pdesc = NULL;

    if (desc == NULL) {
        return;
    }

    pdesc = desc;
    *len  = ((uint16_t) USBD_GetLen(pdesc) * 2U) + 2U;

    unicode[idx++] = *(uint8_t *) len;
    unicode[idx++] = USB_DESC_TYPE_STRING;

    while (*pdesc != (uint8_t) '\0') {
        unicode[idx++] = *pdesc;
        unicode[idx++] = 0U;
        pdesc++;
    }
}

/**
  * @brief  USBD_GetLen
  *         return the string length
   * @param  buf : pointer to the ascii string buffer
  * @retval string length
  */
static uint8_t USBD_GetLen(uint8_t *buf)
{
    uint8_t  len   = 0U;
    uint8_t *pbuff = buf;

    while (*pbuff != (uint8_t) '\0') {
        len++;
        pbuff++;
    }

    return len;
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
