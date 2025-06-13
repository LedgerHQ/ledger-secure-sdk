/**
  ******************************************************************************
  * @file    usbd_ioreq.h
  * @author  MCD Application Team
  * @version V2.4.1
  * @date    19-June-2015
  * @brief   Header file for the usbd_ioreq.c file
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

#ifndef USBD_IOREQ_H
#define USBD_IOREQ_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include "usbd_core.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
USBD_StatusTypeDef USBD_CtlSendData(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len);

USBD_StatusTypeDef USBD_CtlContinueSendData(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len);

USBD_StatusTypeDef USBD_CtlPrepareRx(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len);

USBD_StatusTypeDef USBD_CtlContinueRx(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t len);

USBD_StatusTypeDef USBD_CtlSendStatus(USBD_HandleTypeDef *pdev);
USBD_StatusTypeDef USBD_CtlReceiveStatus(USBD_HandleTypeDef *pdev);

uint32_t USBD_GetRxCount(USBD_HandleTypeDef *pdev, uint8_t ep_addr);

#endif  // USBD_IOREQ_H
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
