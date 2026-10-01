/*
 * Unit tests for the CTAPHID channel handling of lib_stusb/src/usbd_ledger_hid_u2f.c, with
 * the real lib_u2f transport and a mocked USB low level that records every packet sent.
 *
 * A reply must go to the channel that sent the command, even when another channel sends a
 * packet while the command waits for the user and keepalives are flowing.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "Mockusbd_core.h"
#include "Mockusbd_ioreq.h"
#include "Mocklcx_crc.h"

#include "usbd_ledger_hid_u2f.c"

uint8_t USBD_LEDGER_io_buffer[OS_IO_BUFFER_SIZE + 1];

#define OWNER_CID 0x11111111u
#define OTHER_CID 0x22222222u
#define NEW_CID   0x33333333u
#define MAX_SENT  8

static USBD_HandleTypeDef pdev;
static uint8_t            sent[MAX_SENT][LEDGER_HID_U2F_EPIN_SIZE];
static unsigned int       sent_count;
// The IN endpoint holds one packet until the host takes it (data_in).
static bool in_flight;

static USBD_StatusTypeDef record_transmit(USBD_HandleTypeDef *dev,
                                          uint8_t             ep_addr,
                                          const uint8_t      *pbuf,
                                          uint32_t            size,
                                          uint32_t            timeout_ms,
                                          int                 cmock_num_calls)
{
    (void) dev;
    (void) ep_addr;
    (void) timeout_ms;
    (void) cmock_num_calls;

    TEST_ASSERT_FALSE_MESSAGE(in_flight, "IN packet submitted before the previous one completed");
    in_flight = true;
    TEST_ASSERT_LESS_THAN_UINT(MAX_SENT, sent_count);
    memcpy(sent[sent_count++], pbuf, size);
    return USBD_OK;
}

// The host takes the packet in flight.
static void usb_in_complete_one(void)
{
    TEST_ASSERT_TRUE(in_flight);
    in_flight = false;
    USBD_LEDGER_HID_U2F_data_in(&pdev, &ledger_hid_u2f_handle, LEDGER_HID_U2F_EPIN_ADDR);
}

// The host takes every packet submitted so far, one data_in per packet.
static void usb_in_drain(void)
{
    while (in_flight) {
        usb_in_complete_one();
    }
}

static uint32_t sent_cid(unsigned int i)
{
    return ((uint32_t) sent[i][0] << 24) | ((uint32_t) sent[i][1] << 16)
           | ((uint32_t) sent[i][2] << 8) | sent[i][3];
}

// The host writes one packet; the device reads it and hands any complete command to the app.
static int32_t host_sends(uint32_t cid, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t packet[LEDGER_HID_U2F_EPOUT_SIZE] = {0};
    uint8_t app_buffer[OS_IO_BUFFER_SIZE];

    packet[0] = (uint8_t) (cid >> 24);
    packet[1] = (uint8_t) (cid >> 16);
    packet[2] = (uint8_t) (cid >> 8);
    packet[3] = (uint8_t) cid;
    packet[4] = cmd | 0x80;
    packet[5] = (uint8_t) (len >> 8);
    packet[6] = (uint8_t) len;
    // The header declares len; only what fits in this first packet is copied.
    if (len > 0) {
        memcpy(&packet[7],
               data,
               (len < LEDGER_HID_U2F_EPOUT_SIZE - 7) ? len : LEDGER_HID_U2F_EPOUT_SIZE - 7);
    }

    USBD_LEDGER_HID_U2F_data_out(
        &pdev, &ledger_hid_u2f_handle, LEDGER_HID_U2F_EPOUT_ADDR, packet, sizeof(packet));
    int32_t status = USBD_LEDGER_HID_U2F_data_ready(
        &pdev, &ledger_hid_u2f_handle, app_buffer, sizeof(app_buffer));
    usb_in_drain();
    return status;
}

static int32_t host_sends_continuation(uint32_t cid, uint8_t seq)
{
    uint8_t packet[LEDGER_HID_U2F_EPOUT_SIZE] = {0};
    uint8_t app_buffer[OS_IO_BUFFER_SIZE];

    packet[0] = (uint8_t) (cid >> 24);
    packet[1] = (uint8_t) (cid >> 16);
    packet[2] = (uint8_t) (cid >> 8);
    packet[3] = (uint8_t) cid;
    packet[4] = seq;

    USBD_LEDGER_HID_U2F_data_out(
        &pdev, &ledger_hid_u2f_handle, LEDGER_HID_U2F_EPOUT_ADDR, packet, sizeof(packet));
    int32_t status = USBD_LEDGER_HID_U2F_data_ready(
        &pdev, &ledger_hid_u2f_handle, app_buffer, sizeof(app_buffer));
    usb_in_drain();
    return status;
}

// The app sends through the class table, as USBD_LEDGER_send() does.
static USBD_StatusTypeDef app_send(uint8_t packet_type, const uint8_t *message, uint16_t len)
{
    return USBD_LEDGER_HID_U2F_class_info.send_packet(
        &pdev, &ledger_hid_u2f_handle, packet_type, message, len, 0);
}

static void app_sends(uint8_t packet_type, const uint8_t *message, uint16_t len)
{
    app_send(packet_type, message, len);
    usb_in_drain();
}

static void app_sends_keepalive(void)
{
    static const uint8_t keepalive[] = {U2F_COMMAND_HID_KEEP_ALIVE, 0x02};

    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, keepalive, sizeof(keepalive));
}

// A packet reaches the device without the IN endpoint being drained.
static void packet_arrives(uint32_t cid, uint8_t head, const uint8_t *data, uint16_t len)
{
    uint8_t packet[LEDGER_HID_U2F_EPOUT_SIZE] = {0};
    uint8_t app_buffer[OS_IO_BUFFER_SIZE];

    packet[0] = (uint8_t) (cid >> 24);
    packet[1] = (uint8_t) (cid >> 16);
    packet[2] = (uint8_t) (cid >> 8);
    packet[3] = (uint8_t) cid;
    packet[4] = head;
    if (head & 0x80) {
        packet[5] = (uint8_t) (len >> 8);
        packet[6] = (uint8_t) len;
        if (len > 0) {
            memcpy(&packet[7],
                   data,
                   (len < LEDGER_HID_U2F_EPOUT_SIZE - 7) ? len : LEDGER_HID_U2F_EPOUT_SIZE - 7);
        }
    }
    USBD_LEDGER_HID_U2F_data_out(
        &pdev, &ledger_hid_u2f_handle, LEDGER_HID_U2F_EPOUT_ADDR, packet, sizeof(packet));
    USBD_LEDGER_HID_U2F_data_ready(&pdev, &ledger_hid_u2f_handle, app_buffer, sizeof(app_buffer));
}

void setUp(void)
{
    memset(&pdev, 0, sizeof(pdev));
    pdev.dev_state = USBD_STATE_CONFIGURED;
    memset(sent, 0, sizeof(sent));
    sent_count = 0;
    in_flight  = false;

    USBD_LL_PrepareReceive_IgnoreAndReturn(USBD_OK);
    USBD_LL_Transmit_Stub(record_transmit);

    USBD_LEDGER_HID_U2F_init(&pdev, &ledger_hid_u2f_handle);
    ledger_hid_u2f_settings.protocol_version  = 2;
    ledger_hid_u2f_settings.capabilities_flag = U2F_HID_CAPABILITY_CBOR;
    // Seeded from the RNG at start-up on the device.
    ledger_hid_free_cid = NEW_CID;
}

void tearDown(void) {}

void test_reply_goes_to_the_channel_that_sent_the_command(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends_keepalive();

    // Another channel sends a command while the owner's one is still pending.
    TEST_ASSERT_EQUAL_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends_keepalive();

    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    TEST_ASSERT_EQUAL_UINT(4, sent_count);

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_KEEP_ALIVE | 0x80, sent[0][4]);

    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[1][7]);

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(2));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_KEEP_ALIVE | 0x80, sent[2][4]);

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(3));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[3][4]);
}

void test_keepalive_does_not_end_the_command(void)
{
    static const uint8_t get_info = 0x04;

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends_keepalive();

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, ledger_hid_u2f_handle.transport_data.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, ledger_hid_u2f_handle.transport_data.state);
}

// Control: after the owner's reply, another channel is served normally.
void test_other_channel_is_served_after_the_reply(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(1));
}

// A refused message is answered once, not once per packet.
void test_refused_multi_packet_message_gets_one_error(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};
    uint8_t              big[100] = {0x04};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));

    TEST_ASSERT_EQUAL_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, big, sizeof(big)));
    TEST_ASSERT_EQUAL_INT32(0, host_sends_continuation(OTHER_CID, 0));

    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[1][4]);
}

// "the authenticator MUST NOT reply to the CTAPHID_CANCEL message itself".
void test_cancel_from_other_channel_is_not_answered(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    TEST_ASSERT_EQUAL_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    TEST_ASSERT_EQUAL_UINT(1, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[0][4]);
}

// A sender that stops midway must not keep a client that reopens the device out.
void test_broadcast_init_gets_past_a_stalled_sender(void)
{
    static const uint8_t nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    static const uint8_t get_info = 0x04;
    uint8_t              big[100] = {0x04};

    TEST_ASSERT_EQUAL_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, big, sizeof(big)));
    TEST_ASSERT_EQUAL_INT32(
        0, host_sends(U2F_BROADCAST_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce)));

    TEST_ASSERT_EQUAL_UINT(1, sent_count);
    TEST_ASSERT_EQUAL_HEX32(U2F_BROADCAST_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_INIT | 0x80, sent[0][4]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(nonce, &sent[0][7], sizeof(nonce));
    uint32_t allocated = ((uint32_t) sent[0][15] << 24) | ((uint32_t) sent[0][16] << 16)
                         | ((uint32_t) sent[0][17] << 8) | sent[0][18];
    TEST_ASSERT_EQUAL_HEX32(NEW_CID, allocated);

    // The new channel is usable.
    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(NEW_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
}

// A refusal must not overwrite a multi-packet reply still being sent.
void test_refusal_during_a_multi_packet_reply_waits_for_it(void)
{
    static const uint8_t get_info = 0x04;
    uint8_t              reply[150];
    uint8_t              packet[LEDGER_HID_U2F_EPOUT_SIZE] = {0};
    uint8_t              app_buffer[OS_IO_BUFFER_SIZE];
    uint8_t              received[sizeof(reply)];
    unsigned int         offset = 0;

    for (unsigned int i = 0; i < sizeof(reply); i++) {
        reply[i] = (uint8_t) i;
    }
    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));

    // First packet of a 3-packet reply goes out; the IN transfer is still in progress.
    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    // Another channel's packet arrives before that transfer completes.
    packet[0] = (uint8_t) (OTHER_CID >> 24);
    packet[1] = (uint8_t) (OTHER_CID >> 16);
    packet[2] = (uint8_t) (OTHER_CID >> 8);
    packet[3] = (uint8_t) OTHER_CID;
    packet[4] = U2F_COMMAND_HID_CBOR | 0x80;
    packet[6] = 1;
    packet[7] = get_info;
    USBD_LEDGER_HID_U2F_data_out(
        &pdev, &ledger_hid_u2f_handle, LEDGER_HID_U2F_EPOUT_ADDR, packet, sizeof(packet));
    USBD_LEDGER_HID_U2F_data_ready(&pdev, &ledger_hid_u2f_handle, app_buffer, sizeof(app_buffer));

    usb_in_drain();

    // The owner gets its whole reply: 57 bytes, then 59 and 34 in continuation packets.
    TEST_ASSERT_EQUAL_UINT(4, sent_count);
    for (unsigned int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(i));
        unsigned int header = (i == 0) ? 7 : 5;
        unsigned int chunk  = LEDGER_HID_U2F_EPIN_SIZE - header;
        if (chunk > sizeof(reply) - offset) {
            chunk = sizeof(reply) - offset;
        }
        memcpy(&received[offset], &sent[i][header], chunk);
        offset += chunk;
    }
    TEST_ASSERT_EQUAL_UINT(sizeof(reply), offset);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reply, received, sizeof(reply));

    // Then the refused channel gets its CHANNEL_BUSY.
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(3));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR | 0x80, sent[3][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[3][7]);
}

// Copilot on #1728: the refused message's continuation must not erase the waiting refusal.
void test_refusal_survives_its_continuation_during_a_reply(void)
{
    static const uint8_t get_info   = 0x04;
    uint8_t              reply[150] = {0};
    uint8_t              big[100]   = {0x04};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    // Both packets of another channel's message arrive while the reply is in flight.
    packet_arrives(OTHER_CID, U2F_COMMAND_HID_CBOR | 0x80, big, sizeof(big));
    packet_arrives(OTHER_CID, 0, NULL, 0);
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(4, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(2));
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(3));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR | 0x80, sent[3][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[3][7]);
}

// A command the transport answers itself must not be accepted while the reply's last packet
// is still in flight: the channel is locked until the host has it.
void test_init_waits_for_the_reply_to_reach_the_host(void)
{
    static const uint8_t get_info   = 0x04;
    static const uint8_t nonce[8]   = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t              reply[150] = {0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));
    usb_in_complete_one();  // packet 1 taken, packet 2 submitted
    usb_in_complete_one();  // packet 2 taken, packet 3 (the last) submitted
    TEST_ASSERT_EQUAL_UINT(3, sent_count);

    // A client opens the device while the last packet is still in flight.
    packet_arrives(U2F_BROADCAST_CID, U2F_COMMAND_HID_INIT | 0x80, nonce, sizeof(nonce));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(4, sent_count);
    TEST_ASSERT_EQUAL_HEX32(U2F_BROADCAST_CID, sent_cid(3));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR | 0x80, sent[3][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[3][7]);

    // Once the reply is out, the client's retry is served.
    TEST_ASSERT_EQUAL_INT32(
        0, host_sends(U2F_BROADCAST_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce)));
    TEST_ASSERT_EQUAL_UINT(5, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_INIT | 0x80, sent[4][4]);
}

// os_io_tx_cmd() waits while is_busy() and drops the UX events it reads meanwhile. A keepalive
// the host does not take (no FIDO client, or one busy elsewhere) must not hold the app: it is
// sent every tick during a review, and the buttons stopped working.
void test_one_packet_send_does_not_hold_the_sender(void)
{
    static const uint8_t keepalive[] = {U2F_COMMAND_HID_KEEP_ALIVE, 0x02};

    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, keepalive, sizeof(keepalive));
    TEST_ASSERT_FALSE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));

    // The next one finds the packet still in flight: it is dropped, and does not hold either.
    TEST_ASSERT_EQUAL_INT(
        USBD_BUSY, app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, keepalive, sizeof(keepalive)));
    TEST_ASSERT_FALSE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));

    usb_in_drain();
    TEST_ASSERT_EQUAL_UINT(1, sent_count);
}

// A multi-packet message is read from the sender's buffer until its last packet is built.
void test_multi_packet_send_holds_the_sender_until_its_last_packet(void)
{
    static const uint8_t get_info   = 0x04;
    uint8_t              reply[150] = {0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));
    TEST_ASSERT_TRUE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));
    usb_in_complete_one();
    TEST_ASSERT_TRUE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));
    usb_in_complete_one();  // the last packet is built and in flight
    TEST_ASSERT_FALSE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));
    usb_in_drain();
    TEST_ASSERT_EQUAL_UINT(3, sent_count);
}

// A send while the device is not configured changes nothing: no data_in would ever end it, so
// the channel stayed held and is_busy() stayed true.
void test_send_while_not_configured_changes_nothing(void)
{
    static const uint8_t get_info   = 0x04;
    uint8_t              reply[150] = {0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    pdev.dev_state = USBD_STATE_ADDRESSED;
    TEST_ASSERT_EQUAL_INT(USBD_FAIL,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply)));
    TEST_ASSERT_FALSE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));
    TEST_ASSERT_NULL(ledger_hid_u2f_handle.transport_data.tx_message_buffer);
    TEST_ASSERT_EQUAL_UINT(0, sent_count);

    // Configured again: the reply goes out whole.
    pdev.dev_state = USBD_STATE_CONFIGURED;
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));
    TEST_ASSERT_EQUAL_UINT(3, sent_count);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, ledger_hid_u2f_handle.transport_data.cid);
}

// Copilot on #1728: the app's reply sent while a packet is in flight goes out after it.
void test_app_send_while_a_packet_is_in_flight_goes_out_after_it(void)
{
    static const uint8_t get_info    = 0x04;
    static const uint8_t keepalive[] = {U2F_COMMAND_HID_KEEP_ALIVE, 0x02};
    static const uint8_t reply[]     = {0x00, 0xA0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, keepalive, sizeof(keepalive));

    TEST_ASSERT_EQUAL_INT(USBD_OK,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply)));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_KEEP_ALIVE | 0x80, sent[0][4]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, ledger_hid_u2f_handle.transport_data.cid);
    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
}

// Copilot on #1728: the owner's reply must not be lost to a refusal still in flight, and goes
// out before any later refusal.
void test_reply_waits_for_a_refusal_in_flight(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    packet_arrives(OTHER_CID, U2F_COMMAND_HID_CBOR | 0x80, &get_info, 1);
    TEST_ASSERT_EQUAL_UINT(1, sent_count);

    TEST_ASSERT_EQUAL_INT(USBD_OK,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply)));
    // A second refusal waits behind the reply.
    packet_arrives(NEW_CID, U2F_COMMAND_HID_CBOR | 0x80, &get_info, 1);
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(3, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX32(NEW_CID, sent_cid(2));
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[2][7]);
}

// A keepalive that finds the endpoint busy is dropped, and the command goes on.
void test_keepalive_during_a_refusal_keeps_the_command(void)
{
    static const uint8_t get_info    = 0x04;
    static const uint8_t keepalive[] = {U2F_COMMAND_HID_KEEP_ALIVE, 0x02};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    packet_arrives(OTHER_CID, U2F_COMMAND_HID_CBOR | 0x80, &get_info, 1);
    TEST_ASSERT_EQUAL_INT(
        USBD_BUSY, app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, keepalive, sizeof(keepalive)));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(1, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, ledger_hid_u2f_handle.transport_data.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, ledger_hid_u2f_handle.transport_data.state);
}

// Copilot on #1728: after its CANCEL, the owner cannot start a command before the app's reply.
void test_owner_command_after_cancel_waits_for_the_reply(void)
{
    static const uint8_t get_info    = 0x04;
    static const uint8_t cancelled[] = {CTAP2_ERR_KEEPALIVE_CANCEL};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0));
    TEST_ASSERT_EQUAL_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, cancelled, sizeof(cancelled));

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR | 0x80, sent[0][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX8(CTAP2_ERR_KEEPALIVE_CANCEL, sent[1][7]);

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
}

// Copilot on #1728: after the owner's CANCEL the channel stays locked until the app's reply.
void test_owner_cancel_keeps_the_channel_until_the_reply(void)
{
    static const uint8_t get_info    = 0x04;
    static const uint8_t cancelled[] = {CTAP2_ERR_KEEPALIVE_CANCEL};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0));

    TEST_ASSERT_EQUAL_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, cancelled, sizeof(cancelled));

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP2_ERR_KEEPALIVE_CANCEL, sent[1][7]);

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
}

// Copilot on #1728: a command sent before the previous one-packet reply completes gets
// CHANNEL_BUSY instead of an answer that is silently dropped.
void test_command_before_the_previous_reply_completes_is_refused(void)
{
    static const uint8_t first  = 0xA1;
    static const uint8_t second = 0xA2;

    packet_arrives(OWNER_CID, U2F_COMMAND_PING | 0x80, &first, 1);
    packet_arrives(OWNER_CID, U2F_COMMAND_PING | 0x80, &second, 1);
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_PING | 0x80, sent[0][4]);
    TEST_ASSERT_EQUAL_HEX8(first, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_ERROR | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[1][7]);

    // The retry, once the reply is out, is answered.
    TEST_ASSERT_EQUAL_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_PING, &second, 1));
    TEST_ASSERT_EQUAL_UINT(3, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_PING | 0x80, sent[2][4]);
    TEST_ASSERT_EQUAL_HEX8(second, sent[2][7]);
}

// A CANCEL announcing data would start a message over the owner's command; the broadcast
// INIT that then drops the stalled message must not take the owner's reply.
void test_split_cancel_cannot_release_the_channel(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    static const uint8_t reply[]  = {0x00, 0xA0};
    uint8_t              big[100] = {0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    TEST_ASSERT_EQUAL_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CANCEL, big, sizeof(big)));
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, ledger_hid_u2f_handle.transport_data.state);

    TEST_ASSERT_EQUAL_INT32(
        0, host_sends(U2F_BROADCAST_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce)));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(U2F_BROADCAST_CID, sent_cid(0));
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[1][4]);
}

// Copilot on #1728: a refusal sent after the owner's reply keeps new commands out until it
// completes, so their answers are not lost with their channel left claimed.
void test_command_during_a_refusal_after_the_reply_is_refused(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t data     = 0xA1;
    static const uint8_t reply[]  = {0x00, 0xA0};

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply));
    packet_arrives(OTHER_CID, U2F_COMMAND_HID_CBOR | 0x80, &get_info, 1);
    usb_in_complete_one();  // the reply is out; the refusal to OTHER_CID is in flight

    packet_arrives(NEW_CID, U2F_COMMAND_PING | 0x80, &data, 1);
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(3, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX32(NEW_CID, sent_cid(2));
    TEST_ASSERT_EQUAL_HEX8(CTAP1_ERR_CHANNEL_BUSY, sent[2][7]);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, ledger_hid_u2f_handle.transport_data.cid);

    TEST_ASSERT_EQUAL_INT32(0, host_sends(NEW_CID, U2F_COMMAND_PING, &data, 1));
    TEST_ASSERT_EQUAL_UINT(4, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_PING | 0x80, sent[3][4]);
}

// An answer the transport cannot send yet is lost, but must not leave its channel claimed.
void test_lost_transport_answer_frees_the_channel(void)
{
    static const uint8_t data     = 0xA1;
    uint8_t              big[100] = {0};

    // OTHER_CID's two-packet PING is half received when NEW_CID is refused.
    packet_arrives(OTHER_CID, U2F_COMMAND_PING | 0x80, big, sizeof(big));
    packet_arrives(NEW_CID, U2F_COMMAND_PING | 0x80, &data, 1);
    TEST_ASSERT_EQUAL_UINT(1, sent_count);

    // Its last packet arrives while the refusal is in flight.
    packet_arrives(OTHER_CID, 0, NULL, 0);
    usb_in_drain();

    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, ledger_hid_u2f_handle.transport_data.cid);
    TEST_ASSERT_EQUAL_INT32(0, host_sends(NEW_CID, U2F_COMMAND_PING, &data, 1));
    TEST_ASSERT_EQUAL_HEX32(NEW_CID, sent_cid(sent_count - 1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_PING | 0x80, sent[sent_count - 1][4]);
}

// A CANCEL carrying data is not handed to the app, and the command goes on.
void test_cancel_with_data_is_not_handed_to_the_app(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t data     = 0xA1;

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    TEST_ASSERT_EQUAL_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CANCEL, &data, 1));

    TEST_ASSERT_EQUAL_UINT(0, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, ledger_hid_u2f_handle.transport_data.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, ledger_hid_u2f_handle.transport_data.state);
}

static uint16_t fake_crc16(uint16_t crc, const void *buffer, size_t len, int cmock_num_calls)
{
    const uint8_t *bytes = buffer;

    (void) cmock_num_calls;
    for (size_t i = 0; i < len; i++) {
        crc = (uint16_t) ((crc << 5) ^ (crc >> 11) ^ bytes[i]);
    }
    return crc;
}

static const uint8_t register_apdu[] = {0x00, 0x01, 0x03, 0x00};

// The app asks for user presence: the host gets 0x6985 and retries until the user approves.
static void start_user_presence_wait(void)
{
    static const uint8_t ask_user[] = {0xFF, 0xFF};

    cx_crc16_update_Stub(fake_crc16);
    TEST_ASSERT_GREATER_THAN_INT32(
        0, host_sends(OWNER_CID, U2F_COMMAND_MSG, register_apdu, sizeof(register_apdu)));
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, ask_user, sizeof(ask_user));

    TEST_ASSERT_EQUAL_UINT(1, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_MSG | 0x80, sent[0][4]);
    TEST_ASSERT_EQUAL_HEX8(0x69, sent[0][7]);
    TEST_ASSERT_EQUAL_HEX8(0x85, sent[0][8]);
}

// The transport's own MSG answer during a user-presence wait is sent at once: kept as the
// backup reply, it pointed to data_ready()'s stack and went out later from a dead frame.
void test_transport_answer_during_user_presence_is_sent(void)
{
    static const uint8_t short_apdu[] = {0x00};
    static const uint8_t reply[]      = {0x05, 0x04, 0x90, 0x00};

    start_user_presence_wait();

    TEST_ASSERT_EQUAL_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_MSG, short_apdu, 1));
    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_MSG | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(2, sent[1][6]);
    TEST_ASSERT_EQUAL_HEX8(0x67, sent[1][7]);
    TEST_ASSERT_EQUAL_HEX8(0x00, sent[1][8]);
    TEST_ASSERT_EQUAL_UINT8(LEDGER_HID_U2F_USER_PRESENCE_ASKING,
                            ledger_hid_u2f_handle.user_presence);

    // The user approves: the app's reply is kept, and the host's retry gets it.
    app_sends(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, reply, sizeof(reply));
    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_INT32(
        0, host_sends(OWNER_CID, U2F_COMMAND_MSG, register_apdu, sizeof(register_apdu)));
    TEST_ASSERT_EQUAL_UINT(3, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(2));
    TEST_ASSERT_EQUAL_HEX8(sizeof(reply), sent[2][6]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reply, &sent[2][7], sizeof(reply));
}

// The app's request for user presence still becomes 0x6985 when it waits behind a packet.
void test_user_presence_request_behind_a_packet_is_sent_as_0x6985(void)
{
    static const uint8_t ask_user[] = {0xFF, 0xFF};
    static const uint8_t get_info   = 0x04;

    cx_crc16_update_Stub(fake_crc16);
    TEST_ASSERT_GREATER_THAN_INT32(
        0, host_sends(OWNER_CID, U2F_COMMAND_MSG, register_apdu, sizeof(register_apdu)));
    packet_arrives(OTHER_CID, U2F_COMMAND_HID_CBOR | 0x80, &get_info, 1);
    TEST_ASSERT_EQUAL_INT(USBD_OK,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, ask_user, sizeof(ask_user)));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_MSG | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(0x69, sent[1][7]);
    TEST_ASSERT_EQUAL_HEX8(0x85, sent[1][8]);
    TEST_ASSERT_EQUAL_UINT8(LEDGER_HID_U2F_USER_PRESENCE_ASKING,
                            ledger_hid_u2f_handle.user_presence);
}

// The owner's command is with the app, and a refusal to another channel is in flight.
static void start_owner_command_behind_a_refusal(void)
{
    static const uint8_t get_info = 0x04;

    TEST_ASSERT_GREATER_THAN_INT32(0, host_sends(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1));
    packet_arrives(OTHER_CID, U2F_COMMAND_HID_CBOR | 0x80, &get_info, 1);
    TEST_ASSERT_TRUE(in_flight);
}

// A message that waits for the IN endpoint must not be read from the caller's buffer later:
// os_io_tx_cmd() can return first, and the stack callers' frames are then gone.
void test_queued_message_does_not_keep_the_callers_buffer(void)
{
    uint8_t denied[] = {CTAP2_ERR_OPERATION_DENIED};

    start_owner_command_behind_a_refusal();
    TEST_ASSERT_EQUAL_INT(USBD_OK,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, denied, sizeof(denied)));
    // Copied: the sender may go.
    TEST_ASSERT_FALSE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));
    memset(denied, 0xEE, sizeof(denied));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(1));
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(1, sent[1][6]);
    TEST_ASSERT_EQUAL_HEX8(CTAP2_ERR_OPERATION_DENIED, sent[1][7]);
}

// The largest one-packet message is copied whole: a RAW one carries its command byte too.
void test_largest_one_packet_message_is_copied_whole(void)
{
    uint8_t message[LEDGER_HID_U2F_EPIN_SIZE - 7 + 1];
    uint8_t expected[sizeof(message)];

    message[0] = U2F_COMMAND_PING;
    for (unsigned int i = 1; i < sizeof(message); i++) {
        message[i] = (uint8_t) i;
    }
    memcpy(expected, message, sizeof(message));

    start_owner_command_behind_a_refusal();
    TEST_ASSERT_EQUAL_INT(USBD_OK,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, message, sizeof(message)));
    memset(message, 0xEE, sizeof(message));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(2, sent_count);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_PING | 0x80, sent[1][4]);
    TEST_ASSERT_EQUAL_HEX8(sizeof(message) - 1, sent[1][6]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&expected[1], &sent[1][7], sizeof(message) - 1);
}

// Control: a multi-packet reply that waits is still sent whole, from the app's buffer.
void test_queued_multi_packet_reply_is_sent_whole(void)
{
    uint8_t      reply[150];
    uint8_t      received[sizeof(reply)];
    unsigned int offset = 0;

    for (unsigned int i = 0; i < sizeof(reply); i++) {
        reply[i] = (uint8_t) i;
    }
    start_owner_command_behind_a_refusal();
    TEST_ASSERT_EQUAL_INT(USBD_OK,
                          app_send(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, reply, sizeof(reply)));
    // Read from the sender's buffer later: the sender must wait.
    TEST_ASSERT_TRUE(USBD_LEDGER_HID_U2F_is_busy(&ledger_hid_u2f_handle));
    usb_in_drain();

    TEST_ASSERT_EQUAL_UINT(4, sent_count);
    for (unsigned int i = 1; i < 4; i++) {
        TEST_ASSERT_EQUAL_HEX32(OWNER_CID, sent_cid(i));
        unsigned int header = (i == 1) ? 7 : 5;
        unsigned int chunk  = LEDGER_HID_U2F_EPIN_SIZE - header;
        if (chunk > sizeof(reply) - offset) {
            chunk = sizeof(reply) - offset;
        }
        memcpy(&received[offset], &sent[i][header], chunk);
        offset += chunk;
    }
    TEST_ASSERT_EQUAL_UINT(sizeof(reply), offset);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reply, received, sizeof(reply));
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, ledger_hid_u2f_handle.transport_data.cid);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_reply_goes_to_the_channel_that_sent_the_command);
    RUN_TEST(test_keepalive_does_not_end_the_command);
    RUN_TEST(test_other_channel_is_served_after_the_reply);
    RUN_TEST(test_refused_multi_packet_message_gets_one_error);
    RUN_TEST(test_cancel_from_other_channel_is_not_answered);
    RUN_TEST(test_broadcast_init_gets_past_a_stalled_sender);
    RUN_TEST(test_refusal_during_a_multi_packet_reply_waits_for_it);
    RUN_TEST(test_refusal_survives_its_continuation_during_a_reply);
    RUN_TEST(test_init_waits_for_the_reply_to_reach_the_host);
    RUN_TEST(test_one_packet_send_does_not_hold_the_sender);
    RUN_TEST(test_multi_packet_send_holds_the_sender_until_its_last_packet);
    RUN_TEST(test_send_while_not_configured_changes_nothing);
    RUN_TEST(test_app_send_while_a_packet_is_in_flight_goes_out_after_it);
    RUN_TEST(test_reply_waits_for_a_refusal_in_flight);
    RUN_TEST(test_keepalive_during_a_refusal_keeps_the_command);
    RUN_TEST(test_owner_command_after_cancel_waits_for_the_reply);
    RUN_TEST(test_owner_cancel_keeps_the_channel_until_the_reply);
    RUN_TEST(test_command_before_the_previous_reply_completes_is_refused);
    RUN_TEST(test_split_cancel_cannot_release_the_channel);
    RUN_TEST(test_command_during_a_refusal_after_the_reply_is_refused);
    RUN_TEST(test_lost_transport_answer_frees_the_channel);
    RUN_TEST(test_cancel_with_data_is_not_handed_to_the_app);
    RUN_TEST(test_transport_answer_during_user_presence_is_sent);
    RUN_TEST(test_user_presence_request_behind_a_packet_is_sent_as_0x6985);
    RUN_TEST(test_queued_message_does_not_keep_the_callers_buffer);
    RUN_TEST(test_largest_one_packet_message_is_copied_whole);
    RUN_TEST(test_queued_multi_packet_reply_is_sent_whole);
    return UNITY_END();
}
