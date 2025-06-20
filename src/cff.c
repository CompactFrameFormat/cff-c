// MIT License

// Copyright (c) 2025 Richard Keelan

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "cff.h"
#include <stdbool.h>
#include <string.h>

// Binary Primitives ---------------------------------------------------------------------------------------------------

static uint16_t cff_get_uint16_le(const uint8_t *data)
{
    return (uint16_t) (data[0] | (data[1] << 8));
}

static void cff_set_uint16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t) (value & 0xFF);
    data[1] = (uint8_t) ((value >> 8) & 0xFF);
}

// Ring Buffer Implementation ------------------------------------------------------------------------------------------

cff_error_en_t cff_ring_buffer_init(cff_ring_buffer_t *ring_buffer, CFF_RB_T *buffer, uint32_t buffer_size)
{
    if (ring_buffer == NULL || buffer == NULL) {
        return cff_error_null_pointer;
    }

    if (buffer_size == 0) {
        return cff_error_buffer_too_small;
    }

    ring_buffer->buffer = buffer;
    ring_buffer->buffer_size = buffer_size;
    ring_buffer->append_index = 0;
    ring_buffer->consume_index = 0;
    ring_buffer->free_space = buffer_size;

    // Initialize buffer to zero
    memset(buffer, 0, buffer_size * sizeof(CFF_RB_T));

    return cff_error_none;
}

cff_error_en_t cff_ring_buffer_append(cff_ring_buffer_t *ring_buffer, const CFF_RB_T *items, uint32_t number_of_items)
{
    if (ring_buffer == NULL || items == NULL) {
        return cff_error_null_pointer;
    }

    if (number_of_items > ring_buffer->free_space) {
        return cff_error_insufficient_space;
    }

    if (ring_buffer->append_index + number_of_items > ring_buffer->buffer_size) {
        // Wrap-around
        uint32_t amount_to_copy = CFF_MIN(number_of_items, ring_buffer->buffer_size - ring_buffer->append_index);
        memcpy(ring_buffer->buffer + ring_buffer->append_index, items, amount_to_copy * sizeof(CFF_RB_T));
        ring_buffer->append_index = (ring_buffer->append_index + amount_to_copy) % ring_buffer->buffer_size;
        ring_buffer->free_space -= amount_to_copy;
        number_of_items -= amount_to_copy;
        items += amount_to_copy;
    }

    // Now append_index is before consume_index, so we can copy up to it
    memcpy(ring_buffer->buffer + ring_buffer->append_index, items, number_of_items * sizeof(CFF_RB_T));
    ring_buffer->append_index = (ring_buffer->append_index + number_of_items) % ring_buffer->buffer_size;
    ring_buffer->free_space -= number_of_items;

    return cff_error_none;
}

cff_error_en_t cff_ring_buffer_consume(cff_ring_buffer_t *ring_buffer, CFF_RB_T *items, uint32_t number_of_items)
{
    if (ring_buffer == NULL || items == NULL) {
        return cff_error_null_pointer;
    }

    if (number_of_items > (ring_buffer->buffer_size - ring_buffer->free_space)) {
        return cff_error_insufficient_space;
    }

    if (ring_buffer->consume_index + number_of_items > ring_buffer->buffer_size) {
        // Wrap-around
        uint32_t amount_to_consume = CFF_MIN(number_of_items, ring_buffer->buffer_size - ring_buffer->consume_index);
        memcpy(items, ring_buffer->buffer + ring_buffer->consume_index, amount_to_consume * sizeof(CFF_RB_T));
        ring_buffer->consume_index = (ring_buffer->consume_index + amount_to_consume) % ring_buffer->buffer_size;
        ring_buffer->free_space += amount_to_consume;
        number_of_items -= amount_to_consume;
        items += amount_to_consume;
    }

    // Now consume_index is before append_index, so we can consume up to it
    memcpy(items, ring_buffer->buffer + ring_buffer->consume_index, number_of_items * sizeof(CFF_RB_T));
    ring_buffer->consume_index = (ring_buffer->consume_index + number_of_items) % ring_buffer->buffer_size;
    ring_buffer->free_space += number_of_items;

    return cff_error_none;
}

// CRC Calulation ------------------------------------------------------------------------------------------------------

static uint16_t cff_crc_table[256] = {0};
static bool cff_crc_table_initialized = 0;

static void cff_init_crc_table(void)
{
    if (cff_crc_table_initialized) {
        return;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t crc = (uint16_t) (i << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CFF_CRC_POLYNOMIAL;
            }
            else {
                crc = crc << 1;
            }
        }
        cff_crc_table[i] = crc;
    }
    cff_crc_table_initialized = 1;
}

cff_error_en_t cff_crc16(const uint8_t *data, size_t data_size_bytes, uint16_t *crc)
{
    if (data == NULL) {
        return cff_error_null_pointer;
    }

    cff_init_crc_table();

    *crc = CFF_CRC_INIT;
    for (size_t i = 0; i < data_size_bytes; i++) {
        uint8_t tbl_idx = (uint8_t) ((*crc >> 8) ^ data[i]);
        *crc = (*crc << 8) ^ cff_crc_table[tbl_idx];
    }
    return cff_error_none;
}

// Frame

cff_error_en_t cff_frame_builder_init(cff_frame_builder_t *builder, uint8_t *buffer, size_t buffer_size_bytes)
{
    if (builder == NULL || buffer == NULL) {
        return cff_error_null_pointer;
    }

    if (buffer_size_bytes < CFF_MIN_FRAME_SIZE_BYTES) {
        return cff_error_buffer_too_small;
    }

    builder->buffer = buffer;
    builder->buffer_size_bytes = buffer_size_bytes;
    builder->frame_counter = 0;

    return cff_error_none;
}

cff_error_en_t cff_build_frame(cff_frame_builder_t *builder, const uint8_t *payload, size_t payload_size_bytes)
{
    if (builder == NULL || builder->buffer == NULL) {
        return cff_error_null_pointer;
    }

    if (payload_size_bytes > CFF_MAX_PAYLOAD_SIZE_BYTES) {
        return cff_error_payload_too_large;
    }

    if (payload == NULL) {
        return cff_error_null_pointer;
    }

    size_t required_size_bytes = cff_calculate_frame_size_bytes(payload_size_bytes);
    if (required_size_bytes > builder->buffer_size_bytes) {
        return cff_error_buffer_too_small;
    }

    uint8_t *ptr = builder->buffer;

    // Write preamble
    ptr[0] = CFF_PREAMBLE_BYTE_0;
    ptr[1] = CFF_PREAMBLE_BYTE_1;

    // Write frame counter and payload size, then increment the frame counter
    cff_set_uint16_le(&ptr[2], builder->frame_counter++);
    cff_set_uint16_le(&ptr[4], (uint16_t) payload_size_bytes);

    // Calculate and write header CRC
    uint16_t header_crc;
    cff_error_en_t error = cff_crc16(ptr, 6, &header_crc);
    if (error != cff_error_none) {
        return error;
    }
    cff_set_uint16_le(&ptr[6], header_crc);

    // Write payload
    memcpy(&ptr[CFF_HEADER_SIZE_BYTES], payload, payload_size_bytes);

    // Calculate and write payload CRC
    uint16_t payload_crc;
    error = cff_crc16(&ptr[CFF_HEADER_SIZE_BYTES], payload_size_bytes, &payload_crc);
    if (error != cff_error_none) {
        return error;
    }
    cff_set_uint16_le(&ptr[CFF_HEADER_SIZE_BYTES + payload_size_bytes], payload_crc);

    return cff_error_none;
}

cff_error_en_t cff_parse_frame(const uint8_t *buffer, size_t buffer_size_bytes, cff_frame_t *frame,
                               size_t *consumed_bytes)
{
    if (buffer == NULL || frame == NULL) {
        return cff_error_null_pointer;
    }

    if (buffer_size_bytes < CFF_MIN_FRAME_SIZE_BYTES) {
        return cff_error_incomplete_frame;
    }

    // Parse header
    const uint8_t *ptr = buffer;
    frame->header.preamble[0] = ptr[0];
    frame->header.preamble[1] = ptr[1];

    if (frame->header.preamble[0] != CFF_PREAMBLE_BYTE_0 || frame->header.preamble[1] != CFF_PREAMBLE_BYTE_1) {
        return cff_error_invalid_preamble;
    }

    frame->header.frame_counter = cff_get_uint16_le(&ptr[2]);
    frame->header.payload_size_bytes = cff_get_uint16_le(&ptr[4]);
    frame->header.header_crc = cff_get_uint16_le(&ptr[6]);

    // Validate header CRC
    uint8_t header_data[6];
    header_data[0] = frame->header.preamble[0];
    header_data[1] = frame->header.preamble[1];
    cff_set_uint16_le(&header_data[2], frame->header.frame_counter);
    cff_set_uint16_le(&header_data[4], frame->header.payload_size_bytes);

    uint16_t expected_crc;
    cff_error_en_t error = cff_crc16(header_data, 6, &expected_crc);
    if (error != cff_error_none) {
        return error;
    }
    if (expected_crc != frame->header.header_crc) {
        return cff_error_invalid_header_crc;
    }

    // Check if we have enough data for the complete frame
    size_t expected_frame_size_bytes = cff_calculate_frame_size_bytes(frame->header.payload_size_bytes);
    if (buffer_size_bytes < expected_frame_size_bytes) {
        return cff_error_incomplete_frame;
    }

    // Extract payload and payload CRC
    frame->payload = &buffer[CFF_HEADER_SIZE_BYTES];
    frame->payload_crc = cff_get_uint16_le(&buffer[CFF_HEADER_SIZE_BYTES + frame->header.payload_size_bytes]);
    frame->payload_size_bytes_bytes = frame->header.payload_size_bytes;

    // Validate payload CRC
    uint16_t expected_payload_crc;
    error = cff_crc16(frame->payload, frame->header.payload_size_bytes, &expected_payload_crc);
    if (error != cff_error_none) {
        return error;
    }
    if (expected_payload_crc != frame->payload_crc) {
        return cff_error_invalid_payload_crc;
    }

    // Set consumed bytes to the actual frame size
    if (consumed_bytes != NULL) {
        *consumed_bytes = expected_frame_size_bytes;
    }

    return cff_error_none;
}

static size_t cff_find_preamble(const uint8_t *buffer, size_t buffer_size_bytes, size_t start_position)
{
    if (start_position + CFF_PREAMBLE_SIZE_BYTES > buffer_size_bytes) {
        return buffer_size_bytes;
    }

    for (size_t i = start_position; i <= buffer_size_bytes - CFF_PREAMBLE_SIZE_BYTES; i++) {
        if (buffer[i] == CFF_PREAMBLE_BYTE_0 && buffer[i + 1] == CFF_PREAMBLE_BYTE_1) {
            return i;
        }
    }
    return buffer_size_bytes; // Not found
}

size_t cff_parse_frames(const uint8_t *buffer, size_t buffer_size_bytes, cff_callback_t callback)
{
    if (buffer == NULL || callback == NULL) {
        return 0;
    }

    size_t consumed_bytes = 0;
    size_t frames_parsed = 0;

    while (consumed_bytes < buffer_size_bytes) {
        // Find the next preamble starting from current position
        consumed_bytes = cff_find_preamble(buffer, buffer_size_bytes, consumed_bytes);
        if (consumed_bytes >= buffer_size_bytes) {
            // No more preambles found
            break;
        }

        // Check if we have enough bytes for a minimum frame at this position
        if (buffer_size_bytes - consumed_bytes < CFF_MIN_FRAME_SIZE_BYTES) {
            break;
        }

        cff_frame_t frame;
        size_t frame_size_bytes = 0;

        cff_error_en_t error =
            cff_parse_frame(&buffer[consumed_bytes], buffer_size_bytes - consumed_bytes, &frame, &frame_size_bytes);

        if (error == cff_error_none) {
            // Successfully parsed a frame, call the callback
            callback(&frame);
            frames_parsed++;
            consumed_bytes += frame_size_bytes;
        }
        else if (error == cff_error_incomplete_frame) {
            // Not enough data for a complete frame, stop parsing
            break;
        }
        else {
            // Invalid frame found, advance by 1 to continue searching
            consumed_bytes++;
        }
    }
    return frames_parsed;
}
