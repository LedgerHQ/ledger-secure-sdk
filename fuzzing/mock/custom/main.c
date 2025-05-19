#include <stdint.h>
#include <string.h>

#include "os.h"
#include "io.h"
#include "write.h"

#ifdef HAVE_SWAP
#include "swap.h"
#endif

#ifdef HAVE_NFC_READER
#include "os_io_nfc.h"
#endif  // HAVE_NFC_READER

// TODO: Temporary workaround, at some point all status words should be defined by the SDK and
// removed from the application
#define SW_OK                    0x9000
#define SW_WRONG_RESPONSE_LENGTH 0xB000

#ifdef HAVE_NFC_READER
struct nfc_reader_context G_io_reader_ctx;
#endif
#pragma GCC diagnostic ignored "-Wunused-variable"

/**
 * Variable containing the length of the APDU response to send back.
 */
static uint32_t G_output_len = 0;

/**
 * IO state (READY, RECEIVING, WAITING).
 */
static io_state_e G_io_state = READY;

bolos_ux_params_t G_ux_params;
