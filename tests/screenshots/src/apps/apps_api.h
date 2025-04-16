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
#include "os_helpers.h"

// Fake coin apps
void app_fullBitcoin(void);
void app_bitcoinInfos(void);
void app_bitcoinSignTransaction(void);
void app_bitcoinVerifyAddress(void);
void app_fullCardano(void);
void app_cardanoSignTransaction(void);
void app_cardanoVerifyAddress(void);
void app_fullEthereum(void);
void app_ethereumSettings(void);
void app_ethereumSignMessage(void);
void app_ethereumSignForwardOnlyMessage(void);
void app_ethereumSignUnknownLengthMessage(void);
void app_ethereumVerifyAddress(void);
void app_fullEthereumShareAddress(void);
void app_fullAlgorand(void);
void app_fullStellar(void);
void app_stellarSignTransaction(void);
void app_stellarSignStreamedTransaction(void);
void app_fullTron(void);
void app_fullXrp(void);
void app_xrpSignMessage(void);
void app_xrpSignMessageLight(void);
void app_tronSignMessage(void);
void app_tronSettings(void);
void app_fullMonero(void);
void app_moneroSignForwardOnlyMessage(void);
void app_moneroSettings(void);
void app_fullRecoveryCheck(void);
void app_fullLedgerMarket(void);
void app_fullDogecoin(void);
void app_dogecoinSignTransaction(bool blind,
                                 bool dApp,
                                 bool w3c_enabled,
                                 bool w3c_issue,
                                 bool w3c_threat);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* APP_FULL_H */
