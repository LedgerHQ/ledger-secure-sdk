/*
 * Unit tests for the CTAPHID channel handling in lib_u2f/src/u2f_transport.c.
 *
 * While a command is processed, the channel belongs to its sender until the final reply:
 * packets from other channels are refused without taking over the reply address, and
 * keepalives do not end the transaction.
 */

#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "u2f_transport.h"
#include "u2f_types.h"

#define OWNER_CID 0x11111111u
#define OTHER_CID 0x22222222u
#define PACKET    64

static u2f_transport_t transport;
static uint8_t         rx_buffer[1024];
static uint8_t         tx_packet[PACKET];

static void rx_init_packet(uint32_t cid, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t packet[PACKET] = {0};

    packet[0] = (uint8_t) (cid >> 24);
    packet[1] = (uint8_t) (cid >> 16);
    packet[2] = (uint8_t) (cid >> 8);
    packet[3] = (uint8_t) cid;
    packet[4] = cmd | 0x80;
    packet[5] = (uint8_t) (len >> 8);
    packet[6] = (uint8_t) len;
    // The header declares len; only what fits in this first packet is copied.
    if (len > 0) {
        memcpy(&packet[7], data, (len < PACKET - 7) ? len : PACKET - 7);
    }
    U2F_TRANSPORT_rx(&transport, packet, sizeof(packet));
}

static void rx_continuation_packet(uint32_t cid, uint8_t seq)
{
    uint8_t packet[PACKET] = {0};

    packet[0] = (uint8_t) (cid >> 24);
    packet[1] = (uint8_t) (cid >> 16);
    packet[2] = (uint8_t) (cid >> 8);
    packet[3] = (uint8_t) cid;
    packet[4] = seq;
    U2F_TRANSPORT_rx(&transport, packet, sizeof(packet));
}

static uint32_t tx_packet_cid(void)
{
    return ((uint32_t) tx_packet[0] << 24) | ((uint32_t) tx_packet[1] << 16)
           | ((uint32_t) tx_packet[2] << 8) | tx_packet[3];
}

// The owner's command has been received and handed to the app, as data_ready() does.
static void start_owner_command(void)
{
    static const uint8_t get_info = 0x04;

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
    transport.state = U2F_STATE_CMD_PROCESSING;
}

void setUp(void)
{
    memset(&transport, 0, sizeof(transport));
    transport.rx_message_buffer      = rx_buffer;
    transport.rx_message_buffer_size = sizeof(rx_buffer);
    U2F_TRANSPORT_init(&transport, U2F_TRANSPORT_TYPE_USB_HID);
}

void tearDown(void) {}

void test_other_channel_is_refused_without_taking_the_reply_address(void)
{
    static const uint8_t get_info = 0x04;

    start_owner_command();
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.tx_cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

void test_keepalive_keeps_the_channel(void)
{
    static const uint8_t reason = 0x02;

    start_owner_command();
    U2F_TRANSPORT_tx(
        &transport, U2F_COMMAND_HID_KEEP_ALIVE, &reason, 1, tx_packet, sizeof(tx_packet));

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, tx_packet_cid());
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
}

void test_final_reply_goes_to_the_owner_and_releases_the_channel(void)
{
    static const uint8_t reason   = 0x02;
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};

    start_owner_command();
    U2F_TRANSPORT_tx(
        &transport, U2F_COMMAND_HID_KEEP_ALIVE, &reason, 1, tx_packet, sizeof(tx_packet));
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    U2F_TRANSPORT_tx(
        &transport, U2F_COMMAND_HID_CBOR, reply, sizeof(reply), tx_packet, sizeof(tx_packet));

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, tx_packet_cid());
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR | 0x80, tx_packet[4]);
    // Released only once the host has the reply.
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    U2F_TRANSPORT_tx_done(&transport);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);
}

void test_broadcast_non_init_packet_keeps_the_owner(void)
{
    static const uint8_t get_info = 0x04;

    start_owner_command();
    rx_init_packet(U2F_BROADCAST_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_INVALID_CHANNEL, transport.reject_error);
    TEST_ASSERT_EQUAL_HEX32(U2F_BROADCAST_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.tx_cid);
}

// Control: once the owner has its reply, another channel can start a command.
void test_other_channel_is_accepted_after_the_final_reply(void)
{
    static const uint8_t get_info = 0x04;
    static const uint8_t reply[]  = {0x00, 0xA0};

    start_owner_command();
    U2F_TRANSPORT_tx(
        &transport, U2F_COMMAND_HID_CBOR, reply, sizeof(reply), tx_packet, sizeof(tx_packet));
    U2F_TRANSPORT_tx_done(&transport);

    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, transport.cid);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, transport.tx_cid);
}

// "Spurious continuation packets ... will be ignored": no reply, no change of owner.
void test_continuation_from_other_channel_is_ignored(void)
{
    start_owner_command();
    rx_continuation_packet(OTHER_CID, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.tx_cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

void test_owner_continuation_while_processing_is_ignored(void)
{
    start_owner_command();
    rx_continuation_packet(OWNER_CID, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

// A stray continuation on an idle device must not lock the device for other channels.
void test_idle_continuation_does_not_claim_the_channel(void)
{
    static const uint8_t get_info = 0x04;

    rx_continuation_packet(OTHER_CID, 0);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
}

// "A CTAPHID_CANCEL received ... on a non-active CID SHALL be ignored", and never answered.
void test_cancel_from_other_channel_is_ignored(void)
{
    start_owner_command();
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

void test_cancel_while_idle_is_ignored(void)
{
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);
}

// Control: the owner can still cancel its own command.
void test_owner_cancel_is_accepted(void)
{
    start_owner_command();
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CANCEL, rx_buffer[0]);
}

// Control: the owner's own continuation packets still complete its message.
void test_multi_packet_message_is_reassembled(void)
{
    uint8_t data[100] = {0x04};

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_FRAMING, transport.state);

    rx_continuation_packet(OWNER_CID, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
}

// A sender that stopped midway, as a message that announced 100 bytes but sent one packet.
static void start_incomplete_message(uint32_t cid)
{
    uint8_t data[100] = {0x04};

    rx_init_packet(cid, U2F_COMMAND_HID_CBOR, data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_FRAMING, transport.state);
    TEST_ASSERT_EQUAL_HEX32(cid, transport.cid);
}

// "If the device detects an INIT command during a transaction that has the same channel id
// as the active transaction, the transaction is aborted". The owner's new first packet
// already restarts its message; this guards it.
void test_owner_init_drops_its_incomplete_message(void)
{
    static const uint8_t nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    start_incomplete_message(OWNER_CID);
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce));

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_INIT, rx_buffer[0]);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.tx_cid);

    // What remained of the dropped message is now a stray continuation.
    rx_continuation_packet(OWNER_CID, 0);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
}

// A client that reopens the device starts with a broadcast INIT.
void test_broadcast_init_drops_an_incomplete_message(void)
{
    static const uint8_t nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    start_incomplete_message(OTHER_CID);
    rx_init_packet(U2F_BROADCAST_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce));

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_INIT, rx_buffer[0]);
    TEST_ASSERT_EQUAL_HEX32(U2F_BROADCAST_CID, transport.cid);
}

// Only the owner and the broadcast CID may do it; another allocated channel stays refused.
void test_other_channel_init_during_incomplete_message_is_refused(void)
{
    static const uint8_t nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    start_incomplete_message(OWNER_CID);
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce));

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_FRAMING, transport.state);
}

// Once the app has the command, a broadcast INIT is busy like any other channel.
void test_broadcast_init_during_processing_is_refused(void)
{
    static const uint8_t nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    start_owner_command();
    rx_init_packet(U2F_BROADCAST_CID, U2F_COMMAND_HID_INIT, nonce, sizeof(nonce));

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(U2F_BROADCAST_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

// Copilot on #1728: a waiting refusal must outlive the packets that follow it.
void test_refusal_survives_the_next_packet(void)
{
    uint8_t big[100] = {0x04};

    start_owner_command();
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CBOR, big, sizeof(big));
    rx_continuation_packet(OTHER_CID, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
}

// One refusal waits at a time: the first one is kept, later ones are dropped.
void test_first_refusal_is_kept(void)
{
    static const uint8_t get_info = 0x04;

    start_owner_command();
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    rx_init_packet(U2F_BROADCAST_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);

    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OTHER_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
}

// "keep the device locked for other channels until the last packet of the response message
// has been received": building the last packet is not enough.
void test_channel_stays_locked_until_the_reply_reaches_the_host(void)
{
    static const uint8_t get_info   = 0x04;
    uint8_t              reply[150] = {0};

    start_owner_command();
    U2F_TRANSPORT_tx(
        &transport, U2F_COMMAND_HID_CBOR, reply, sizeof(reply), tx_packet, sizeof(tx_packet));
    while (transport.tx_message_buffer) {
        U2F_TRANSPORT_tx(&transport, 0, NULL, 0, tx_packet, sizeof(tx_packet));
    }

    // Every packet is built, the last one is still on its way.
    rx_init_packet(OTHER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);

    U2F_TRANSPORT_tx_done(&transport);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);
}

void test_keepalive_done_keeps_the_channel(void)
{
    static const uint8_t reason = 0x02;

    start_owner_command();
    U2F_TRANSPORT_tx(
        &transport, U2F_COMMAND_HID_KEEP_ALIVE, &reason, 1, tx_packet, sizeof(tx_packet));
    U2F_TRANSPORT_tx_done(&transport);

    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

// The same channel's next command is refused until its reply's last packet reaches the host.
void test_command_during_the_reply_is_refused(void)
{
    static const uint8_t data     = 0xA1;
    static const uint8_t get_info = 0x04;

    // A PING answered by the transport, its one-packet reply still in flight.
    rx_init_packet(OWNER_CID, U2F_COMMAND_PING, &data, 1);
    transport.state = U2F_STATE_IDLE;
    U2F_TRANSPORT_tx(&transport, U2F_COMMAND_PING, &data, 1, tx_packet, sizeof(tx_packet));

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);

    U2F_TRANSPORT_tx_done(&transport);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);
}

// After its CANCEL is handed to the app, the owner's next command waits for the reply.
void test_owner_command_after_cancel_is_refused(void)
{
    static const uint8_t get_info = 0x04;

    start_owner_command();
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);
    transport.state = U2F_STATE_CMD_PROCESSING_CANCEL;

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    TEST_ASSERT_TRUE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.reject_cid);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_CHANNEL_BUSY, transport.reject_error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING_CANCEL, transport.state);
}

// CTAPHID_CANCEL has no data: one that needs continuation packets is ignored.
void test_cancel_spread_over_packets_is_ignored(void)
{
    uint8_t big[100] = {0};

    start_owner_command();
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, big, sizeof(big));

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
}

// Even a one-packet CANCEL must carry no data, which would land in the app's buffer.
void test_cancel_with_data_is_ignored(void)
{
    static const uint8_t data = 0xA1;

    start_owner_command();
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, &data, 1);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_PROCESSING, transport.state);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR, rx_buffer[0]);
}

// An oversized first packet from the owner must end its message: its continuation packets
// were otherwise copied against the refused length, past the end of the buffer.
void test_oversized_init_drops_the_message_being_received(void)
{
    uint8_t data[PACKET] = {0x04};

    start_incomplete_message(OWNER_CID);
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, data, 2000);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_INVALID_LENGTH, transport.error);
    TEST_ASSERT_NOT_EQUAL_UINT8(U2F_STATE_CMD_FRAMING, transport.state);
    TEST_ASSERT_LESS_OR_EQUAL_UINT16(sizeof(rx_buffer), transport.rx_message_length);

    for (uint8_t seq = 0; seq < 20; seq++) {
        rx_continuation_packet(OWNER_CID, seq);
        TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    }
    TEST_ASSERT_NOT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
}

// BCNT 0xFFFD-0xFFFF wrapped to a small length that passed the size check.
void test_largest_bcnt_is_refused(void)
{
    uint8_t data[PACKET] = {0};

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, data, 0xFFFF);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_INVALID_LENGTH, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);
}

// An empty CBOR from the owner also ends its message, which would go on as a CBOR.
void test_empty_cbor_drops_the_message_being_received(void)
{
    uint8_t data[100] = {0x01};

    rx_init_packet(OWNER_CID, U2F_COMMAND_PING, data, sizeof(data));
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, NULL, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP2_ERR_INVALID_CBOR, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);
    rx_continuation_packet(OWNER_CID, 0);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);
}

// Last line of defence: a length larger than the buffer never reaches the copy.
void test_continuation_never_writes_past_the_buffer(void)
{
    start_incomplete_message(OWNER_CID);
    transport.rx_message_buffer_size = 64;
    rx_continuation_packet(OWNER_CID, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_EQUAL_UINT16(PACKET - 4, transport.rx_message_offset);
}

// With no command in the app, a CANCEL is ignored: handed over, it held the channel with
// nothing left to answer it. The message being received goes on.
void test_owner_cancel_during_reception_is_ignored(void)
{
    start_incomplete_message(OWNER_CID);
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_FALSE(transport.reject_pending);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_FRAMING, transport.state);
    TEST_ASSERT_EQUAL_HEX8(U2F_COMMAND_HID_CBOR, rx_buffer[0]);

    rx_continuation_packet(OWNER_CID, 0);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
}

// The same while the transport's own answer is on its way.
void test_owner_cancel_during_a_transport_answer_is_ignored(void)
{
    static const uint8_t data = 0xA1;

    rx_init_packet(OWNER_CID, U2F_COMMAND_PING, &data, 1);
    transport.state = U2F_STATE_IDLE;
    U2F_TRANSPORT_tx(&transport, U2F_COMMAND_PING, &data, 1, tx_packet, sizeof(tx_packet));

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_IDLE, transport.state);

    U2F_TRANSPORT_tx_done(&transport);
    TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);
}

// Control: a second CANCEL for a command already cancelled is still handed over.
void test_owner_cancel_after_cancel_is_accepted(void)
{
    start_owner_command();
    transport.state = U2F_STATE_CMD_PROCESSING_CANCEL;
    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CANCEL, NULL, 0);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_COMPLETE, transport.state);
}

// A packet too short for its header is dropped before it can claim the channel: it got no
// answer, and held the channel against every other client.
void test_short_packets_do_not_claim_the_channel(void)
{
    static const uint8_t get_info                = 0x04;
    static const uint8_t short_packets[][PACKET] = {
        {0x22, 0x22, 0x22, 0x22},
        {0x22, 0x22, 0x22, 0x22, U2F_COMMAND_HID_CBOR | 0x80},
        {0x22, 0x22, 0x22, 0x22, U2F_COMMAND_HID_CBOR | 0x80, 0x00},
    };

    for (unsigned int i = 0; i < 3; i++) {
        uint8_t packet[PACKET];

        memcpy(packet, short_packets[i], sizeof(packet));
        U2F_TRANSPORT_rx(&transport, packet, (uint16_t) (4 + i));
        TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_OTHER, transport.error);
        TEST_ASSERT_FALSE(transport.reject_pending);
        TEST_ASSERT_EQUAL_HEX32(U2F_FORBIDDEN_CID, transport.cid);
    }

    rx_init_packet(OWNER_CID, U2F_COMMAND_HID_CBOR, &get_info, 1);
    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_HEX32(OWNER_CID, transport.cid);
}

// Control: a continuation needs only CID and SEQ, so a 5-byte one still counts.
void test_five_byte_continuation_is_accepted(void)
{
    uint8_t packet[PACKET] = {0x11, 0x11, 0x11, 0x11, 0x00};

    start_incomplete_message(OWNER_CID);
    U2F_TRANSPORT_rx(&transport, packet, 5);

    TEST_ASSERT_EQUAL_UINT8(CTAP1_ERR_SUCCESS, transport.error);
    TEST_ASSERT_EQUAL_UINT16(1, transport.rx_message_expected_sequence_number);
    TEST_ASSERT_EQUAL_UINT8(U2F_STATE_CMD_FRAMING, transport.state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_other_channel_is_refused_without_taking_the_reply_address);
    RUN_TEST(test_keepalive_keeps_the_channel);
    RUN_TEST(test_final_reply_goes_to_the_owner_and_releases_the_channel);
    RUN_TEST(test_broadcast_non_init_packet_keeps_the_owner);
    RUN_TEST(test_other_channel_is_accepted_after_the_final_reply);
    RUN_TEST(test_continuation_from_other_channel_is_ignored);
    RUN_TEST(test_owner_continuation_while_processing_is_ignored);
    RUN_TEST(test_idle_continuation_does_not_claim_the_channel);
    RUN_TEST(test_cancel_from_other_channel_is_ignored);
    RUN_TEST(test_cancel_while_idle_is_ignored);
    RUN_TEST(test_owner_cancel_is_accepted);
    RUN_TEST(test_multi_packet_message_is_reassembled);
    RUN_TEST(test_owner_init_drops_its_incomplete_message);
    RUN_TEST(test_broadcast_init_drops_an_incomplete_message);
    RUN_TEST(test_other_channel_init_during_incomplete_message_is_refused);
    RUN_TEST(test_broadcast_init_during_processing_is_refused);
    RUN_TEST(test_refusal_survives_the_next_packet);
    RUN_TEST(test_first_refusal_is_kept);
    RUN_TEST(test_channel_stays_locked_until_the_reply_reaches_the_host);
    RUN_TEST(test_keepalive_done_keeps_the_channel);
    RUN_TEST(test_command_during_the_reply_is_refused);
    RUN_TEST(test_owner_command_after_cancel_is_refused);
    RUN_TEST(test_cancel_spread_over_packets_is_ignored);
    RUN_TEST(test_cancel_with_data_is_ignored);
    RUN_TEST(test_oversized_init_drops_the_message_being_received);
    RUN_TEST(test_largest_bcnt_is_refused);
    RUN_TEST(test_empty_cbor_drops_the_message_being_received);
    RUN_TEST(test_continuation_never_writes_past_the_buffer);
    RUN_TEST(test_owner_cancel_during_reception_is_ignored);
    RUN_TEST(test_owner_cancel_during_a_transport_answer_is_ignored);
    RUN_TEST(test_owner_cancel_after_cancel_is_accepted);
    RUN_TEST(test_short_packets_do_not_claim_the_channel);
    RUN_TEST(test_five_byte_continuation_is_accepted);
    return UNITY_END();
}
