#pragma once

#include "memory.h"

#include <cstddef>
#include <type_traits>

namespace halo::memory {

/**
 * Operations on the growable_array record. The view adds no data members, so a growable_array pointer can be
 * viewed as a growable_array_view in place and the C layout is unchanged.
 */
struct growable_array_view : ::growable_array {
    uint32_t add_element();
    void remove_element(uint32_t index);
};

inline growable_array_view *view(::growable_array *record) { return static_cast<growable_array_view *>(record); }

/**
 * Operations on the circular_buffer record. The view adds no data members, so a circular_buffer pointer can be
 * viewed as a circular_buffer_view in place and the C layout is unchanged.
 */
struct circular_buffer_view : ::circular_buffer {
    static circular_buffer *create(char *name, int32_t requested_size);
    uint32_t read(uint8_t *destination, uint32_t byte_count, char consume);
    uint32_t write(uint32_t byte_count, uint8_t *source);
};

inline circular_buffer_view *view(::circular_buffer *record) { return static_cast<circular_buffer_view *>(record); }

/**
 * Operations on the bit_stream record. The view adds no data members, so a bit_stream pointer can be
 * viewed as a bit_stream_view in place and the C layout is unchanged.
 */
struct bit_stream_view : ::bit_stream {
    uint32_t read_bit(uint8_t *out_bit);
    uint32_t read_bits(uint32_t bit_count, uint32_t *out_value);
    int32_t read_bits_chunked(int32_t total_bit_count, uint32_t *buffer);
    uint8_t write_bit(int32_t bit_value);
    uint8_t write_bits(uint32_t bit_count, uint32_t value);
    int32_t write_bits_chunked(const uint32_t *values, int32_t total_bit_count);
};

inline bit_stream_view *view(::bit_stream *record) { return static_cast<bit_stream_view *>(record); }

/**
 * Operations on the byte_stream record. The view adds no data members, so a byte_stream pointer can be
 * viewed as a byte_stream_view in place and the C layout is unchanged.
 */
struct byte_stream_view : ::byte_stream {
    uint32_t read_long();
    uint32_t read_ranged_integer(int32_t maximum);
    char *read_string();
    uint32_t write_ranged_integer(int32_t maximum, uint32_t value);
    uint32_t write_string(char *string, int16_t max_length);
};

inline byte_stream_view *view(::byte_stream *record) { return static_cast<byte_stream_view *>(record); }

/**
 * Operations on the crc32_table record. The view adds no data members, so a crc32_table pointer can be
 * viewed as a crc32_table_view in place and the C layout is unchanged.
 */
struct crc32_table_view : ::crc32_table {
    void build();
};

inline crc32_table_view *view(::crc32_table *record) { return static_cast<crc32_table_view *>(record); }

/**
 * Operations on the data_array record. The view adds no data members, so a data_array pointer can be
 * viewed as a data_array_view in place and the C layout is unchanged.
 */
struct data_array_view : ::data_array {
    void delete_all();
    static data_array *create(int16_t element_size, char *name, int16_t maximum_count);
    void delete_datum(datum_index handle);
    void initialize_element(void *element);
    void *get(datum_index handle);
    datum_index new_datum();
    datum_index new_at_index(int16_t index);
    datum_index new_at_index_with_salt(datum_index requested_handle);
    datum_index next_datum(int16_t after_index);
};

inline data_array_view *view(::data_array *record) { return static_cast<data_array_view *>(record); }

/**
 * Operations on the data_iterator record. The view adds no data members, so a data_iterator pointer can be
 * viewed as a data_iterator_view in place and the C layout is unchanged.
 */
struct data_iterator_view : ::data_iterator {
    void *next();
};

inline data_iterator_view *view(::data_iterator *record) { return static_cast<data_iterator_view *>(record); }

/**
 * Operations on the memory_pool record. The view adds no data members, so a memory_pool pointer can be
 * viewed as a memory_pool_view in place and the C layout is unchanged.
 */
struct memory_pool_view : ::memory_pool {
    int32_t allocate(int32_t requested_size, void **owner);
    void compact();
    int32_t reallocate(void **owner_cell, int32_t new_size);
    void unlink(void **payload_ptr);
};

inline memory_pool_view *view(::memory_pool *record) { return static_cast<memory_pool_view *>(record); }

/**
 * Operations on the heap record. The view adds no data members, so a heap pointer can be
 * viewed as a heap_view in place and the C layout is unchanged.
 */
struct heap_view : ::heap {
    void advance_free_slot();
    void *allocate(uint32_t size);
    uint32_t allocate_raw(uint32_t size);
    void compact();
    uint32_t find_first_free_slot();
    int32_t find_free_block(uint32_t size_needed, void **out_predecessor);
    int32_t get_free_bytes();
    void *reallocate(void *old_payload, uint32_t new_size);
    void *resize_block(uint32_t new_size, heap_block *old_block);
    void unlink_block(heap_block *block);
};

inline heap_view *view(::heap *record) { return static_cast<heap_view *>(record); }

/**
 * Operations on the cache record. The view adds no data members, so a cache pointer can be
 * viewed as a cache_view in place and the C layout is unchanged.
 */
struct cache_view : ::cache {
    void initialize(char *name, int32_t block_count, int32_t block_shift, int16_t maximum_count, void *release_procedure, void *in_use_procedure);
    datum_index allocate_block(uint32_t requested_bytes);
    void build_status_bitmap(uint8_t *bitmap);
    void evict_entry(datum_index handle);
    void flush();
    cache_entry *entry_at(datum_index handle);
};

inline cache_view *view(::cache *record) { return static_cast<cache_view *>(record); }

/**
 * Operations on the byte_swap_definition record. The view adds no data members, so a byte_swap_definition pointer can be
 * viewed as a byte_swap_definition_view in place and the C layout is unchanged.
 */
struct byte_swap_definition_view : ::byte_swap_definition {
    void swap(int32_t data, int32_t *codes, int32_t *out_size, int32_t *out_record_count);
};

inline byte_swap_definition_view *view(::byte_swap_definition *record) { return static_cast<byte_swap_definition_view *>(record); }

/**
 * Operations on the struct_definition record. The view adds no data members, so a struct_definition pointer can be
 * viewed as a struct_definition_view in place and the C layout is unchanged.
 */
struct struct_definition_view : ::struct_definition {
    void compute_size(int16_t *out_size, struct_definition_field *fields, int16_t *out_field_count);
    void decode(byte_stream *input, int16_t version, void *dest_instance, int16_t *out_dest_size, struct_definition_field *fields, int16_t *out_field_count);
    void encode(byte_stream *output, int16_t version, void *source, int16_t *out_source_size, struct_definition_field *fields, int16_t *out_field_count);
    uint8_t decode_packet_body(uint8_t *buffer, int16_t remaining_length, void *dest, uint16_t *out_version_used, int16_t *out_bytes_consumed);
    int32_t encode_packet_body(int16_t version, uint8_t *version_byte_dest, byte_stream *output, void *source, int16_t *out_wrote_version_byte, int16_t capacity_check);
};

inline struct_definition_view *view(::struct_definition *record) { return static_cast<struct_definition_view *>(record); }

/**
 * Operations on the data_packet_group record. The view adds no data members, so a data_packet_group pointer can be
 * viewed as a data_packet_group_view in place and the C layout is unchanged.
 */
struct data_packet_group_view : ::data_packet_group {
    void compute_sizes();
    int32_t append_packet_header(uint8_t *buffer, int16_t *cursor, uint8_t header_byte);
    int32_t decode_packet(int16_t *remaining_length, void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    int32_t encode_packet(int16_t version, struct_definition *definition, uint8_t *version_byte_dest, byte_stream *output, uint8_t *buffer, int16_t *cursor, void *source, int16_t *out_wrote_version_byte, uint8_t packet_type);
};

inline data_packet_group_view *view(::data_packet_group *record) { return static_cast<data_packet_group_view *>(record); }

void crc32_update(uint32_t *crc, const void *data, int32_t length);
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

