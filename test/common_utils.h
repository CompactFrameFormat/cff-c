#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "cff.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @brief Helper function to setup ring buffer from linear buffer data
//!
//! Initializes a ring buffer and appends the provided linear data to it.
//! This is commonly used in tests to convert test data into ring buffer format.
//!
//! @param ring_buffer Pointer to ring buffer structure to initialize
//! @param ring_storage Pointer to storage buffer for the ring buffer
//! @param storage_size Size of the storage buffer
//! @param data Pointer to linear data to append to ring buffer
//! @param data_size Size of the data to append
void setup_ring_buffer_from_data(cff_ring_buffer_t *ring_buffer, uint8_t *ring_storage, uint32_t storage_size,
                                 const uint8_t *data, size_t data_size);

#ifdef __cplusplus
}
#endif

#endif // COMMON_UTILS_H