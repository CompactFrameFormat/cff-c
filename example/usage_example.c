#include "cff.h"
#include <stdio.h>
#include <string.h>

// Callback function to handle parsed frames
void frame_handler(const cff_frame_t *frame)
{
    printf("Received frame %d with %zu byte payload: ", frame->header.frame_counter, frame->payload_size_bytes);

    // Copy payload from ring buffer to linear buffer for printing
    if (frame->payload_size_bytes > 0) {
        uint8_t payload_buffer[256]; // Assume payload won't exceed this size for the example
        cff_error_en_t copy_result = cff_copy_frame_payload(frame, payload_buffer, sizeof(payload_buffer));

        if (copy_result == cff_error_none) {
            // Print payload as string (assuming it's text)
            for (size_t i = 0; i < frame->payload_size_bytes; i++) {
                printf("%c", payload_buffer[i]);
            }
        }
        else {
            printf("[Error copying payload]");
        }
    }
    printf("\n");
}

int main()
{
    // Initialize frame builder with a buffer
    uint8_t build_buffer[256];
    cff_frame_builder_t builder;
    cff_error_en_t result = cff_frame_builder_init(&builder, build_buffer, sizeof(build_buffer));
    if (result != cff_error_none) {
        printf("Failed to initialize frame builder\n");
        return -1;
    }

    // Create a larger buffer to hold multiple frames
    uint8_t frame_stream[512];
    size_t stream_pos = 0;

    // Build multiple frames
    const char *messages[] = {"Hello, World!", "CFF Frame 2", "Final message"};

    for (int i = 0; i < 3; i++) {
        // Build frame with current message
        result = cff_build_frame(&builder, (const uint8_t *) messages[i], strlen(messages[i]));
        if (result != cff_error_none) {
            printf("Failed to build frame %d\n", i);
            return -1;
        }

        // Calculate frame size and copy to stream buffer
        size_t frame_size = cff_calculate_frame_size_bytes(strlen(messages[i]));
        if (stream_pos + frame_size > sizeof(frame_stream)) {
            printf("Stream buffer too small\n");
            return -1;
        }

        memcpy(frame_stream + stream_pos, build_buffer, frame_size);
        stream_pos += frame_size;

        printf("Built frame %d (%zu bytes): \"%s\"\n", i + 1, frame_size, messages[i]);
    }

    printf("\nTotal stream size: %zu bytes\n\n", stream_pos);

    // Parse all frames from the stream using ring buffer
    printf("Parsing frames:\n");

    // Set up ring buffer with the frame stream data
    uint8_t ring_storage[1024]; // Large enough for our example stream
    cff_ring_buffer_t ring_buffer;
    cff_error_en_t ring_result = cff_ring_buffer_init(&ring_buffer, ring_storage, sizeof(ring_storage));
    if (ring_result != cff_error_none) {
        printf("Failed to initialize ring buffer\n");
        return -1;
    }

    // Append stream data to ring buffer
    ring_result = cff_ring_buffer_append(&ring_buffer, frame_stream, (uint32_t) stream_pos);
    if (ring_result != cff_error_none) {
        printf("Failed to append data to ring buffer\n");
        return -1;
    }

    // Parse frames from ring buffer
    size_t parsed_frames = cff_parse_frames(&ring_buffer, frame_handler);

    printf("\nParsed %zu frames from stream\n", parsed_frames);

    return 0;
}