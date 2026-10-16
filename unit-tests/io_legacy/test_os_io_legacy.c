/*
 * Unit tests for io_legacy/src/os_io_legacy.c: while a command awaits its reply, no other
 * command may reach the app, whatever transport it comes from, and a CTAPHID_CANCEL only
 * reaches a pending CTAP2 command.
 *
 * Built for the in-app IO stack (no USE_OS_IO_STACK), where a USB class can still read a reply
 * from the app's buffer after os_io_tx_cmd() returns.
 *
 * The source is included to reach its static state. The OS calls are fakes: one packet is
 * queued for os_io_rx_app_evt(), and os_io_tx_cmd() records what is sent.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "os_io_legacy.c"

unsigned char G_io_rx_buffer[OS_IO_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[OS_IO_BUFFER_SIZE + 1];
unsigned char G_io_seproxyhal_spi_buffer[OS_IO_SEPH_BUFFER_SIZE];
u2f_service_t G_io_u2f;

/* Fakes ---------------------------------------------------------------------*/

static uint8_t rx_packet[OS_IO_BUFFER_SIZE + 1];
static int     rx_packet_length;

#define MAX_SENT 4
static struct {
    uint8_t type;
    uint8_t data[16];
    size_t  length;
} sent[MAX_SENT];
static int sent_count;

int32_t os_io_rx_evt(uint8_t  *buffer,
                     uint16_t  buffer_max_length,
                     uint32_t *timeout_ms,
                     bool      check_se_event)
{
    (void) timeout_ms;
    (void) check_se_event;
    int length       = rx_packet_length;
    rx_packet_length = 0;
    TEST_ASSERT_LESS_OR_EQUAL_INT(buffer_max_length, length);
    memcpy(buffer, rx_packet, length);
    return length;
}

// The real one waits for a non-empty event; a test always queues one first.
int32_t os_io_rx_app_evt(uint8_t  *buffer,
                         uint16_t  buffer_max_length,
                         uint32_t *timeout_ms,
                         bool      check_se_event)
{
    return os_io_rx_evt(buffer, buffer_max_length, timeout_ms, check_se_event);
}

int os_io_tx_cmd(unsigned char        type,
                 const unsigned char *buffer,
                 unsigned short       length,
                 unsigned int        *timeout_ms)
{
    (void) timeout_ms;
    TEST_ASSERT_LESS_THAN_INT(MAX_SENT, sent_count);
    sent[sent_count].type   = type;
    sent[sent_count].length = length;
    memcpy(sent[sent_count].data,
           buffer,
           length < sizeof(sent[0].data) ? length : sizeof(sent[0].data));
    sent_count++;
    return length;
}

// Whether a USB class still reads a reply from the app's buffer.
static bool usb_busy;

int32_t USBD_LEDGER_is_busy(void)
{
    return usb_busy;
}

bolos_bool_t os_perso_is_pin_set(void)
{
    return BOLOS_FALSE;
}

bolos_bool_t os_global_pin_is_validated(void)
{
    return BOLOS_TRUE;
}

bolos_err_t os_io_handle_default_apdu(uint8_t                  *buffer_in,
                                      size_t                    buffer_in_length,
                                      uint8_t                  *buffer_out,
                                      size_t                   *buffer_out_length,
                                      os_io_apdu_post_action_t *post_action)
{
    (void) buffer_in;
    (void) buffer_in_length;
    (void) buffer_out;
    (void) post_action;
    *buffer_out_length = 0;
    return SWO_SUCCESS;
}

unsigned char io_event(unsigned char channel)
{
    (void) channel;
    return 0;
}

int io_process_itc_ux_event(uint8_t *buffer_in, size_t buffer_in_length)
{
    (void) buffer_in;
    (void) buffer_in_length;
    return 0;
}

int os_io_init(os_io_init_t *init)
{
    (void) init;
    return 0;
}

int os_io_start(void)
{
    return 0;
}

int os_io_stop(void)
{
    return 0;
}

int os_io_seph_cmd_general_status(void)
{
    return 0;
}

void os_reset(void)
{
    TEST_FAIL_MESSAGE("os_reset");
    for (;;) {
    }
}

void os_sched_exit(bolos_task_status_t exit_code)
{
    (void) exit_code;
    TEST_FAIL_MESSAGE("os_sched_exit");
}

/* Helpers -------------------------------------------------------------------*/

// What a transport hands to the app layer: the packet type, then the payload.
static int receive(uint8_t type, const uint8_t *payload, size_t length)
{
    rx_packet[0] = type;
    if (length) {
        memcpy(&rx_packet[1], payload, length);
    }
    rx_packet_length = length + 1;
    return io_legacy_apdu_rx(1);
}

static const uint8_t HID_APDU[]   = {0xE0, 0x02, 0x00, 0x00, 0x00};
static const uint8_t U2F_APDU[]   = {0x00, 0x02, 0x03, 0x00, 0x00};
static const uint8_t CBOR_LOGIN[] = {0x02, 0xA1, 0x01, 0x61, 0x61};
static const uint8_t OTHER_APDU[] = {0xE0, 0x04, 0x00, 0x00, 0x00};
static const uint8_t GET_INFO[]   = {0x04};
static const uint8_t PAYLOAD[]    = {0xAA, 0xBB, 0xCC};

static void assert_pending_untouched(const uint8_t  *command,
                                     size_t          length,
                                     apdu_type_t     type,
                                     io_apdu_state_e state)
{
    TEST_ASSERT_TRUE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(
        command, G_io_apdu_buffer, length, "pending command overwritten");
    TEST_ASSERT_EQUAL_MESSAGE(type, io_os_legacy_apdu_type, "reply route changed");
    TEST_ASSERT_EQUAL_MESSAGE(state, G_io_app.apdu_state, "APDU state changed");
}

void setUp(void)
{
    usb_busy           = false;
    io_reply_in_flight = false;
    os_io_set_reply_pending(false);
    io_os_legacy_apdu_type = APDU_TYPE_NONE;
    memset(&G_io_app, 0, sizeof(G_io_app));
    memset(&G_io_u2f, 0, sizeof(G_io_u2f));
    memset(G_io_tx_buffer, 0, sizeof(G_io_tx_buffer));
    rx_packet_length = 0;
    sent_count       = 0;
}

void tearDown(void) {}

/* Tests ---------------------------------------------------------------------*/

void test_command_arms_the_latch(void)
{
    TEST_ASSERT_EQUAL_INT(sizeof(HID_APDU),
                          receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU)));
    TEST_ASSERT_TRUE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_HEX8_ARRAY(HID_APDU, G_io_apdu_buffer, sizeof(HID_APDU));
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

// The refusal must not be built in G_io_tx_buffer: it is G_io_apdu_buffer, which holds
// the command under review.
void test_command_during_review_is_refused_without_touching_the_pending_one(void)
{
    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));

    TEST_ASSERT_EQUAL_INT(0,
                          receive(OS_IO_PACKET_TYPE_USB_HID_APDU, OTHER_APDU, sizeof(OTHER_APDU)));
    TEST_ASSERT_EQUAL_INT(1, sent_count);
    TEST_ASSERT_EQUAL_UINT8(OS_IO_PACKET_TYPE_USB_HID_APDU, sent[0].type);
    TEST_ASSERT_EQUAL_size_t(2, sent[0].length);
    TEST_ASSERT_EQUAL_HEX8(0x69, sent[0].data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, sent[0].data[1]);
    assert_pending_untouched(HID_APDU, sizeof(HID_APDU), APDU_TYPE_USB_HID, APDU_USB_HID);
}

// V-072: a CTAPHID_CBOR from the U2F interface during a review on another transport.
void test_cbor_during_another_transport_review_is_refused(void)
{
    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));
    io_apdu_media_t media = G_io_app.apdu_media;

    TEST_ASSERT_EQUAL_INT(0,
                          receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, GET_INFO, sizeof(GET_INFO)));
    // A CTAP2 client gets a CTAPHID error frame, not a status word.
    TEST_ASSERT_EQUAL_INT(1, sent_count);
    TEST_ASSERT_EQUAL_UINT8(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, sent[0].type);
    TEST_ASSERT_EQUAL_size_t(2, sent[0].length);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR, sent[0].data[0]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[0].data[1]);
    assert_pending_untouched(HID_APDU, sizeof(HID_APDU), APDU_TYPE_USB_HID, APDU_USB_HID);
    TEST_ASSERT_EQUAL(media, G_io_app.apdu_media);
}

void test_cbor_during_cbor_review_is_refused(void)
{
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, CBOR_LOGIN, sizeof(CBOR_LOGIN));

    TEST_ASSERT_EQUAL_INT(0,
                          receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, GET_INFO, sizeof(GET_INFO)));
    TEST_ASSERT_EQUAL_INT(1, sent_count);
    TEST_ASSERT_EQUAL_UINT8(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, sent[0].type);
    assert_pending_untouched(CBOR_LOGIN, sizeof(CBOR_LOGIN), APDU_TYPE_USB_U2F_CBOR, APDU_U2F_CBOR);
}

// A refused CBOR must not release the latch for the others either.
void test_refused_cbor_keeps_the_gate_closed(void)
{
    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, GET_INFO, sizeof(GET_INFO));

    TEST_ASSERT_EQUAL_INT(0,
                          receive(OS_IO_PACKET_TYPE_USB_HID_APDU, OTHER_APDU, sizeof(OTHER_APDU)));
    TEST_ASSERT_EQUAL_INT(2, sent_count);
    assert_pending_untouched(HID_APDU, sizeof(HID_APDU), APDU_TYPE_USB_HID, APDU_USB_HID);
}

// The cancel reaches the app, but its data and its packet type change nothing.
void test_cancel_during_cbor_review_reaches_the_app_alone(void)
{
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, CBOR_LOGIN, sizeof(CBOR_LOGIN));
    io_apdu_media_t media = G_io_app.apdu_media;

    TEST_ASSERT_EQUAL_INT(1,
                          receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, PAYLOAD, sizeof(PAYLOAD)));
    TEST_ASSERT_EQUAL(APDU_U2F_CANCEL, G_io_app.apdu_state);
    TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(
        CBOR_LOGIN, G_io_apdu_buffer, sizeof(CBOR_LOGIN), "CANCEL data copied");
    TEST_ASSERT_EQUAL_MESSAGE(
        APDU_TYPE_USB_U2F_CBOR, io_os_legacy_apdu_type, "reply route changed");
    TEST_ASSERT_EQUAL(media, G_io_app.apdu_media);
    TEST_ASSERT_TRUE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

void test_cancel_without_pending_command_is_dropped(void)
{
    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, NULL, 0));
    TEST_ASSERT_NOT_EQUAL(APDU_U2F_CANCEL, G_io_app.apdu_state);
    TEST_ASSERT_FALSE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

// A U2F MSG cannot be cancelled: its reply would go out with the CANCEL type and be lost.
void test_cancel_during_u2f_review_is_dropped(void)
{
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, U2F_APDU, sizeof(U2F_APDU));

    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, NULL, 0));
    assert_pending_untouched(U2F_APDU, sizeof(U2F_APDU), APDU_TYPE_USB_U2F, APDU_U2F);
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

void test_cancel_during_another_transport_review_is_dropped(void)
{
    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));

    TEST_ASSERT_EQUAL_INT(0,
                          receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, PAYLOAD, sizeof(PAYLOAD)));
    assert_pending_untouched(HID_APDU, sizeof(HID_APDU), APDU_TYPE_USB_HID, APDU_USB_HID);
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

// u2f_message_reply() releases the latch: the command it answered can no longer be cancelled.
void test_cancel_after_the_cbor_reply_is_dropped(void)
{
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, CBOR_LOGIN, sizeof(CBOR_LOGIN));
    os_io_set_reply_pending(false);

    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, NULL, 0));
    TEST_ASSERT_EQUAL(APDU_U2F_CBOR, G_io_app.apdu_state);
}

// The latch remembers what armed it: a later non-CTAP2 command cannot be cancelled.
void test_cbor_flag_does_not_outlive_its_command(void)
{
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, CBOR_LOGIN, sizeof(CBOR_LOGIN));
    os_io_set_reply_pending(false);
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, U2F_APDU, sizeof(U2F_APDU));

    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, NULL, 0));
    assert_pending_untouched(U2F_APDU, sizeof(U2F_APDU), APDU_TYPE_USB_U2F, APDU_U2F);
}

// RAW is a transmit-only type: a received one is ignored.
void test_received_raw_packet_is_ignored(void)
{
    static const uint8_t error[] = {U2F_COMMAND_ERROR, CTAP1_ERR_CHANNEL_BUSY};

    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error, sizeof(error)));
    TEST_ASSERT_FALSE(os_io_reply_pending());
    TEST_ASSERT_EQUAL(APDU_TYPE_NONE, io_os_legacy_apdu_type);
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

// USB CDC input (AT_CMD) has no reply, but it is copied into G_io_apdu_buffer: it must wait
// like any command while another one awaits its reply.
void test_cdc_input_during_review_is_dropped(void)
{
    static const uint8_t at_cmd[] = {'A', 'T', '+', 'X'};

    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));
    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_AT_CMD, at_cmd, sizeof(at_cmd)));
    assert_pending_untouched(HID_APDU, sizeof(HID_APDU), APDU_TYPE_USB_HID, APDU_USB_HID);

    os_io_set_reply_pending(false);
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, CBOR_LOGIN, sizeof(CBOR_LOGIN));
    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_AT_CMD, at_cmd, sizeof(at_cmd)));
    assert_pending_untouched(CBOR_LOGIN, sizeof(CBOR_LOGIN), APDU_TYPE_USB_U2F_CBOR, APDU_U2F_CBOR);
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

// Control: with nothing pending, CDC input reaches the app as before, without arming the latch.
void test_cdc_input_reaches_the_app_when_idle(void)
{
    static const uint8_t at_cmd[] = {'A', 'T', '+', 'X'};

    TEST_ASSERT_GREATER_THAN_INT(0, receive(OS_IO_PACKET_TYPE_AT_CMD, at_cmd, sizeof(at_cmd)));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(at_cmd, G_io_apdu_buffer, sizeof(at_cmd));
    TEST_ASSERT_FALSE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_INT(0, sent_count);
}

// os_io_tx_cmd() returned, but USB still sends the reply from G_io_tx_buffer: a command from
// another transport would be copied over it.
void test_gate_stays_closed_until_usb_has_read_the_reply(void)
{
    static const uint8_t reply[] = {0x01, 0x02, 0x03, 0x04, 0x90, 0x00};

    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));
    memcpy(G_io_tx_buffer, reply, sizeof(reply));
    usb_busy = true;
    io_legacy_apdu_tx(G_io_tx_buffer, sizeof(reply));
    TEST_ASSERT_TRUE(os_io_reply_pending());

    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_BLE_APDU, OTHER_APDU, sizeof(OTHER_APDU)));
    TEST_ASSERT_EQUAL_UINT8(OS_IO_PACKET_TYPE_BLE_APDU, sent[1].type);
    TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(reply, G_io_tx_buffer, sizeof(reply), "reply overwritten");

    // USB has read it all: the next command is taken.
    usb_busy = false;
    TEST_ASSERT_EQUAL_INT(sizeof(OTHER_APDU),
                          receive(OS_IO_PACKET_TYPE_BLE_APDU, OTHER_APDU, sizeof(OTHER_APDU)));
    TEST_ASSERT_TRUE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_INT(2, sent_count);
}

// The CTAP2 command is answered even if its reply is still being sent: it cannot be cancelled,
// and another CTAP2 command still waits.
void test_cbor_reply_in_flight_closes_cancel_but_not_the_gate(void)
{
    receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, CBOR_LOGIN, sizeof(CBOR_LOGIN));
    usb_busy = true;
    os_io_set_reply_pending(false);  // as u2f_message_reply() does after os_io_tx_cmd()

    TEST_ASSERT_EQUAL_INT(0, receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, NULL, 0));
    TEST_ASSERT_EQUAL(APDU_U2F_CBOR, G_io_app.apdu_state);
    TEST_ASSERT_EQUAL_INT(0,
                          receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, GET_INFO, sizeof(GET_INFO)));
    TEST_ASSERT_EQUAL_UINT8(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, sent[0].type);
    TEST_ASSERT_TRUE(os_io_reply_pending());
}

// USB busy with something else does not arm a latch that no command armed.
void test_usb_busy_without_pending_command_keeps_the_gate_open(void)
{
    usb_busy = true;
    os_io_set_reply_pending(false);
    TEST_ASSERT_FALSE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_INT(sizeof(HID_APDU),
                          receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU)));
}

void test_reply_reopens_the_gate(void)
{
    static const uint8_t reply[] = {0x90, 0x00};

    receive(OS_IO_PACKET_TYPE_USB_HID_APDU, HID_APDU, sizeof(HID_APDU));
    io_legacy_apdu_tx(reply, sizeof(reply));
    TEST_ASSERT_FALSE(os_io_reply_pending());
    TEST_ASSERT_EQUAL_UINT8(OS_IO_PACKET_TYPE_USB_HID_APDU, sent[0].type);

    TEST_ASSERT_EQUAL_INT(sizeof(GET_INFO),
                          receive(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, GET_INFO, sizeof(GET_INFO)));
    TEST_ASSERT_TRUE(os_io_reply_pending());
    TEST_ASSERT_EQUAL(APDU_U2F_CBOR, G_io_app.apdu_state);
    TEST_ASSERT_EQUAL_INT(1, sent_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_command_arms_the_latch);
    RUN_TEST(test_command_during_review_is_refused_without_touching_the_pending_one);
    RUN_TEST(test_cbor_during_another_transport_review_is_refused);
    RUN_TEST(test_cbor_during_cbor_review_is_refused);
    RUN_TEST(test_refused_cbor_keeps_the_gate_closed);
    RUN_TEST(test_cancel_during_cbor_review_reaches_the_app_alone);
    RUN_TEST(test_cancel_without_pending_command_is_dropped);
    RUN_TEST(test_cancel_during_u2f_review_is_dropped);
    RUN_TEST(test_cancel_during_another_transport_review_is_dropped);
    RUN_TEST(test_cancel_after_the_cbor_reply_is_dropped);
    RUN_TEST(test_cbor_flag_does_not_outlive_its_command);
    RUN_TEST(test_received_raw_packet_is_ignored);
    RUN_TEST(test_cdc_input_during_review_is_dropped);
    RUN_TEST(test_cdc_input_reaches_the_app_when_idle);
    RUN_TEST(test_gate_stays_closed_until_usb_has_read_the_reply);
    RUN_TEST(test_cbor_reply_in_flight_closes_cancel_but_not_the_gate);
    RUN_TEST(test_usb_busy_without_pending_command_keeps_the_gate_open);
    RUN_TEST(test_reply_reopens_the_gate);
    return UNITY_END();
}
