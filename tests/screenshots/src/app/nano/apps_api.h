/**
 * @file apps_api.h
 * @brief Header for Full app
 *
 */

#ifndef APP_FULL_H
#define APP_FULL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nbgl_types.h"
#include "glyphs.h"

// Fake coin apps
void app_fullBitcoin(void);
void app_bitcoinInfos(void);
void app_bitcoinSignTransaction(void);
void app_bitcoinVerifyAddress(void);
void app_fullEthereum(void);
void app_ethereumSettings(void);
void app_ethereumSignMessage(void);
void app_ethereumVerifyAddress(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_FULL_H */
