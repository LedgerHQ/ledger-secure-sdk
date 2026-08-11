#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "unity.h"

#include "buffer.h"

void test_buffer_get_cur(void)
{
    // clang-format off
    uint8_t temp[6] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55
    };
    buffer_t buf = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    uint8_t *result;

    result = buffer_get_cur(&buf);
    TEST_ASSERT_EQUAL_PTR(temp, result);

    buffer_seek_set(&buf, 3);
    result = buffer_get_cur(&buf);
    TEST_ASSERT_EQUAL_PTR(temp + 3, result);

    buffer_seek_set(&buf, 5);
    result = buffer_get_cur(&buf);
    TEST_ASSERT_EQUAL_PTR(temp + 5, result);
}

void test_buffer_can_read(void)
{
    uint8_t  temp[20] = {0};
    buffer_t buf      = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    TEST_ASSERT_TRUE(buffer_can_read(&buf, 20));

    TEST_ASSERT_TRUE(buffer_seek_cur(&buf, 20));
    TEST_ASSERT_FALSE(buffer_can_read(&buf, 1));
}

void test_buffer_seek(void)
{
    uint8_t  temp[20] = {0};
    buffer_t buf      = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    TEST_ASSERT_TRUE(buffer_can_read(&buf, 20));

    TEST_ASSERT_TRUE(buffer_seek_cur(&buf, 20));  // seek at offset 20
    TEST_ASSERT_FALSE(buffer_can_read(&buf, 1));  // can't read 1 byte
    TEST_ASSERT_FALSE(buffer_seek_cur(&buf, 1));  // can't move at offset 21

    TEST_ASSERT_TRUE(buffer_seek_end(&buf, 19));
    TEST_ASSERT_EQUAL_INT(buf.offset, 1);
    TEST_ASSERT_FALSE(buffer_seek_end(&buf, 21));  // can't seek at offset -1

    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 10));
    TEST_ASSERT_EQUAL_INT(buf.offset, 10);
    TEST_ASSERT_FALSE(buffer_seek_set(&buf, 21));  // can't seek at offset 21
}

void test_buffer_read(void)
{
    // clang-format off
    uint8_t temp[15] = {
        0xFF,
        0x01, 0x02,
        0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E
    };
    buffer_t buf = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    uint8_t first = 0;
    TEST_ASSERT_TRUE(buffer_read_u8(&buf, &first));
    TEST_ASSERT_EQUAL_INT(first, 255);                // 0xFF
    TEST_ASSERT_TRUE(buffer_seek_end(&buf, 0));       // seek at offset 19
    TEST_ASSERT_FALSE(buffer_read_u8(&buf, &first));  // can't read 1 byte

    uint16_t second = 0;
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 1));             // set back to offset 1
    TEST_ASSERT_TRUE(buffer_read_u16(&buf, &second, BE));   // big endian
    TEST_ASSERT_EQUAL_INT(second, 258);                     // 0x01 0x02
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 1));             // set back to offset 1
    TEST_ASSERT_TRUE(buffer_read_u16(&buf, &second, LE));   // little endian
    TEST_ASSERT_EQUAL_INT(second, 513);                     // 0x02 0x01
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 14));            // seek at offset 14
    TEST_ASSERT_FALSE(buffer_read_u16(&buf, &second, BE));  // can't read 2 bytes

    uint32_t third = 0;
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 3));            // set back to offset 3
    TEST_ASSERT_TRUE(buffer_read_u32(&buf, &third, BE));   // big endian
    TEST_ASSERT_EQUAL_INT(third, 50595078);                // 0x03 0x04 0x05 0x06
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 3));            // set back to offset 3
    TEST_ASSERT_TRUE(buffer_read_u32(&buf, &third, LE));   // little endian
    TEST_ASSERT_EQUAL_INT(third, 100992003);               // 0x06 0x05 0x04 0x03
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 12));           // seek at offset 12
    TEST_ASSERT_FALSE(buffer_read_u32(&buf, &third, BE));  // can't read 4 bytes

    uint64_t fourth = 0;
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 7));             // set back to offset 7
    TEST_ASSERT_TRUE(buffer_read_u64(&buf, &fourth, BE));   // big endian
    TEST_ASSERT_EQUAL_INT(fourth, 506664896818842894);      // 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 7));             // set back to offset 7
    TEST_ASSERT_TRUE(buffer_read_u64(&buf, &fourth, LE));   // little endian
    TEST_ASSERT_EQUAL_INT(fourth, 1012478732780767239);     // 0x0E 0x0D 0x0C 0x0B 0x0A 0x09 0x08 0x07
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 8));             // seek at offset 8
    TEST_ASSERT_FALSE(buffer_read_u64(&buf, &fourth, BE));  // can't read 8 bytes


    uint8_t bytes[32];

    memset(bytes, 0x42, sizeof(bytes));                // we use 0x42 as marker for data that is left unchanged

    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 7));             // set back to offset 7
    TEST_ASSERT_TRUE(buffer_read_bytes(&buf, bytes, 0));   // read zero bytes
    TEST_ASSERT_EQUAL_INT(bytes[0], 0x42);

    memset(bytes, 0x42, sizeof(bytes));
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 7));             // set back to offset 7
    TEST_ASSERT_TRUE(buffer_read_bytes(&buf, bytes, 1));
    TEST_ASSERT_EQUAL_INT(bytes[0], 0x07);
    TEST_ASSERT_EQUAL_INT(bytes[1], 0x42);

    memset(bytes, 0x42, sizeof(bytes));
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 7));             // set back to offset 7
    TEST_ASSERT_TRUE(buffer_read_bytes(&buf, bytes, 5));
    TEST_ASSERT_EQUAL_INT(bytes[0], 0x07);
    TEST_ASSERT_EQUAL_INT(bytes[1], 0x08);
    TEST_ASSERT_EQUAL_INT(bytes[2], 0x09);
    TEST_ASSERT_EQUAL_INT(bytes[3], 0x0A);
    TEST_ASSERT_EQUAL_INT(bytes[4], 0x0B);
    TEST_ASSERT_EQUAL_INT(bytes[5], 0x42);

    /* Verify buffer_read_bytes failure does not modify output buffer or offset */
    memset(bytes, 0xAA, sizeof(bytes));
    uint8_t bytes_before[sizeof(bytes)];
    memcpy(bytes_before, bytes, sizeof(bytes));
    size_t original_offset = buf.size - 1;
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, original_offset));
    TEST_ASSERT_FALSE(buffer_read_bytes(&buf, bytes, 2));    // request more bytes than available
    TEST_ASSERT_EQUAL_INT(buf.offset, original_offset);
    TEST_ASSERT_EQUAL_MEMORY(bytes, bytes_before, sizeof(bytes));

    // clang-format off
    uint8_t temp_varint[] = {
        0xFC, // 1 byte varint
        0xFD, 0x00, 0x01, // 2 bytes varint
        0xFE, 0x00, 0x01, 0x02, 0x03,  // 4 bytes varint
        0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 // 8 bytes varint
    };
    buffer_t buf_varint = {.ptr = temp_varint, .size = sizeof(temp_varint), .offset = 0};
    uint64_t varint = 0;
    TEST_ASSERT_TRUE(buffer_read_varint(&buf_varint, &varint));
    TEST_ASSERT_EQUAL_INT(varint, 0xFC);
    TEST_ASSERT_TRUE(buffer_read_varint(&buf_varint, &varint));
    TEST_ASSERT_EQUAL_INT(varint, 0x0100);
    TEST_ASSERT_TRUE(buffer_read_varint(&buf_varint, &varint));
    TEST_ASSERT_EQUAL_INT(varint, 0x03020100);
    TEST_ASSERT_TRUE(buffer_read_varint(&buf_varint, &varint));
    TEST_ASSERT_EQUAL_INT(varint, 0x0706050403020100);
    TEST_ASSERT_FALSE(buffer_read_varint(&buf_varint, &varint));
}

void test_buffer_copy(void) {
    uint8_t output[5] = {0};
    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    buffer_t buf = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    TEST_ASSERT_TRUE(buffer_copy(&buf, output, sizeof(output)));
    TEST_ASSERT_EQUAL_MEMORY(output, temp, sizeof(output));

    uint8_t output2[3] = {0};
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 2));
    TEST_ASSERT_TRUE(buffer_copy(&buf, output2, sizeof(output2)));
    TEST_ASSERT_EQUAL_MEMORY(output2, ((uint8_t[3]){0x03, 0x04, 0x05}), 3);
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 0));                      // seek at offset 0
    TEST_ASSERT_FALSE(buffer_copy(&buf, output2, sizeof(output2)));  // can't read 5 bytes
}

void test_buffer_move(void) {
    uint8_t output[5] = {0};
    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    buffer_t buf = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    TEST_ASSERT_TRUE(buffer_move(&buf, output, sizeof(output)));
    TEST_ASSERT_EQUAL_MEMORY(output, temp, sizeof(output));
    TEST_ASSERT_EQUAL_INT(buf.offset, sizeof(output));

    uint8_t output2[3] = {0};
    TEST_ASSERT_TRUE(buffer_seek_set(&buf, 0));                      // seek at offset 0
    TEST_ASSERT_FALSE(buffer_move(&buf, output2, sizeof(output2)));  // can't read 5 bytes
}

void test_buffer_peek(void) {
    uint8_t temp[6] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55
    };
    buffer_t buf = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    bool result;
    uint8_t c;

    result = buffer_peek(&buf, &c);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(c, 0x00);

    buf.offset += 3;

    result = buffer_peek(&buf, &c);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(c, 0x33);

    buf.offset += 2;
    result = buffer_peek(&buf, &c);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(c, 0x55);

    buf.offset += 1; // buffer is now empty
    result = buffer_peek(&buf, &c);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(c, 0x55); // unchanged because of failure
}

void test_buffer_peek_n(void) {
    uint8_t temp[6] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55
    };
    buffer_t buf = {.ptr = temp, .size = sizeof(temp), .offset = 0};

    bool result;
    uint8_t c;

    for (int i = 0; i < 6; i++) {
        result = buffer_peek_n(&buf, i, &c);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL_INT(c, temp[i]);
    }

    c = 42;
    result = buffer_peek_n(&buf, 6, &c); // past the end
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(c, 42); // c should not change on failure

    buf.offset += 3;

    for (int i = 0; i < 3; i++) {
        result = buffer_peek_n(&buf, i, &c);
        TEST_ASSERT_TRUE(result);
        TEST_ASSERT_EQUAL_INT(c, temp[3+i]);
    }

    c = 42;
    result = buffer_peek_n(&buf, 4, &c); // past the end
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(c, 42); // c should not change on failure
}


void test_buffer_write(void) {
    uint8_t template[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    uint8_t data[sizeof(template)];

    memcpy(data, template, sizeof(template));
    buffer_t buf = {.ptr = data, .size = sizeof(data), .offset = 0};

    // TEST buffer_write_u8
    TEST_ASSERT_TRUE(buffer_write_u8(&buf, 42));
    TEST_ASSERT_EQUAL_INT(data[0], 42);
    TEST_ASSERT_EQUAL_INT(data[1], 0x01);
    TEST_ASSERT_EQUAL_INT(buf.offset, 1);
    buffer_seek_end(&buf, 0);
    size_t old_offset = buf.offset;
    TEST_ASSERT_FALSE(buffer_write_u8(&buf, 42));
    TEST_ASSERT_EQUAL_INT(buf.offset, old_offset);                             // offset should not change on failed write
    TEST_ASSERT_EQUAL_INT(buf.offset, buf.size);
    buffer_seek_end(&buf, 1);
    TEST_ASSERT_TRUE(buffer_write_u8(&buf, 42));

    // reset data
    memcpy(data, template, sizeof(template));
    buffer_seek_set(&buf, 0);


    // TEST buffer_write_u16
    buffer_seek_set(&buf, 3);
    TEST_ASSERT_TRUE(buffer_write_u16(&buf, 0x3344, BE));
    TEST_ASSERT_EQUAL_INT(data[2], 0x02);
    TEST_ASSERT_EQUAL_INT(data[3], 0x33);
    TEST_ASSERT_EQUAL_INT(data[4], 0x44);
    TEST_ASSERT_EQUAL_INT(data[5], 0x05);
    TEST_ASSERT_EQUAL_INT(buf.offset, 5);
    buffer_seek_set(&buf, 3);
    TEST_ASSERT_TRUE(buffer_write_u16(&buf, 0x3344, LE));
    TEST_ASSERT_EQUAL_INT(data[2], 0x02);
    TEST_ASSERT_EQUAL_INT(data[3], 0x44);
    TEST_ASSERT_EQUAL_INT(data[4], 0x33);
    TEST_ASSERT_EQUAL_INT(data[5], 0x05);
    TEST_ASSERT_EQUAL_INT(buf.offset, 5);

    buffer_seek_end(&buf, 1);
    old_offset = buf.offset;
    TEST_ASSERT_FALSE(buffer_write_u16(&buf, 0x4242, BE));                     // not enough space
    TEST_ASSERT_EQUAL_INT(buf.offset, old_offset);                             // offset should not change on failed write
    TEST_ASSERT_EQUAL_INT(data[sizeof(data) - 1], template[sizeof(data) - 1]); // shouldn't change data if not enough space
    buffer_seek_end(&buf, 2);
    TEST_ASSERT_TRUE(buffer_write_u16(&buf, 0x4242, BE));                      // enough space this time

    // reset data
    memcpy(data, template, sizeof(template));
    buffer_seek_set(&buf, 0);


    // TEST buffer_write_u32
    buffer_seek_set(&buf, 3);
    TEST_ASSERT_TRUE(buffer_write_u32(&buf, 0x33445566, BE));
    TEST_ASSERT_EQUAL_INT(data[2], 0x02);
    TEST_ASSERT_EQUAL_INT(data[3], 0x33);
    TEST_ASSERT_EQUAL_INT(data[4], 0x44);
    TEST_ASSERT_EQUAL_INT(data[5], 0x55);
    TEST_ASSERT_EQUAL_INT(data[6], 0x66);
    TEST_ASSERT_EQUAL_INT(data[7], 0x07);
    TEST_ASSERT_EQUAL_INT(buf.offset, 7);
    buffer_seek_set(&buf, 3);
    TEST_ASSERT_TRUE(buffer_write_u32(&buf, 0x33445566, LE));
    TEST_ASSERT_EQUAL_INT(data[2], 0x02);
    TEST_ASSERT_EQUAL_INT(data[3], 0x66);
    TEST_ASSERT_EQUAL_INT(data[4], 0x55);
    TEST_ASSERT_EQUAL_INT(data[5], 0x44);
    TEST_ASSERT_EQUAL_INT(data[6], 0x33);
    TEST_ASSERT_EQUAL_INT(data[7], 0x07);
    TEST_ASSERT_EQUAL_INT(buf.offset, 7);

    buffer_seek_end(&buf, 3);
    old_offset = buf.offset;
    TEST_ASSERT_FALSE(buffer_write_u32(&buf, 0x42424242, BE));                 // not enough space
    TEST_ASSERT_EQUAL_INT(buf.offset, old_offset);                             // offset should not change on failed write
    TEST_ASSERT_EQUAL_INT(data[sizeof(data) - 1], template[sizeof(data) - 1]); // shouldn't change data if not enough space
    buffer_seek_end(&buf, 4);
    TEST_ASSERT_TRUE(buffer_write_u32(&buf, 0x42424242, BE));                  // enough space this time

    // reset data
    memcpy(data, template, sizeof(template));
    buffer_seek_set(&buf, 0);


    // TEST buffer_write_u64
    buffer_seek_set(&buf, 3);
    TEST_ASSERT_TRUE(buffer_write_u64(&buf, 0x33445566778899aaULL, BE));
    TEST_ASSERT_EQUAL_INT(data[2], 0x02);
    TEST_ASSERT_EQUAL_INT(data[3], 0x33);
    TEST_ASSERT_EQUAL_INT(data[4], 0x44);
    TEST_ASSERT_EQUAL_INT(data[5], 0x55);
    TEST_ASSERT_EQUAL_INT(data[6], 0x66);
    TEST_ASSERT_EQUAL_INT(data[7], 0x77);
    TEST_ASSERT_EQUAL_INT(data[8], 0x88);
    TEST_ASSERT_EQUAL_INT(data[9], 0x99);
    TEST_ASSERT_EQUAL_INT(data[10], 0xaa);
    TEST_ASSERT_EQUAL_INT(data[11], 0x0b);
    TEST_ASSERT_EQUAL_INT(buf.offset, 11);
    buffer_seek_set(&buf, 3);
    TEST_ASSERT_TRUE(buffer_write_u64(&buf, 0x33445566778899aaULL, LE));
    TEST_ASSERT_EQUAL_INT(data[2], 0x02);
    TEST_ASSERT_EQUAL_INT(data[3], 0xaa);
    TEST_ASSERT_EQUAL_INT(data[4], 0x99);
    TEST_ASSERT_EQUAL_INT(data[5], 0x88);
    TEST_ASSERT_EQUAL_INT(data[6], 0x77);
    TEST_ASSERT_EQUAL_INT(data[7], 0x66);
    TEST_ASSERT_EQUAL_INT(data[8], 0x55);
    TEST_ASSERT_EQUAL_INT(data[9], 0x44);
    TEST_ASSERT_EQUAL_INT(data[10], 0x33);
    TEST_ASSERT_EQUAL_INT(data[11], 0x0b);
    TEST_ASSERT_EQUAL_INT(buf.offset, 11);

    buffer_seek_end(&buf, 7);
    old_offset = buf.offset;
    TEST_ASSERT_FALSE(buffer_write_u64(&buf, 0x4242424242424242ULL, BE));      // not enough space
    TEST_ASSERT_EQUAL_INT(buf.offset, old_offset);                             // offset should not change on failed write
    TEST_ASSERT_EQUAL_INT(data[sizeof(data) - 1], template[sizeof(data) - 1]); // shouldn't change data if not enough space

    buffer_seek_end(&buf, 8);
    TEST_ASSERT_TRUE(buffer_write_u64(&buf, 0x4242424242424242ULL, BE));       // enough space this time

    // Tests for buffer_write_bytes
    uint8_t bytes_data[8] = {0};
    buffer_t bytes_buf = {.ptr = bytes_data, .size = sizeof(bytes_data), .offset = 0};
    // multiple-byte write should succeed and advance offset
    uint8_t src_multi[4] = {0xaa, 0xbb, 0xcc, 0xdd};
    TEST_ASSERT_TRUE(buffer_write_bytes(&bytes_buf, src_multi, sizeof(src_multi)));
    TEST_ASSERT_EQUAL_INT(bytes_buf.offset, (int)sizeof(src_multi));
    for (size_t i = 0; i < sizeof(src_multi); i++) {
        TEST_ASSERT_EQUAL_INT(bytes_data[i], src_multi[i]);
    }
    // zero-length write should be a no-op and succeed
    size_t offset_before = bytes_buf.offset;
    TEST_ASSERT_TRUE(buffer_write_bytes(&bytes_buf, src_multi, 0));
    TEST_ASSERT_EQUAL_INT(bytes_buf.offset, (int)offset_before);
    for (size_t i = 0; i < sizeof(src_multi); i++) {
        TEST_ASSERT_EQUAL_INT(bytes_data[i], src_multi[i]);
    }
    // insufficient space: write should fail and not modify buffer or offset
    uint8_t small_data[4] = {0x11, 0x22, 0x33, 0x44};
    buffer_t small_buf = {.ptr = small_data, .size = sizeof(small_data), .offset = 3};
    uint8_t src_too_large[2] = {0xaa, 0xbb};
    TEST_ASSERT_FALSE(buffer_write_bytes(&small_buf, src_too_large, sizeof(src_too_large)));
    TEST_ASSERT_EQUAL_INT(small_buf.offset, 3);
    TEST_ASSERT_EQUAL_INT(small_data[3], 0x44);  // last byte must remain unchanged
}

void test_buffer_get_path_bip32(void)
{
    // clang-format off
    uint8_t temp[] = {
        0x05,                            // length = 5
        0x80, 0x00, 0x00, 0x2C,          // 44'
        0x80, 0x00, 0x00, 0x01,          // 1'
        0x80, 0x00, 0x00, 0x00,          // 0'
        0x00, 0x00, 0x00, 0x00,          // 0
        0x00, 0x00, 0x00, 0x00,          // 0
        0xAB,                            // extra byte (should not be consumed)
    };
    // clang-format on

    buffer_t     buf         = {.ptr = temp, .size = sizeof(temp), .offset = 0};
    path_bip32_t bip32       = {0};
    uint32_t     expected[5] = {0x8000002C, 0x80000001, 0x80000000, 0, 0};

    TEST_ASSERT_TRUE(buffer_get_path_bip32(&buf, &bip32));
    TEST_ASSERT_EQUAL_INT(bip32.length, 5);
    TEST_ASSERT_EQUAL_MEMORY(bip32.path, expected, 5 * sizeof(uint32_t));
    // offset must advance past length byte (1) + 5 components * 4 bytes = 21
    TEST_ASSERT_EQUAL_INT(buf.offset, 21);
    // extra byte is still available
    TEST_ASSERT_TRUE(buffer_can_read(&buf, 1));
}

void test_bad_buffer_get_path_bip32(void)
{
    // clang-format off
    uint8_t temp[21] = {
        0x05,
        0x80, 0x00, 0x00, 0x2C,
        0x80, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    // clang-format on

    buffer_t     buf   = {.ptr = temp, .size = sizeof(temp), .offset = 0};
    path_bip32_t bip32 = {0};

    // NULL buffer
    TEST_ASSERT_FALSE(buffer_get_path_bip32(NULL, &bip32));

    // NULL path output
    TEST_ASSERT_FALSE(buffer_get_path_bip32(&buf, NULL));

    // Empty buffer (no bytes at all)
    buffer_t empty = {.ptr = temp, .size = 0, .offset = 0};
    TEST_ASSERT_FALSE(buffer_get_path_bip32(&empty, &bip32));

    // Buffer fully consumed (offset == size)
    buf.offset = buf.size;
    TEST_ASSERT_FALSE(buffer_get_path_bip32(&buf, &bip32));
    buf.offset = 0;

    // Length byte present but not enough data for the declared components
    uint8_t  short_data[] = {0x05, 0x80, 0x00, 0x00};  // only 3 bytes of path
    buffer_t short_buf    = {.ptr = short_data, .size = sizeof(short_data), .offset = 0};
    TEST_ASSERT_FALSE(buffer_get_path_bip32(&short_buf, &bip32));

    // length == 0 (rejected by bip32_path_read)
    uint8_t  zero_len[] = {0x00};
    buffer_t zero_buf   = {.ptr = zero_len, .size = sizeof(zero_len), .offset = 0};
    TEST_ASSERT_FALSE(buffer_get_path_bip32(&zero_buf, &bip32));

    // length > MAX_BIP32_PATH (rejected by bip32_path_read even with enough data)
    uint8_t too_long[1 + (MAX_BIP32_PATH + 1) * 4];
    memset(too_long, 0, sizeof(too_long));
    too_long[0]           = MAX_BIP32_PATH + 1;
    buffer_t too_long_buf = {.ptr = too_long, .size = sizeof(too_long), .offset = 0};
    TEST_ASSERT_FALSE(buffer_get_path_bip32(&too_long_buf, &bip32));
}

void test_buffer_create(void)
{
    uint8_t data[32];

    buffer_t buffer = buffer_create(data, 15);

    TEST_ASSERT_EQUAL_PTR(buffer.ptr, data);
    TEST_ASSERT_EQUAL_INT(buffer.size, 15);
    TEST_ASSERT_EQUAL_INT(buffer.offset, 0);
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_buffer_get_cur);
    RUN_TEST(test_buffer_can_read);
    RUN_TEST(test_buffer_seek);
    RUN_TEST(test_buffer_read);
    RUN_TEST(test_buffer_copy);
    RUN_TEST(test_buffer_move);
    RUN_TEST(test_buffer_peek);
    RUN_TEST(test_buffer_peek_n);
    RUN_TEST(test_buffer_write);
    RUN_TEST(test_buffer_get_path_bip32);
    RUN_TEST(test_bad_buffer_get_path_bip32);
    RUN_TEST(test_buffer_create);
    return UNITY_END();
}
