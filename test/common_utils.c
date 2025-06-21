#include "common_utils.h"

void setup_ring_buffer_from_data(cff_ring_buffer_t *ring_buffer, uint8_t *ring_storage, uint32_t storage_size,
                                 const uint8_t *data, size_t data_size)
{
    cff_ring_buffer_init(ring_buffer, ring_storage, storage_size);
    cff_ring_buffer_append(ring_buffer, data, (uint32_t) data_size);
}