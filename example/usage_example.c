#include "cff.h"
#include <stdio.h>
#include <string.h>

// Callback function to handle parsed frames
void frame_handler(const cff_frame_t *frame)
{
    printf("Received frame %d with %zu byte payload: ", frame->header.frame_counter, frame->payload_size_bytes_bytes);

    // Print payload as string (assuming it's text)
    for (size_t i = 0; i < frame->payload_size_bytes_bytes; i++) {
        printf("%c", frame->payload[i]);
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

    // Parse all frames from the stream
    printf("Parsing frames:\n");
    size_t consumed = cff_parse_frames(frame_stream, stream_pos, frame_handler);

    printf("\nParsed %zu bytes from stream\n", consumed);

    return 0;
}