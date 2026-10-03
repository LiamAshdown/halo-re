/**
 * @file include/halo/memory/api.hpp
 * The public C++ API of the memory module (namespace halo::memory): the record operations of memory.hpp as free
 * functions taking the engine's records by pointer, in the argument order of the original functions, plus the
 * Globals service object.
 */
#pragma once

#include "halo/memory/globals.hpp"
#include "halo/memory/memory.hpp"

namespace halo::memory {

inline uint32_t growable_array_add_element(growable_array *array)
{
    return halo::memory::view(array)->add_element();
}

inline void growable_array_remove_element(growable_array *array, uint32_t index)
{
    halo::memory::view(array)->remove_element(index);
}

inline circular_buffer *circular_buffer_new(char *name, int32_t requested_size)
{
    return halo::memory::circular_buffer_view::create(name, requested_size);
}

inline uint32_t circular_buffer_read(uint8_t *destination, uint32_t byte_count, char consume, circular_buffer *stream)
{
    return halo::memory::view(stream)->read(destination, byte_count, consume);
}

inline uint32_t circular_buffer_write(uint32_t byte_count, circular_buffer *stream, uint8_t *source)
{
    return halo::memory::view(stream)->write(byte_count, source);
}

inline uint32_t bit_stream_read_bit(uint8_t *out_bit, bit_stream *stream)
{
    return halo::memory::view(stream)->read_bit(out_bit);
}

inline uint32_t bit_stream_read_bits(uint32_t bit_count, uint32_t *out_value, bit_stream *stream)
{
    return halo::memory::view(stream)->read_bits(bit_count, out_value);
}

inline int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer, bit_stream *stream)
{
    return halo::memory::view(stream)->read_bits_chunked(total_bit_count, buffer);
}

inline uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream)
{
    return halo::memory::view(stream)->write_bit(bit_value);
}

inline uint8_t bit_stream_write_bits(uint32_t bit_count, uint32_t value, bit_stream *stream)
{
    return halo::memory::view(stream)->write_bits(bit_count, value);
}

inline int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count)
{
    return halo::memory::view(stream)->write_bits_chunked(values, total_bit_count);
}

inline uint32_t byte_stream_read_long(byte_stream *stream)
{
    return halo::memory::view(stream)->read_long();
}

inline uint32_t byte_stream_read_ranged_integer(int32_t maximum, byte_stream *stream)
{
    return halo::memory::view(stream)->read_ranged_integer(maximum);
}

inline char * byte_stream_read_string(byte_stream *stream)
{
    return halo::memory::view(stream)->read_string();
}

inline uint32_t byte_stream_write_ranged_integer(int32_t maximum, uint32_t value, byte_stream *stream)
{
    return halo::memory::view(stream)->write_ranged_integer(maximum, value);
}

inline uint32_t byte_stream_write_string(char *string, int16_t max_length, byte_stream *stream)
{
    return halo::memory::view(stream)->write_string(string, max_length);
}

inline void crc32_build_table(crc32_table *table)
{
    halo::memory::view(table)->build();
}

inline void data_delete_all(data_array *array)
{
    halo::memory::view(array)->delete_all();
}

inline data_array * data_new(int16_t element_size, char *name, int16_t maximum_count)
{
    return halo::memory::data_array_view::create(element_size, name, maximum_count);
}

inline void datum_delete(data_array *array, datum_index handle)
{
    halo::memory::view(array)->delete_datum(handle);
}

inline void datum_element_initialize(data_array *array, void *element)
{
    halo::memory::view(array)->initialize_element(element);
}

inline void * datum_get(datum_index handle, data_array *array)
{
    return halo::memory::view(array)->get(handle);
}

inline datum_index datum_new(data_array *array)
{
    return halo::memory::view(array)->new_datum();
}

inline datum_index datum_new_at_index(int16_t index, data_array *array)
{
    return halo::memory::view(array)->new_at_index(index);
}

inline datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array)
{
    return halo::memory::view(array)->new_at_index_with_salt(requested_handle);
}

inline datum_index datum_next(int16_t after_index, data_array *array)
{
    return halo::memory::view(array)->next_datum(after_index);
}

inline void * data_iterator_next(data_iterator *iterator)
{
    return halo::memory::view(iterator)->next();
}

inline int32_t block_list_allocate(memory_pool *arena, int32_t requested_size, void **owner)
{
    return halo::memory::view(arena)->allocate(requested_size, owner);
}

inline void block_list_compact(memory_pool *arena)
{
    halo::memory::view(arena)->compact();
}

inline int32_t block_list_reallocate(void **owner_cell, int32_t new_size, memory_pool *arena)
{
    return halo::memory::view(arena)->reallocate(owner_cell, new_size);
}

inline void block_list_unlink(void **payload_ptr, memory_pool *arena)
{
    halo::memory::view(arena)->unlink(payload_ptr);
}

inline void heap_advance_free_slot(heap *self)
{
    halo::memory::view(self)->advance_free_slot();
}

inline void * heap_allocate(uint32_t size, heap *self)
{
    return halo::memory::view(self)->allocate(size);
}

inline uint32_t heap_allocate_raw(uint32_t size, heap *self)
{
    return halo::memory::view(self)->allocate_raw(size);
}

inline void heap_compact(heap *self)
{
    halo::memory::view(self)->compact();
}

inline uint32_t heap_find_first_free_slot(heap *self)
{
    return halo::memory::view(self)->find_first_free_slot();
}

inline int32_t heap_find_free_block(heap *self, uint32_t size_needed, void **out_predecessor)
{
    return halo::memory::view(self)->find_free_block(size_needed, out_predecessor);
}

inline int32_t heap_get_free_bytes(heap *self)
{
    return halo::memory::view(self)->get_free_bytes();
}

inline void * heap_reallocate(void *old_payload, uint32_t new_size, heap *self)
{
    return halo::memory::view(self)->reallocate(old_payload, new_size);
}

inline void * heap_resize_block(uint32_t new_size, heap_block *old_block, heap *self)
{
    return halo::memory::view(self)->resize_block(new_size, old_block);
}

inline void heap_unlink_block(heap_block *block, heap *self)
{
    halo::memory::view(self)->unlink_block(block);
}

inline void cache_new(char *name, cache *self, int32_t block_count, int32_t block_shift, int16_t maximum_count, void *release_procedure, void *in_use_procedure)
{
    halo::memory::view(self)->initialize(name, block_count, block_shift, maximum_count, release_procedure, in_use_procedure);
}

inline datum_index cache_allocate_block(cache *self, uint32_t requested_bytes)
{
    return halo::memory::view(self)->allocate_block(requested_bytes);
}

inline void cache_build_status_bitmap(cache *self, uint8_t *bitmap)
{
    halo::memory::view(self)->build_status_bitmap(bitmap);
}

inline void cache_evict_entry(datum_index handle, cache *self)
{
    halo::memory::view(self)->evict_entry(handle);
}

inline void cache_flush(cache *self)
{
    halo::memory::view(self)->flush();
}

inline void struct_definition_byte_swap(byte_swap_definition *definition, int32_t data, int32_t *codes, int32_t *out_size, int32_t *out_record_count)
{
    halo::memory::view(definition)->swap(data, codes, out_size, out_record_count);
}

inline void struct_definition_compute_size(struct_definition *definition, int16_t *out_size, struct_definition_field *fields, int16_t *out_field_count)
{
    halo::memory::view(definition)->compute_size(out_size, fields, out_field_count);
}

inline void struct_definition_decode(struct_definition *definition, byte_stream *input, int16_t version, void *dest_instance, int16_t *out_dest_size, struct_definition_field *fields, int16_t *out_field_count)
{
    halo::memory::view(definition)->decode(input, version, dest_instance, out_dest_size, fields, out_field_count);
}

inline void struct_definition_encode(struct_definition *definition, byte_stream *output, int16_t version, void *source, int16_t *out_source_size, struct_definition_field *fields, int16_t *out_field_count)
{
    halo::memory::view(definition)->encode(output, version, source, out_source_size, fields, out_field_count);
}

inline void struct_definition_table_compute_sizes(data_packet_group *group)
{
    halo::memory::view(group)->compute_sizes();
}

inline int32_t data_packet_group_append_packet_header(uint8_t *buffer, data_packet_group *group, int16_t *cursor, uint8_t header_byte)
{
    return halo::memory::view(group)->append_packet_header(buffer, cursor, header_byte);
}

inline int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class)
{
    return halo::memory::view(group)->decode_packet(remaining_length, decoded_body, buffer, out_type, out_version_used, expected_class);
}

inline int32_t data_packet_group_encode_packet(int16_t version, struct_definition *definition, uint8_t *version_byte_dest, byte_stream *output, uint8_t *buffer, int16_t *cursor, data_packet_group *group, void *source, int16_t *out_wrote_version_byte, uint8_t packet_type)
{
    return halo::memory::view(group)->encode_packet(version, definition, version_byte_dest, output, buffer, cursor, source, out_wrote_version_byte, packet_type);
}

inline uint8_t data_packet_group_decode_packet_body(uint8_t *buffer, struct_definition *definition, int16_t remaining_length, void *dest, uint16_t *out_version_used, int16_t *out_bytes_consumed)
{
    return halo::memory::view(definition)->decode_packet_body(buffer, remaining_length, dest, out_version_used, out_bytes_consumed);
}

inline int32_t data_packet_group_encode_packet_body(int16_t version, struct_definition *definition, uint8_t *version_byte_dest, byte_stream *output, void *source, int16_t *out_wrote_version_byte, int16_t capacity_check)
{
    return halo::memory::view(definition)->encode_packet_body(version, version_byte_dest, output, source, out_wrote_version_byte, capacity_check);
}

}  // namespace halo::memory
