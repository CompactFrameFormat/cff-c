#include "cff.h"
#include "unity.h"
#include <string.h>

// Test helper callback for cff_parse_frames
static int callback_count = 0;
static cff_frame_t captured_frames[10];

void frame_callback(const cff_frame_t *frame)
{
    if (frame != NULL && callback_count < 10) {
        captured_frames[callback_count] = *frame;
        callback_count++;
    }
}

void setUp(void)
{
    // Reset callback state before each test
    callback_count = 0;
    memset(captured_frames, 0, sizeof(captured_frames));
}

void tearDown(void)
{
    // Clean up code to run after each test
}

// Helper function to build a test frame
static size_t build_test_frame(uint8_t *buffer, size_t buffer_size, const char *payload)
{
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, buffer_size);

    size_t payload_size = payload ? strlen(payload) : 0;
    cff_build_frame(&builder, (const uint8_t *) payload, payload_size);

    return cff_calculate_frame_size_bytes(payload_size);
}

void test_parse_frame_success(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    cff_frame_t parsed_frame;
    size_t consumed_bytes;

    cff_error_en_t result = cff_parse_frame(frame_buffer, frame_size, &parsed_frame, &consumed_bytes);

    TEST_ASSERT_EQUAL(cff_error_none, result);
    TEST_ASSERT_EQUAL(frame_size, consumed_bytes);
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_0, parsed_frame.header.preamble[0]);
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_1, parsed_frame.header.preamble[1]);
    TEST_ASSERT_EQUAL(0, parsed_frame.header.frame_counter);
    TEST_ASSERT_EQUAL(strlen(test_payload), parsed_frame.header.payload_size_bytes);
    TEST_ASSERT_EQUAL_MEMORY(test_payload, parsed_frame.payload, strlen(test_payload));
}

void test_parse_frame_null_pointers(void)
{
    uint8_t frame_buffer[100];
    cff_frame_t parsed_frame;
    size_t consumed_bytes;

    // Test NULL buffer
    cff_error_en_t result1 = cff_parse_frame(NULL, 100, &parsed_frame, &consumed_bytes);
    TEST_ASSERT_EQUAL(cff_error_null_pointer, result1);

    // Test NULL frame
    cff_error_en_t result2 = cff_parse_frame(frame_buffer, 100, NULL, &consumed_bytes);
    TEST_ASSERT_EQUAL(cff_error_null_pointer, result2);
}

void test_parse_frame_incomplete_frame(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    cff_frame_t parsed_frame;
    size_t consumed_bytes;

    // Try to parse with insufficient buffer size
    cff_error_en_t result = cff_parse_frame(frame_buffer, frame_size - 1, &parsed_frame, &consumed_bytes);

    TEST_ASSERT_EQUAL(cff_error_incomplete_frame, result);
}

void test_parse_frame_invalid_preamble(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    // Corrupt the preamble
    frame_buffer[0] = 0x00;

    cff_frame_t parsed_frame;
    size_t consumed_bytes;

    cff_error_en_t result = cff_parse_frame(frame_buffer, sizeof(frame_buffer), &parsed_frame, &consumed_bytes);

    TEST_ASSERT_EQUAL(cff_error_invalid_preamble, result);
}

void test_parse_frame_invalid_header_crc(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    // Corrupt the header CRC
    frame_buffer[6] = 0x00;
    frame_buffer[7] = 0x00;

    cff_frame_t parsed_frame;
    size_t consumed_bytes;

    cff_error_en_t result = cff_parse_frame(frame_buffer, sizeof(frame_buffer), &parsed_frame, &consumed_bytes);

    TEST_ASSERT_EQUAL(cff_error_invalid_header_crc, result);
}

void test_parse_frame_invalid_payload_crc(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    // Corrupt the payload CRC (last 2 bytes of frame)
    frame_buffer[frame_size - 2] = 0x00;
    frame_buffer[frame_size - 1] = 0x00;

    cff_frame_t parsed_frame;
    size_t consumed_bytes;

    cff_error_en_t result = cff_parse_frame(frame_buffer, frame_size, &parsed_frame, &consumed_bytes);

    TEST_ASSERT_EQUAL(cff_error_invalid_payload_crc, result);
}

void test_binary_payload_all_byte_values(void)
{
    uint8_t frame_buffer[300]; // Large enough for 256 byte payload + frame overhead
    cff_frame_builder_t builder;
    cff_error_en_t init_result = cff_frame_builder_init(&builder, frame_buffer, sizeof(frame_buffer));
    TEST_ASSERT_EQUAL(cff_error_none, init_result);

    // Create binary payload with all possible byte values
    uint8_t binary_data[256];
    for (int i = 0; i < 256; i++) {
        binary_data[i] = (uint8_t) i;
    }

    cff_error_en_t build_result = cff_build_frame(&builder, binary_data, sizeof(binary_data));
    TEST_ASSERT_EQUAL(cff_error_none, build_result);

    if (build_result == cff_error_none) {
        size_t frame_size = cff_calculate_frame_size_bytes(sizeof(binary_data));

        cff_frame_t parsed_frame;
        size_t consumed_bytes;
        cff_error_en_t parse_result = cff_parse_frame(frame_buffer, frame_size, &parsed_frame, &consumed_bytes);
        TEST_ASSERT_EQUAL(cff_error_none, parse_result);

        if (parse_result == cff_error_none) {
            // Verify payload size
            TEST_ASSERT_EQUAL(256, parsed_frame.header.payload_size_bytes);

            // Verify binary payload integrity - all 256 byte values preserved
            TEST_ASSERT_EQUAL_MEMORY(binary_data, parsed_frame.payload, 256);

            // Verify specific boundary values within the payload
            TEST_ASSERT_EQUAL_HEX8(0x00, parsed_frame.payload[0]);   // Minimum value
            TEST_ASSERT_EQUAL_HEX8(0xFF, parsed_frame.payload[255]); // Maximum value
            TEST_ASSERT_EQUAL_HEX8(0x7F, parsed_frame.payload[127]); // Mid-range value
            TEST_ASSERT_EQUAL_HEX8(0x80, parsed_frame.payload[128]); // Mid-range value + 1
        }
    }
}

void test_special_byte_sequences_in_payload(void)
{
    uint8_t frame_buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, frame_buffer, sizeof(frame_buffer));

    // Test payload containing frame preamble bytes (should not confuse parser)
    uint8_t special_payload[] = {CFF_PREAMBLE_BYTE_0, CFF_PREAMBLE_BYTE_1, 0x00, 0x01, 0x02};

    cff_error_en_t build_result = cff_build_frame(&builder, special_payload, sizeof(special_payload));
    TEST_ASSERT_EQUAL(cff_error_none, build_result);

    size_t frame_size = cff_calculate_frame_size_bytes(sizeof(special_payload));

    cff_frame_t parsed_frame;
    size_t consumed_bytes;
    cff_error_en_t parse_result = cff_parse_frame(frame_buffer, frame_size, &parsed_frame, &consumed_bytes);
    TEST_ASSERT_EQUAL(cff_error_none, parse_result);

    // Verify the special bytes in payload are preserved
    TEST_ASSERT_EQUAL(sizeof(special_payload), parsed_frame.header.payload_size_bytes);
    TEST_ASSERT_EQUAL_MEMORY(special_payload, parsed_frame.payload, sizeof(special_payload));
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_0, parsed_frame.payload[0]);
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_1, parsed_frame.payload[1]);
}

void test_parse_frames_single_frame(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    size_t frames_parsed = cff_parse_frames(frame_buffer, frame_size, frame_callback);

    TEST_ASSERT_EQUAL(1, frames_parsed);
    TEST_ASSERT_EQUAL(1, callback_count);
    TEST_ASSERT_EQUAL(strlen(test_payload), captured_frames[0].header.payload_size_bytes);
    TEST_ASSERT_EQUAL_MEMORY(test_payload, captured_frames[0].payload, strlen(test_payload));
}

void test_parse_frames_multiple_frames(void)
{
    uint8_t multi_frame_buffer[200];
    size_t total_size = 0;

    // Build first frame
    uint8_t frame1_buffer[100];
    const char *payload1 = "Hello";
    size_t frame1_size = build_test_frame(frame1_buffer, sizeof(frame1_buffer), payload1);
    memcpy(multi_frame_buffer, frame1_buffer, frame1_size);
    total_size += frame1_size;

    // Build second frame
    uint8_t frame2_buffer[100];
    const char *payload2 = "World";
    size_t frame2_size = build_test_frame(frame2_buffer, sizeof(frame2_buffer), payload2);
    memcpy(multi_frame_buffer + total_size, frame2_buffer, frame2_size);
    total_size += frame2_size;

    size_t frames_parsed = cff_parse_frames(multi_frame_buffer, total_size, frame_callback);

    TEST_ASSERT_EQUAL(2, frames_parsed);
    TEST_ASSERT_EQUAL(2, callback_count);

    // Verify first frame
    TEST_ASSERT_EQUAL(strlen(payload1), captured_frames[0].header.payload_size_bytes);
    TEST_ASSERT_EQUAL_MEMORY(payload1, captured_frames[0].payload, strlen(payload1));

    // Verify second frame
    TEST_ASSERT_EQUAL(strlen(payload2), captured_frames[1].header.payload_size_bytes);
    TEST_ASSERT_EQUAL_MEMORY(payload2, captured_frames[1].payload, strlen(payload2));
}

void test_parse_frames_null_pointers(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    // Test NULL buffer
    size_t result1 = cff_parse_frames(NULL, frame_size, frame_callback);
    TEST_ASSERT_EQUAL(0, result1);

    // Test NULL callback
    size_t result2 = cff_parse_frames(frame_buffer, frame_size, NULL);
    TEST_ASSERT_EQUAL(0, result2);
}

void test_parse_frames_partial_data(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    // Parse with partial frame data
    size_t frames_parsed = cff_parse_frames(frame_buffer, frame_size - 5, frame_callback);

    TEST_ASSERT_EQUAL(0, frames_parsed);
    TEST_ASSERT_EQUAL(0, callback_count);
}

void test_parse_frames_corrupted_data_recovery(void)
{
    uint8_t multi_frame_buffer[200];
    size_t total_size = 0;

    // Build first frame
    uint8_t frame1_buffer[100];
    const char *payload1 = "Hello";
    size_t frame1_size = build_test_frame(frame1_buffer, sizeof(frame1_buffer), payload1);
    memcpy(multi_frame_buffer, frame1_buffer, frame1_size);
    total_size += frame1_size;

    // Build second frame
    uint8_t frame2_buffer[100];
    const char *payload2 = "World";
    size_t frame2_size = build_test_frame(frame2_buffer, sizeof(frame2_buffer), payload2);
    memcpy(multi_frame_buffer + total_size, frame2_buffer, frame2_size);
    total_size += frame2_size;

    // Corrupt the second frame's preamble
    multi_frame_buffer[frame1_size + 1] = 0x00;

    size_t frames_parsed = cff_parse_frames(multi_frame_buffer, total_size, frame_callback);

    // Should parse first frame and skip corrupted second frame
    TEST_ASSERT_EQUAL(1, frames_parsed);
    TEST_ASSERT_EQUAL(1, callback_count);
    TEST_ASSERT_EQUAL_MEMORY(payload1, captured_frames[0].payload, strlen(payload1));
}

void test_parse_frames_empty_buffer(void)
{
    uint8_t empty_buffer[5] = {0}; // Too small for any frame

    size_t frames_parsed = cff_parse_frames(empty_buffer, sizeof(empty_buffer), frame_callback);

    TEST_ASSERT_EQUAL(0, frames_parsed);
    TEST_ASSERT_EQUAL(0, callback_count);
}

void test_parse_frames_small_buffer(void)
{
    uint8_t frame_buffer[100];
    const char *test_payload = "Hello";
    size_t frame_size = build_test_frame(frame_buffer, sizeof(frame_buffer), test_payload);

    // Test parsing with every buffer size from 1 up to frame_size - 1
    for (size_t buffer_size = 1; buffer_size < frame_size; buffer_size++) {
        // Reset callback state for each iteration
        callback_count = 0;
        memset(captured_frames, 0, sizeof(captured_frames));

        size_t frames_parsed = cff_parse_frames(frame_buffer, buffer_size, frame_callback);

        // All partial buffer sizes should result in 0 frames parsed
        TEST_ASSERT_EQUAL(0, frames_parsed);
        TEST_ASSERT_EQUAL(0, callback_count);
    }
}
