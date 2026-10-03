#pragma once

#include "memory.h"

#include <cstddef>
#include <type_traits>

/**
 * Public C++ API of the memory module.
 *
 * The record types (data_array, cache, heap, memory_pool, the bit and byte streams, struct_definition and
 * the packet group) keep the exact C layouts from types/memory.h and carry their operations as member
 * functions; the free functions below have no natural owning record. Every original C symbol still exists
 * as a thin extern "C" shim in memory_c_api.cpp.
 */

extern "C" {
extern uint8_t bit_mask_keep[9];
extern uint8_t bit_mask_clear[8];
extern crc32_table crc32_lookup_table;
extern uint8_t crc32_lookup_table_initialized;
extern char *data_packet_group_error;
extern byte_swap_definition packet_header_byte_swap_definition;
}

namespace halo::memory {

void crc32_update(uint32_t *crc, uint8_t *data, int32_t length);
void datum_index_invalidate(datum_index *out_index);
void byte_swap_array(int32_t size_code, uint32_t *array, int32_t count);

} // namespace halo::memory

static_assert(sizeof(growable_array) == 0x0c && std::is_standard_layout_v<growable_array>);
static_assert(sizeof(bit_stream) == 0x18 && offsetof(bit_stream, last_bit) == 0x14);
static_assert(sizeof(byte_stream) == 0x10 && offsetof(byte_stream, overflow) == 0x0c);
static_assert(sizeof(circular_buffer) == 0x18 && offsetof(circular_buffer, data) == 0x14);
static_assert(sizeof(crc32_table) == 0x400);
static_assert(sizeof(data_array) == 0x38 && offsetof(data_array, signature) == 0x28 &&
              offsetof(data_array, data) == 0x34 && std::is_standard_layout_v<data_array>);
static_assert(sizeof(data_iterator) == 0x10 && offsetof(data_iterator, signature) == 0x0c);
static_assert(sizeof(struct_definition_field) == 0x0a);
static_assert(sizeof(struct_definition) == 0x14 && offsetof(struct_definition, size_computed) == 0x10);
static_assert(sizeof(byte_swap_definition) == 0x14);
static_assert(sizeof(data_packet_type) == 0x08);
static_assert(sizeof(data_packet_group) == 0x30 && offsetof(data_packet_group, types) == 0x10);
static_assert(sizeof(cache_entry) == 0x1c);
static_assert(sizeof(cache) == 0x7c && offsetof(cache, entry_data) == 0x44 && std::is_standard_layout_v<cache>);
static_assert(sizeof(memory_pool_block) == 0x18);
static_assert(sizeof(memory_pool) == 0x38 && offsetof(memory_pool, last_block) == 0x34);
static_assert(sizeof(heap_block) == 0x10);
static_assert(sizeof(heap) == 0x38 && offsetof(heap, first_block) == 0x2c && std::is_standard_layout_v<heap>);
