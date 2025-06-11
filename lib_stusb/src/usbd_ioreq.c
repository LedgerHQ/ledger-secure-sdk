/**
  ******************************************************************************
  * @file    usbd_ioreq.c
  * @author  MCD Application Team
  * @version V2.4.1
  * @date    19-June-2015
  * @brief   This file provides the IO requests APIs for control endpoints.
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
* @brief  USBD_CtlSendData
*         send data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be sent
* @retval status
*/
USBD_StatusTypeDef USBD_CtlSendData(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len)
{
    // Set EP0 State
    pdev->ep0_state             = USBD_EP0_DATA_IN;
    pdev->ep_in[0].total_length = len;
    pdev->ep_in[0].rem_length   = len;
    pdev->pData                 = pbuf;

    // Start the transfer
    (void) USBD_LL_Transmit(pdev, 0x00U, pbuf, MIN(len, pdev->ep_in[0].maxpacket), 0);

    return USBD_OK;
}

/**
* @brief  USBD_CtlContinueSendData
*         continue sending data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be sent
* @retval status
*/
USBD_StatusTypeDef USBD_CtlContinueSendData(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len)
{
    // Start the next transfer
    (void) USBD_LL_Transmit(pdev, 0x00U, pbuf, MIN(len, pdev->ep_in[0].maxpacket), 0);

    return USBD_OK;
}

/**
* @brief  USBD_CtlPrepareRx
*         receive data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be received
* @retval status
*/
USBD_StatusTypeDef USBD_CtlPrepareRx(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len)
{
    // Set EP0 State
    pdev->ep0_state              = USBD_EP0_DATA_OUT;
    pdev->ep_out[0].total_length = len;
    pdev->ep_out[0].rem_length   = len;

    // Start the transfer
    (void) USBD_LL_PrepareReceive(pdev, 0U, pbuf, len);

    return USBD_OK;
}

/**
* @brief  USBD_CtlContinueRx
*         continue receive data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be received
* @retval status
*/
USBD_StatusTypeDef USBD_CtlContinueRx(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len)
{
    (void) USBD_LL_PrepareReceive(pdev, 0U, pbuf, len);

    return USBD_OK;
}

/**
* @brief  USBD_CtlSendStatus
*         send zero lzngth packet on the ctl pipe
* @param  pdev: device instance
* @retval status
*/
USBD_StatusTypeDef USBD_CtlSendStatus(USBD_HandleTypeDef *pdev)
{
    // Set EP0 State
    pdev->ep0_state = USBD_EP0_STATUS_IN;

    // Start the transfer
    (void) USBD_LL_Transmit(pdev, 0x00U, NULL, 0U, 0);

    return USBD_OK;
}

/**
* @brief  USBD_CtlReceiveStatus
*         receive zero lzngth packet on the ctl pipe
* @param  pdev: device instance
* @retval status
*/
USBD_StatusTypeDef USBD_CtlReceiveStatus(USBD_HandleTypeDef *pdev)
{
    // Set EP0 State
    pdev->ep0_state = USBD_EP0_STATUS_OUT;

    // Start the transfer
    (void) USBD_LL_PrepareReceive(pdev, 0U, NULL, 0U);

    return USBD_OK;
}

uint32_t USBD_GetRxCount(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return USBD_LL_GetRxDataSize(pdev, ep_addr);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
