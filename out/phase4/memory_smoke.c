#include "tags.h"
#include "memory.h"

// Compile-time layout checks for types/memory.h. Records that hold pointers are only checked
// for their 32-bit (halo.exe) layout; on a 64-bit host the pointer-bearing checks pass vacuously.
#define PTRS32 (sizeof(void *) == 4)
#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// constants
CHECK(sig_data_array, k_data_array_signature == 0x64407440);
CHECK(sig_iterator, k_data_iterator_signature == 0x69746572);

// pointer-free records
CHECK(datum_header, sizeof(datum_header) == 0x02);
CHECK(crc32_table, sizeof(crc32_table) == 0x400);
CHECK(struct_definition_field, sizeof(struct_definition_field) == 0x0a);
CHECK(data_packet_header, sizeof(data_packet_header) == 0x01);

// data_array (0x38)
CHECK(data_array, !PTRS32 || sizeof(data_array) == 0x38);
CHECK(data_array_maximum_count, OFF(data_array, maximum_count) == 0x20);
CHECK(data_array_size, OFF(data_array, size) == 0x22);
CHECK(data_array_signature, OFF(data_array, signature) == 0x28);
CHECK(data_array_next_index, OFF(data_array, next_index) == 0x2c);
CHECK(data_array_last_index, OFF(data_array, last_index) == 0x2e);
CHECK(data_array_data, OFF(data_array, data) == 0x34);

// data_iterator (0x10): the inline constructors store data, WORD next_index, index and
// data ^ 'iter' (0x430a23..0x430a44); data_iterator_next 0x4d05d0 uses [+0], WORD [+4], [+8].
CHECK(data_iterator, !PTRS32 || sizeof(data_iterator) == 0x10);
CHECK(data_iterator_next_index_size, sizeof(((data_iterator *)0)->next_index) == 2);
CHECK(data_iterator_next_index, !PTRS32 || OFF(data_iterator, next_index) == 0x04);
CHECK(data_iterator_index, !PTRS32 || OFF(data_iterator, index) == 0x08);
CHECK(data_iterator_signature, !PTRS32 || OFF(data_iterator, signature) == 0x0c);

// the remaining size comments in types/memory.h
CHECK(growable_array, !PTRS32 || sizeof(growable_array) == 0x0c);
CHECK(bit_stream, !PTRS32 || sizeof(bit_stream) == 0x18);
CHECK(byte_stream, !PTRS32 || sizeof(byte_stream) == 0x10);
CHECK(circular_buffer, !PTRS32 || sizeof(circular_buffer) == 0x18);
CHECK(struct_definition, !PTRS32 || sizeof(struct_definition) == 0x14);
CHECK(byte_swap_definition, !PTRS32 || sizeof(byte_swap_definition) == 0x14);
CHECK(data_packet_type, !PTRS32 || sizeof(data_packet_type) == 0x08);
CHECK(data_packet_group, !PTRS32 || sizeof(data_packet_group) == 0x30);
CHECK(cache_entry, !PTRS32 || sizeof(cache_entry) == 0x1c);
CHECK(cache, !PTRS32 || sizeof(cache) == 0x7c);
CHECK(cache_allocation_gap, !PTRS32 || sizeof(cache_allocation_gap) == 0x10);
CHECK(memory_pool_block, !PTRS32 || sizeof(memory_pool_block) == 0x18);
CHECK(memory_pool, !PTRS32 || sizeof(memory_pool) == 0x38);
CHECK(heap_block, !PTRS32 || sizeof(heap_block) == 0x10);
CHECK(heap_blocks, !PTRS32 || OFF(heap, blocks) == 0x34); // size 0x34 + maximum_blocks*4
