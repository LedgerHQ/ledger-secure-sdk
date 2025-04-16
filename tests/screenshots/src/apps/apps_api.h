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

#define exit_app NULL

#if defined(SCREEN_SIZE_WALLET)
#define ALGO_MAIN_ICON      Algorand32px
#define BTC_MAIN_ICON       C_ic_asset_btc_64
#define CARD_MAIN_ICON      C_ic_asset_cardano_64
#define CHARON_MAIN_ICON    charon_updater_32px
#define DOGE_MAIN_ICON      C_ic_asset_doge_64
#define DOT_MAIN_ICON       dot_32px
#define ETH_MAIN_ICON       C_ic_asset_eth_64
#define LTC_MAIN_ICON       eth_32px
#define MARKET_MAIN_ICON    LedgerMarket32px
#define MON_MAIN_ICON       C_ic_asset_monero_64
#define REC_CHECK_MAIN_ICON RecoveryCheck32px
#define STELLAR_MAIN_ICON   C_ic_asset_stellar_64
#define TRON_MAIN_ICON      Tron32px
#define XRP_MAIN_ICON       xrp_32px
#elif defined(SCREEN_SIZE_NANO)
#define ALGO_MAIN_ICON      C_eth_log_14px
#define BTC_MAIN_ICON       C_btc_log_14px
#define CARD_MAIN_ICON      C_btc_log_14px
#define CHARON_MAIN_ICON    C_btc_log_14px
#define DOGE_MAIN_ICON      C_btc_log_14px
#define DOT_MAIN_ICON       C_btc_log_14px
#define ETH_MAIN_ICON       C_eth_log_14px
#define LTC_MAIN_ICON       C_eth_log_14px
#define MARKET_MAIN_ICON    C_eth_log_14px
#define MON_MAIN_ICON       C_icon_certificate
#define REC_CHECK_MAIN_ICON C_eth_log_14px
#define STELLAR_MAIN_ICON   C_eth_log_14px
#define TRON_MAIN_ICON      C_eth_log_14px
#define XRP_MAIN_ICON       C_eth_log_14px
#endif  // TARGET_STAX

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
void app_ethereumReviewBlindSigningTransaction(void);
void app_ethereumReviewENSTransaction(void);
void app_ethereumReviewDappTransaction(void);
void app_ethereumVerifyAddress(void);
void app_ethereumSecurityKeyLogin(void);
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
