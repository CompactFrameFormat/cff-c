#include "cff.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test frame files array - shared across tests
static const char *test_frame_files[] = {"test/support/01_empty_payload.bin", "test/support/02_simple_text.bin",
                                         "test/support/03_binary_data.bin",   "test/support/04_large_text.bin",
                                         "test/support/05_json-like.bin",     "test/support/06_with_nulls.bin",
                                         "test/support/07_all_spaces.bin",    "test/support/08_numbers.bin"};
static const size_t num_test_frame_files = sizeof(test_frame_files) / sizeof(test_frame_files[0]);

// Test data structure to hold extracted payloads
typedef struct {
    uint8_t *data;
    size_t size;
    uint16_t frame_counter;
} payload_info_t;

// Storage for test data
static payload_info_t extracted_payloads[10];
static size_t num_extracted_payloads = 0;
static uint8_t *stream_buffer = NULL;
static size_t stream_size = 0;

void setUp(void)
{
    // Clear extracted payloads
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        if (extracted_payloads[i].data) {
            free(extracted_payloads[i].data);
        }
    }
    memset(extracted_payloads, 0, sizeof(extracted_payloads));
    num_extracted_payloads = 0;

    // Clear stream buffer
    if (stream_buffer) {
        free(stream_buffer);
        stream_buffer = NULL;
    }
    stream_size = 0;
}

void tearDown(void)
{
    // Clean up resources
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        if (extracted_payloads[i].data) {
            free(extracted_payloads[i].data);
        }
    }
    memset(extracted_payloads, 0, sizeof(extracted_payloads));
    num_extracted_payloads = 0;

    if (stream_buffer) {
        free(stream_buffer);
        stream_buffer = NULL;
    }
    stream_size = 0;
}

//! @brief Helper function to read a binary file into memory
//!
//! @param filename Path to the file to read
//! @param file_size Pointer to store the size of the file in bytes
//! @return Pointer to allocated buffer containing file data, or NULL on failure
//!
//! The caller is responsible for freeing the returned buffer.
static uint8_t *read_binary_file(const char *filename, size_t *file_size)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        fclose(file);
        return NULL;
    }

    // Allocate buffer and read file
    uint8_t *buffer = malloc((size_t) size);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, (size_t) size, file);
    fclose(file);

    if (bytes_read != (size_t) size) {
        free(buffer);
        return NULL;
    }

    *file_size = (size_t) size;
    return buffer;
}

//! @brief Helper function to store payload information for later comparison
//!
//! @param payload Pointer to payload data (can be NULL for empty payloads)
//! @param size Size of the payload in bytes
//! @param frame_counter Frame counter value associated with this payload
//!
//! Stores payload data in the global extracted_payloads array for use in tests.
//! Handles both empty and non-empty payloads appropriately.
static void store_payload(const uint8_t *payload, size_t size, uint16_t frame_counter)
{
    if (num_extracted_payloads >= 10) {
        return; // Array full
    }

    if (size > 0) {
        extracted_payloads[num_extracted_payloads].data = malloc(size);
        if (!extracted_payloads[num_extracted_payloads].data) {
            return; // Memory allocation failed
        }
        memcpy(extracted_payloads[num_extracted_payloads].data, payload, size);
    } else {
        // Empty payload
        extracted_payloads[num_extracted_payloads].data = NULL;
    }

    extracted_payloads[num_extracted_payloads].size = size;
    extracted_payloads[num_extracted_payloads].frame_counter = frame_counter;
    num_extracted_payloads++;
}

//! @brief Callback function for stream parsing
//!
//! @param frame Pointer to the parsed frame structure
//!
//! Called by cff_parse_frames() for each successfully parsed frame.
//! Extracts and stores the payload information for later comparison in tests.
//! Handles both empty and non-empty payloads correctly.
void stream_frame_callback(const cff_frame_t *frame)
{
    if (frame) {
        // Handle both empty and non-empty payloads
        if (frame->header.payload_size_bytes > 0 && frame->payload) {
            store_payload(frame->payload, frame->header.payload_size_bytes, frame->header.frame_counter);
        } else {
            // Handle empty payload
            store_payload(NULL, 0, frame->header.frame_counter);
        }
    }
}

//! @brief Test parsing of individual frame files to extract payloads
//!
//! Parses each individual frame file from the test support directory and verifies:
//! - Frame parsing succeeds for all files
//! - Frame structure is valid (preamble, size, CRC)
//! - Payloads are correctly extracted and stored
//! - Frame counters are as expected
void test_parse_individual_frame_files(void)
{
    for (size_t i = 0; i < num_test_frame_files; i++) {
        size_t file_size;
        uint8_t *file_data = read_binary_file(test_frame_files[i], &file_size);

        // Verify we could read the file
        TEST_ASSERT_NOT_NULL_MESSAGE(file_data, "Failed to read frame file");
        TEST_ASSERT_GREATER_THAN(0, file_size);

        // Parse the frame
        cff_frame_t frame;
        size_t consumed_bytes;
        cff_error_en_t result = cff_parse_frame(file_data, file_size, &frame, &consumed_bytes);

        TEST_ASSERT_EQUAL_MESSAGE(cff_error_none, result, "Failed to parse individual frame");
        TEST_ASSERT_EQUAL_MESSAGE(file_size, consumed_bytes, "Frame size mismatch");

        // Verify frame structure
        TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_0, frame.header.preamble[0]);
        TEST_ASSERT_EQUAL_HEX8(CFF_PREAMBLE_BYTE_1, frame.header.preamble[1]);

        // Store payload for later comparison
        if (frame.header.payload_size_bytes > 0) {
            store_payload(frame.payload, frame.header.payload_size_bytes, frame.header.frame_counter);
        } else {
            // Store empty payload
            store_payload(NULL, 0, frame.header.frame_counter);
        }

        free(file_data);
    }

    // We should have extracted payloads from all frame files
    TEST_ASSERT_EQUAL(num_test_frame_files, num_extracted_payloads);
}

//! @brief Test parsing of combined frame stream
//!
//! Reads and parses the combined stream file containing multiple concatenated frames
//! and verifies:
//! - All expected frames are successfully parsed from the stream
//! - Frame counters are sequential (0, 1, 2, ...)
//! - Number of parsed frames matches the expected count
void test_parse_combined_stream(void)
{
    // Read the combined stream file
    stream_buffer = read_binary_file("test/support/stream.bin", &stream_size);

    TEST_ASSERT_NOT_NULL_MESSAGE(stream_buffer, "Failed to read stream file");
    TEST_ASSERT_GREATER_THAN(0, stream_size);

    // Clear previous payloads to test stream parsing
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        if (extracted_payloads[i].data) {
            free(extracted_payloads[i].data);
        }
    }
    memset(extracted_payloads, 0, sizeof(extracted_payloads));
    num_extracted_payloads = 0;

    // Parse the stream
    size_t parsed_frames = cff_parse_frames(stream_buffer, stream_size, stream_frame_callback);

    // We should have parsed all frames (based on the individual files)
    TEST_ASSERT_EQUAL(num_test_frame_files, parsed_frames);
    TEST_ASSERT_EQUAL(num_test_frame_files, num_extracted_payloads);

    // Verify frame counters are sequential
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        TEST_ASSERT_EQUAL(i, extracted_payloads[i].frame_counter);
    }
}

//! @brief Test frame recreation and stream verification
//!
//! Performs a complete round-trip test by:
//! - Parsing the original stream to extract payloads
//! - Rebuilding frames from the extracted payloads with matching frame counters
//! - Comparing the recreated stream byte-for-byte with the original
//!
//! This test validates that the frame building functionality can perfectly recreate the original stream when given the
//! same payloads and frame counters, ensuring data integrity throughout the encode/decode process.
void test_recreate_frames_and_verify_stream(void)
{
    // Parse the original stream to get the correct frame counter sequence
    uint8_t *byte_stream = read_binary_file("test/support/stream.bin", &stream_size);
    TEST_ASSERT_NOT_NULL(byte_stream);

    // Clear payloads and parse stream to get the proper frame sequence
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        if (extracted_payloads[i].data) {
            free(extracted_payloads[i].data);
        }
    }
    memset(extracted_payloads, 0, sizeof(extracted_payloads));
    num_extracted_payloads = 0;

    // Parse the stream to extract payloads in order
    cff_parse_frames(byte_stream, stream_size, stream_frame_callback);

    // Create a buffer to hold recreated stream
    uint8_t recreated_stream[2048];
    size_t total_stream_size = 0;

    // Recreate frames from extracted payloads
    // Build frames one by one, matching the original frame counter sequence
    size_t stream_offset = 0;

    for (size_t i = 0; i < num_extracted_payloads; i++) {
        // Create a new frame builder for each frame to ensure we get the right frame counter
        cff_frame_builder_t builder;
        uint8_t frame_buffer[1024];
        cff_error_en_t init_result = cff_frame_builder_init(&builder, frame_buffer, sizeof(frame_buffer));
        TEST_ASSERT_EQUAL(cff_error_none, init_result);

        // Set the frame counter to match the original
        builder.frame_counter = extracted_payloads[i].frame_counter;

        // Handle empty payloads - use a dummy byte pointer for empty payloads
        const uint8_t *payload_ptr = extracted_payloads[i].data;
        uint8_t dummy_byte = 0;
        if (extracted_payloads[i].size == 0) {
            payload_ptr = &dummy_byte; // Use a valid pointer for empty payload
        }

        cff_error_en_t build_result = cff_build_frame(&builder, payload_ptr, extracted_payloads[i].size);
        TEST_ASSERT_EQUAL_MESSAGE(cff_error_none, build_result, "Failed to rebuild frame");

        // Calculate the size of this frame and copy to the recreated stream
        size_t frame_size = cff_calculate_frame_size_bytes(extracted_payloads[i].size);
        memcpy(recreated_stream + stream_offset, frame_buffer, frame_size);
        stream_offset += frame_size;
        total_stream_size += frame_size;
    }

    // Compare the recreated stream with the original
    TEST_ASSERT_EQUAL_MESSAGE(stream_size, total_stream_size, "Stream size mismatch");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(byte_stream, recreated_stream, stream_size,
                                     "Recreated stream does not match original");

    free(byte_stream);
}

//! @brief Test complete round-trip consistency of the framing protocol
//!
//! Performs a comprehensive integration test that validates the entire workflow:
//! 1. Parse individual frame files to extract payloads
//! 2. Parse the combined stream to extract the same payloads
//! 3. Compare payloads from both sources for consistency
void test_round_trip_consistency(void)
{
    // Clear any existing payloads
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        if (extracted_payloads[i].data) {
            free(extracted_payloads[i].data);
        }
    }
    memset(extracted_payloads, 0, sizeof(extracted_payloads));
    num_extracted_payloads = 0;

    // Step 1: Parse individual frames to extract payloads
    for (size_t i = 0; i < num_test_frame_files; i++) {
        size_t file_size;
        uint8_t *file_data = read_binary_file(test_frame_files[i], &file_size);
        TEST_ASSERT_NOT_NULL(file_data);

        cff_frame_t frame;
        size_t consumed_bytes;
        cff_error_en_t result = cff_parse_frame(file_data, file_size, &frame, &consumed_bytes);
        TEST_ASSERT_EQUAL(cff_error_none, result);

        // Store payload
        if (frame.header.payload_size_bytes > 0) {
            store_payload(frame.payload, frame.header.payload_size_bytes, frame.header.frame_counter);
        } else {
            store_payload(NULL, 0, frame.header.frame_counter);
        }

        free(file_data);
    }

    // Save the individual payloads for comparison
    payload_info_t individual_payloads[10];
    size_t num_individual = num_extracted_payloads;
    for (size_t i = 0; i < num_individual; i++) {
        individual_payloads[i].size = extracted_payloads[i].size;
        individual_payloads[i].frame_counter = extracted_payloads[i].frame_counter;
        if (extracted_payloads[i].size > 0) {
            individual_payloads[i].data = malloc(extracted_payloads[i].size);
            memcpy(individual_payloads[i].data, extracted_payloads[i].data, extracted_payloads[i].size);
        } else {
            individual_payloads[i].data = NULL;
        }
    }

    // Step 2: Parse the combined stream
    uint8_t *stream_data = read_binary_file("test/support/stream.bin", &stream_size);
    TEST_ASSERT_NOT_NULL(stream_data);

    // Clear payloads for stream parsing
    for (size_t i = 0; i < num_extracted_payloads; i++) {
        if (extracted_payloads[i].data) {
            free(extracted_payloads[i].data);
        }
    }
    memset(extracted_payloads, 0, sizeof(extracted_payloads));
    num_extracted_payloads = 0;

    // Parse stream
    size_t parsed_frames = cff_parse_frames(stream_data, stream_size, stream_frame_callback);
    TEST_ASSERT_EQUAL(num_test_frame_files, parsed_frames);

    // Step 3: Compare payloads from individual files vs stream
    TEST_ASSERT_EQUAL_MESSAGE(num_individual, num_extracted_payloads,
                              "Mismatch in number of payloads between individual files and stream");

    for (size_t i = 0; i < num_individual; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(individual_payloads[i].size, extracted_payloads[i].size,
                                  "Payload size mismatch between individual file and stream");

        if (individual_payloads[i].size > 0) {
            TEST_ASSERT_EQUAL_MEMORY_MESSAGE(individual_payloads[i].data, extracted_payloads[i].data,
                                             individual_payloads[i].size,
                                             "Payload content mismatch between individual file and stream");
        }
    }

    // Clean up
    for (size_t i = 0; i < num_individual; i++) {
        if (individual_payloads[i].data) {
            free(individual_payloads[i].data);
        }
    }
    free(stream_data);
}

//! @brief Test stream parsing error recovery with systematic byte corruption
//!
//! Validates the parser's ability to recover from single-byte corruption by:
//! - Systematically corrupting each byte in the stream (one at a time)
//! - Parsing the corrupted stream and counting successful frames
//! - Verifying exactly (num_test_frame_files - 1) frames are parsed
//! - Restoring the corrupted byte and moving to the next position
void test_stream_parsing_error_recovery(void)
{
    // Read the original stream
    size_t stream_size_bytes;
    uint8_t *byte_stream = read_binary_file("test/support/stream.bin", &stream_size_bytes);
    TEST_ASSERT_NOT_NULL(byte_stream);

    // Test corruption at each byte position
    for (size_t corrupt_pos = 0; corrupt_pos < stream_size_bytes; corrupt_pos++) {
        // Corrupt the byte at this position
        byte_stream[corrupt_pos] ^= 0xFF; // Flip all bits

        // Clear extracted payloads for this test
        for (size_t i = 0; i < num_extracted_payloads; i++) {
            if (extracted_payloads[i].data) {
                free(extracted_payloads[i].data);
            }
        }
        memset(extracted_payloads, 0, sizeof(extracted_payloads));
        num_extracted_payloads = 0;

        // Parse the corrupted stream
        size_t parsed_frames = cff_parse_frames(byte_stream, stream_size_bytes, stream_frame_callback);

        // Corrupting any single byte should result in exactly one frame being corrupted,
        // so we should parse exactly (num_test_frame_files - 1) frames
        TEST_ASSERT_EQUAL_MESSAGE(num_test_frame_files - 1, parsed_frames,
                                  "Corrupting a single byte should corrupt exactly one frame");

        // Fix the corrupted byte before moving to the next position
        byte_stream[corrupt_pos] ^= 0xFF; // Flip bits back to original
    }

    free(byte_stream);
}
