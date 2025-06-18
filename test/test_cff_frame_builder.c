#include "cff.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
    // Set up code to run before each test
}

void tearDown(void)
{
    // Clean up code to run after each test
}

void test_frame_builder_init_success(void)
{
    uint8_t buffer[1024];
    cff_frame_builder_t builder;

    cff_error_en_t result = cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(cff_error_none, result);
    TEST_ASSERT_EQUAL_PTR(buffer, builder.buffer);
    TEST_ASSERT_EQUAL(sizeof(buffer), builder.buffer_size_bytes);
    TEST_ASSERT_EQUAL(0, builder.frame_counter);
}

void test_frame_builder_init_null_builder(void)
{
    uint8_t buffer[1024];

    cff_error_en_t result = cff_frame_builder_init(NULL, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL(cff_error_null_pointer, result);
}

void test_frame_builder_init_null_buffer(void)
{
    cff_frame_builder_t builder;

    cff_error_en_t result = cff_frame_builder_init(&builder, NULL, 1024);

    TEST_ASSERT_EQUAL(cff_error_null_pointer, result);
}

void test_frame_builder_init_buffer_too_small(void)
{
    uint8_t small_buffer[5];
    cff_frame_builder_t builder;

    cff_error_en_t result = cff_frame_builder_init(&builder, small_buffer, sizeof(small_buffer));

    TEST_ASSERT_EQUAL(cff_error_buffer_too_small, result);
}

void test_build_frame_empty_payload(void)
{
    uint8_t buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    // Use a valid pointer for empty payload (size 0)
    uint8_t dummy_byte = 0;
    cff_error_en_t result = cff_build_frame(&builder, &dummy_byte, 0);

    TEST_ASSERT_EQUAL(cff_error_none, result);

    // Verify frame structure
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_0, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_1, buffer[1]);

    // Frame counter should be 0 for first frame
    uint16_t frame_counter = (uint16_t) (buffer[2] | (buffer[3] << 8));
    TEST_ASSERT_EQUAL(0, frame_counter);

    // Payload size should be 0
    uint16_t payload_size = (uint16_t) (buffer[4] | (buffer[5] << 8));
    TEST_ASSERT_EQUAL(0, payload_size);
}

void test_build_frame_with_payload(void)
{
    uint8_t buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    const char *test_payload = "Hello";
    size_t payload_size = strlen(test_payload);

    cff_error_en_t result = cff_build_frame(&builder, (const uint8_t *) test_payload, payload_size);

    TEST_ASSERT_EQUAL(cff_error_none, result);

    // Verify frame structure
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_0, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_1, buffer[1]);

    // Payload size should match
    uint16_t stored_payload_size = (uint16_t) (buffer[4] | (buffer[5] << 8));
    TEST_ASSERT_EQUAL(payload_size, stored_payload_size);

    // Verify payload data
    TEST_ASSERT_EQUAL_MEMORY(test_payload, &buffer[CFF_HEADER_SIZE_BYTES], payload_size);
}

void test_build_frame_increments_counter(void)
{
    uint8_t buffer[200];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    const char *payload1 = "First";
    const char *payload2 = "Second";

    // Build first frame
    cff_error_en_t result1 = cff_build_frame(&builder, (const uint8_t *) payload1, strlen(payload1));
    TEST_ASSERT_EQUAL(cff_error_none, result1);

    uint16_t counter1 = (uint16_t) (buffer[2] | (buffer[3] << 8));
    TEST_ASSERT_EQUAL(0, counter1);

    // Build second frame (should increment counter)
    cff_error_en_t result2 = cff_build_frame(&builder, (const uint8_t *) payload2, strlen(payload2));
    TEST_ASSERT_EQUAL(cff_error_none, result2);

    uint16_t counter2 = (uint16_t) (buffer[2] | (buffer[3] << 8));
    TEST_ASSERT_EQUAL(1, counter2);
}

void test_build_frame_null_builder(void)
{
    const char *test_payload = "Test";

    cff_error_en_t result = cff_build_frame(NULL, (const uint8_t *) test_payload, strlen(test_payload));

    TEST_ASSERT_EQUAL(cff_error_null_pointer, result);
}

void test_build_frame_null_payload_with_size(void)
{
    uint8_t buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    cff_error_en_t result = cff_build_frame(&builder, NULL, 5);

    TEST_ASSERT_EQUAL(cff_error_null_pointer, result);
}

void test_build_frame_payload_too_large(void)
{
    uint8_t buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    const char *test_payload = "Test";

    cff_error_en_t result = cff_build_frame(&builder, (const uint8_t *) test_payload, CFF_MAX_PAYLOAD_SIZE_BYTES + 1);

    TEST_ASSERT_EQUAL(cff_error_payload_too_large, result);
}

void test_build_frame_buffer_too_small(void)
{
    uint8_t small_buffer[15]; // Too small for frame + large payload
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, small_buffer, sizeof(small_buffer));

    const char *large_payload = "This payload is too large for the buffer";

    cff_error_en_t result = cff_build_frame(&builder, (const uint8_t *) large_payload, strlen(large_payload));

    TEST_ASSERT_EQUAL(cff_error_buffer_too_small, result);
}

void test_frame_builder_buffer_access(void)
{
    uint8_t buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, buffer, sizeof(buffer));

    // Test that the builder points to our buffer
    TEST_ASSERT_EQUAL_PTR(buffer, builder.buffer);
    TEST_ASSERT_EQUAL(sizeof(buffer), builder.buffer_size_bytes);
    TEST_ASSERT_EQUAL(0, builder.frame_counter);

    // Build a frame and verify it was written to our buffer
    const char *test_payload = "Hello";
    cff_error_en_t result = cff_build_frame(&builder, (const uint8_t *) test_payload, strlen(test_payload));
    TEST_ASSERT_EQUAL(cff_error_none, result);

    // Verify frame structure in buffer
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_0, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_1, buffer[1]);

    // Verify payload in buffer
    TEST_ASSERT_EQUAL_MEMORY(test_payload, &buffer[CFF_HEADER_SIZE_BYTES], strlen(test_payload));
}

void test_frame_counter_rollover_sequence(void)
{
    uint8_t frame_buffer[100];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, frame_buffer, sizeof(frame_buffer));

    // Manually set counter near maximum
    builder.frame_counter = 65534;

    const char *payload = "test";

    // Build frame with counter = 65534
    cff_error_en_t result1 = cff_build_frame(&builder, (const uint8_t *) payload, strlen(payload));
    TEST_ASSERT_EQUAL(cff_error_none, result1);

    // Parse and verify counter
    cff_frame_t frame1;
    size_t consumed_bytes1;
    size_t frame_size = cff_calculate_frame_size_bytes(strlen(payload));
    cff_error_en_t parse_result1 = cff_parse_frame(frame_buffer, frame_size, &frame1, &consumed_bytes1);
    TEST_ASSERT_EQUAL(cff_error_none, parse_result1);
    TEST_ASSERT_EQUAL(65534, frame1.header.frame_counter);

    // Build frame with counter = 65535
    cff_error_en_t result2 = cff_build_frame(&builder, (const uint8_t *) payload, strlen(payload));
    TEST_ASSERT_EQUAL(cff_error_none, result2);

    cff_frame_t frame2;
    size_t consumed_bytes2;
    cff_error_en_t parse_result2 = cff_parse_frame(frame_buffer, frame_size, &frame2, &consumed_bytes2);
    TEST_ASSERT_EQUAL(cff_error_none, parse_result2);
    TEST_ASSERT_EQUAL(65535, frame2.header.frame_counter);

    // Build frame with counter = 0 (after rollover)
    cff_error_en_t result3 = cff_build_frame(&builder, (const uint8_t *) payload, strlen(payload));
    TEST_ASSERT_EQUAL(cff_error_none, result3);

    cff_frame_t frame3;
    size_t consumed_bytes3;
    cff_error_en_t parse_result3 = cff_parse_frame(frame_buffer, frame_size, &frame3, &consumed_bytes3);
    TEST_ASSERT_EQUAL(cff_error_none, parse_result3);
    TEST_ASSERT_EQUAL(0, frame3.header.frame_counter);
}

void test_maximum_payload_size_calculation(void)
{
    // Test that we can calculate frame size for maximum payload
    size_t max_frame_size = cff_calculate_frame_size_bytes(CFF_MAX_PAYLOAD_SIZE_BYTES);
    size_t expected_max_size = CFF_HEADER_SIZE_BYTES + CFF_MAX_PAYLOAD_SIZE_BYTES + CFF_PAYLOAD_CRC_SIZE_BYTES;
    TEST_ASSERT_EQUAL(expected_max_size, max_frame_size);

    // Verify the calculation is consistent
    TEST_ASSERT_GREATER_THAN(CFF_MIN_FRAME_SIZE_BYTES, max_frame_size);
    TEST_ASSERT_EQUAL(CFF_MAX_PAYLOAD_SIZE_BYTES, max_frame_size - CFF_HEADER_SIZE_BYTES - CFF_PAYLOAD_CRC_SIZE_BYTES);
}

void test_frame_builder_with_large_buffer(void)
{
    // Test frame builder initialization with a large buffer
    uint8_t large_buffer[1000];
    cff_frame_builder_t builder;

    cff_error_en_t result = cff_frame_builder_init(&builder, large_buffer, sizeof(large_buffer));
    TEST_ASSERT_EQUAL(cff_error_none, result);
    TEST_ASSERT_EQUAL_PTR(large_buffer, builder.buffer);
    TEST_ASSERT_EQUAL(sizeof(large_buffer), builder.buffer_size_bytes);
    TEST_ASSERT_EQUAL(0, builder.frame_counter);
}

void test_sequential_frames_with_different_payloads(void)
{
    uint8_t frame_buffer[200];
    cff_frame_builder_t builder;
    cff_frame_builder_init(&builder, frame_buffer, sizeof(frame_buffer));

    // Test different payload sizes in sequence
    const char *payloads[] = {"", "A", "Hello", "This is a longer test payload"};
    size_t num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (size_t i = 0; i < num_payloads; i++) {
        size_t payload_size = strlen(payloads[i]);

        cff_error_en_t build_result = cff_build_frame(&builder, (const uint8_t *) payloads[i], payload_size);
        TEST_ASSERT_EQUAL(cff_error_none, build_result);

        size_t frame_size = cff_calculate_frame_size_bytes(payload_size);

        cff_frame_t parsed_frame;
        size_t consumed_bytes;
        cff_error_en_t parse_result = cff_parse_frame(frame_buffer, frame_size, &parsed_frame, &consumed_bytes);
        TEST_ASSERT_EQUAL(cff_error_none, parse_result);

        // Verify frame counter increments
        TEST_ASSERT_EQUAL(i, parsed_frame.header.frame_counter);

        // Verify payload
        TEST_ASSERT_EQUAL(payload_size, parsed_frame.header.payload_size_bytes);
        if (payload_size > 0) {
            TEST_ASSERT_EQUAL_MEMORY(payloads[i], parsed_frame.payload, payload_size);
        }
    }
}

void test_zero_and_max_counter_values(void)
{
    uint8_t frame_buffer[50];
    cff_frame_builder_t builder;

    // Test with counter = 0 (initial value)
    cff_frame_builder_init(&builder, frame_buffer, sizeof(frame_buffer));
    builder.frame_counter = 0;

    const char *payload = "zero";
    cff_error_en_t result1 = cff_build_frame(&builder, (const uint8_t *) payload, strlen(payload));
    TEST_ASSERT_EQUAL(cff_error_none, result1);

    size_t frame_size = cff_calculate_frame_size_bytes(strlen(payload));
    cff_frame_t frame1;
    size_t consumed_bytes1;
    cff_error_en_t parse_result1 = cff_parse_frame(frame_buffer, frame_size, &frame1, &consumed_bytes1);
    TEST_ASSERT_EQUAL(cff_error_none, parse_result1);
    TEST_ASSERT_EQUAL(0, frame1.header.frame_counter);

    // Test with counter = maximum value
    builder.frame_counter = 65535;
    payload = "max";

    cff_error_en_t result2 = cff_build_frame(&builder, (const uint8_t *) payload, strlen(payload));
    TEST_ASSERT_EQUAL(cff_error_none, result2);

    frame_size = cff_calculate_frame_size_bytes(strlen(payload));
    cff_frame_t frame2;
    size_t consumed_bytes2;
    cff_error_en_t parse_result2 = cff_parse_frame(frame_buffer, frame_size, &frame2, &consumed_bytes2);
    TEST_ASSERT_EQUAL(cff_error_none, parse_result2);
    TEST_ASSERT_EQUAL(65535, frame2.header.frame_counter);
}
