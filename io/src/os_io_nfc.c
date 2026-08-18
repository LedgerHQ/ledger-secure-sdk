#ifdef HAVE_NFC_READER

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_io.h"
#include "os_io_seph_cmd.h"
#include "checks.h"
#include "errors.h"
#include "os_io_nfc.h"
#include "os_pic.h"
#include "decorators.h"

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define LOG_IO(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

struct nfc_reader_context {
    nfc_resp_callback_t resp_callback;
    nfc_evt_callback_t  evt_callback;
    bool                reader_mode;
    unsigned int        remaining_ms;
    struct card_info    card;
};

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void nfc_event(uint8_t *buffer_in, size_t buffer_in_length);
static void nfc_ticker(void);

/* Private variables ---------------------------------------------------------*/

static struct nfc_reader_context G_io_reader_ctx = {};

/* Private functions ---------------------------------------------------------*/

static void nfc_event(uint8_t *buffer_in, size_t buffer_in_length)
{
    if (buffer_in_length < 3) {
        return;
    }
    size_t size = U2BE(buffer_in, 1);
    if (buffer_in_length < 3 + size) {
        return;
    }

    if (size >= 1) {
        switch (buffer_in[3]) {
            case SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED: {
                G_io_reader_ctx.card.tech
                    = (buffer_in[4] == SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED_A) ? NFC_A : NFC_B;
                G_io_reader_ctx.card.nfcid_len = MIN(size - 2, sizeof(G_io_reader_ctx.card.nfcid));
                memcpy((void *) G_io_reader_ctx.card.nfcid,
                       buffer_in + 5,
                       G_io_reader_ctx.card.nfcid_len);

                if (G_io_reader_ctx.evt_callback != NULL) {
                    G_io_reader_ctx.evt_callback(CARD_DETECTED,
                                                 (struct card_info *) &G_io_reader_ctx.card);
                }
            } break;

            case SEPROXYHAL_TAG_NFC_EVENT_CARD_LOST:
                // If card is removed during an APDU processing, call the resp_callback with an
                // error
                if (G_io_reader_ctx.resp_callback != NULL) {
                    nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
                    G_io_reader_ctx.resp_callback = NULL;
                    resp_cb(true, false, NULL, 0);
                }

                if (G_io_reader_ctx.evt_callback != NULL) {
                    G_io_reader_ctx.evt_callback(CARD_REMOVED,
                                                 (struct card_info *) &G_io_reader_ctx.card);
                }
                memset((void *) &G_io_reader_ctx.card, 0, sizeof(G_io_reader_ctx.card));
                break;
        }
    }
}

static void nfc_ticker(void)
{
    if (G_io_reader_ctx.resp_callback != NULL) {
        if (G_io_reader_ctx.remaining_ms <= 100) {
            G_io_reader_ctx.remaining_ms  = 0;
            nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
            G_io_reader_ctx.resp_callback = NULL;
            resp_cb(false, true, NULL, 0);
        }
        else {
            G_io_reader_ctx.remaining_ms -= 100;
        }
    }
}

/* Exported functions --------------------------------------------------------*/

void os_io_nfc_reader_rx(uint8_t *in_buffer, size_t in_buffer_len)
{
    if (G_io_reader_ctx.resp_callback != NULL) {
        nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
        G_io_reader_ctx.resp_callback = NULL;
        resp_cb(false, false, in_buffer, in_buffer_len);
    }
}

void os_io_nfc_evt(uint8_t *buffer_in, size_t buffer_in_length)
{
    switch (buffer_in[0]) {
        case SEPROXYHAL_TAG_NFC_EVENT:
            nfc_event(buffer_in, buffer_in_length);
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            nfc_ticker();
            break;

        default:
            break;
    }
}

bool os_io_nfc_reader_send(const uint8_t      *cmd_data,
                           size_t              cmd_len,
                           nfc_resp_callback_t callback,
                           int                 timeout_ms)
{
    G_io_reader_ctx.resp_callback = PIC(callback);
    os_io_tx_cmd(APDU_TYPE_NFC, PIC(cmd_data), cmd_len, 0);

    G_io_reader_ctx.remaining_ms = timeout_ms;

    return true;
}

bool os_io_nfc_reader_start(nfc_evt_callback_t callback)
{
    G_io_reader_ctx.evt_callback  = PIC(callback);
    G_io_reader_ctx.reader_mode   = true;
    G_io_reader_ctx.resp_callback = NULL;
    os_io_nfc_cmd_start_reader();
    return true;
}

void os_io_nfc_reader_stop(void)
{
    G_io_reader_ctx.evt_callback  = NULL;
    G_io_reader_ctx.reader_mode   = false;
    G_io_reader_ctx.resp_callback = NULL;
    os_io_nfc_cmd_stop();
}

bool os_io_nfc_is_reader(void)
{
    return G_io_reader_ctx.reader_mode;
}
#endif  // HAVE_NFC_READER
