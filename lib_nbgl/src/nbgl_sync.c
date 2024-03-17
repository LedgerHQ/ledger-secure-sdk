#include "nbgl_use_case.h"
#include "nbgl_sync.h"
#include "nbgl_wrapper.h"
#include "nbgl_page.h"
// #include "io.h"
#include "ledger_assert.h"
#include "os.h"

static sync_nbgl_ret_t ret;
static bool ended;


static void choice_callback(bool confirm) {
    if (confirm) {
        ret = NBGL_SYNC_RET_SUCCESS;
    } else {
        ret = NBGL_SYNC_RET_REJECTED;
    }

    ended = true;
}

static void quit_callback(void) {
    ret = NBGL_SYNC_RET_SUCCESS;
    ended = true;
}

static void sync_nbgl_init(void) {
   ended = false;
   ret = NBGL_SYNC_RET_ERROR;
}

__attribute((weak)) bool recv_and_process_event(bool return_on_apdu) {
    // int apdu_state = G_io_app.apdu_state;
    // Check for a apdu_state change.
    // This would means an APDU was received.
    // In such case, and if return_on_apdu is set to true by init_recv_and_process_event
    // we should return true, otherwise call os_io_seph_recv_and_process(0) and return false.
    if (/*apdu_state == APDU_IDLE &&*/ G_io_app.apdu_state != APDU_IDLE && return_on_apdu) {
        return true;
    }
    os_io_seph_recv_and_process(0);
    return false;
}

static sync_nbgl_ret_t sync_nbgl_wait(bool return_on_apdu) {
    while (!ended) {

        // - Looping on os_io_seph_recv_and_process(0);
        // - This will send a general_status and then wait for an event.
        // - Upon event reception this will call io_seproxyhal_handle_event()
        //   - On some case this will call io_event() which usually forward the
        //     event to the UX lib.
        if (recv_and_process_event(return_on_apdu)) {
            return NBGL_SYNC_RET_RX_APDU;
        }
    }
    return ret;
}

sync_nbgl_ret_t sync_nbgl_useCaseTransactionReview(
    const nbgl_layoutTagValueList_t *tagValueList,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle, /* Most often this is empty, but sometimes indicates a path / index */
    const char *finish_page_text /*unused on Nano*/)
{
    sync_nbgl_init();
    nbgl_useCaseTransactionReview(tagValueList,
                                  icon,
                                  reviewTitle,
                                  reviewSubTitle,
                                  finish_page_text,
                                  choice_callback);
    return sync_nbgl_wait(false);
}

sync_nbgl_ret_t sync_nbgl_useCaseAddressReview(
    const char *address,
    const nbgl_icon_details_t *icon,
    const char *reviewTitle,
    const char *reviewSubTitle)
{
    sync_nbgl_init();
    nbgl_useCaseAddressReview(address,
                              icon,
                              reviewTitle,
                              reviewSubTitle,
                              choice_callback);

    return sync_nbgl_wait(false);
}

sync_nbgl_ret_t sync_nbgl_useCaseStatus(const char *message, bool isSuccess)
{
    sync_nbgl_init();
    nbgl_useCaseStatus(message, isSuccess, quit_callback);
    return sync_nbgl_wait(false);
}

//////////////////////////
#define APPNAME "MyApp"
// static const char* const INFO_TYPES[] = {"Version", "Developer"};
// static const char* const INFO_CONTENTS[] = {"1.1.0", "Ledger"};


void app_quit(void) {
    // exit app here
    os_sched_exit(-1);
}

// home page definition
sync_nbgl_ret_t sync_nbgl_useCaseHomeAndSettings(void) {
    sync_nbgl_init();
    nbgl_useCaseHome(APPNAME,
                     NULL,
                     NULL,
                     false,
                     NULL,
                     app_quit);
    return sync_nbgl_wait(true);
}
