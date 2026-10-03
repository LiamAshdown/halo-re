#pragma once
// Blam memory module (halo.exe 1.0.10 retail, 0x4cf810..0x4d3980).
// Structures recovered from the decompiled module plus the static definition
// tables in .data. Offsets in comments are byte offsets from the struct base.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// datum handles
// ---------------------------------------------------------------------------
// A datum_index is index in the low 16 bits and identifier (salt) in the high
// 16 bits. Identifier 0 means an empty slot, and 0 as the high half of a handle
// is a wildcard that matches any live slot (datum_get, datum_delete).
typedef uint32_t datum_index;

typedef enum datum_constants {
    k_datum_index_none = -1,               // 0xffffffff
    k_datum_identifier_none = 0,
    k_datum_identifier_wrap = 0x8000       // next_identifier reseeds here on overflow
} datum_constants;

typedef enum memory_signatures {
    k_data_array_signature = 0x64407440,   // 'd@t@' at data_array+0x28
    k_circular_buffer_signature = 0x63697263, // 'circ' at circular_buffer+0x04
    k_cache_signature = 0x77656565,        // 'weee' at cache+0x40
    k_memory_pool_signature = 0x706f6f6c,  // 'pool' at memory_pool+0x00
    k_memory_pool_block_head_signature = 0x68656164, // 'head'
    k_memory_pool_block_tail_signature = 0x7461696c, // 'tail'
    k_byte_swap_definition_signature = 0x62797377,   // 'bysw'
    k_data_iterator_signature = 0x69746572 // 'iter', XORed with the data_array pointer into
                                           // data_iterator.signature
} memory_signatures;

// ---------------------------------------------------------------------------
// growable_array  (growable_array_add_element / growable_array_remove_element)
// GlobalAlloc/GlobalReAlloc backed vector of fixed-size elements.
// ---------------------------------------------------------------------------
typedef struct growable_array {
    int32_t element_size;      // 0x00
    int32_t count;             // 0x04 element count; add_element refuses at 0x7fffffff
    void *data;                // 0x08 element_size*count bytes, NULL when count == 0
#ifdef __cplusplus
    uint32_t add_element();
    void remove_element(uint32_t index);
#endif
} growable_array;              // size 0x0c

// ---------------------------------------------------------------------------
// bit_stream  (bit_stream_write_bit/write_bits/read_bit/read_bits)
// The cursor is kept split: byte_cursor*8 + bit_cursor is the absolute bit
// position, and it is only committed when it stays inside [first_bit, last_bit]
// (or is exactly last_bit+1, i.e. the end of the stream).
// ---------------------------------------------------------------------------
typedef struct bit_stream {
    uint32_t unknown_00;       // 0x00 never read or written by the module
    uint8_t *data;             // 0x04
    uint32_t first_bit;        // 0x08 inclusive lower bound, absolute bit index
    uint32_t byte_cursor;      // 0x0c byte offset into data
    uint32_t bit_cursor;       // 0x10 bit within that byte, 0..7
    uint32_t last_bit;         // 0x14 inclusive upper bound, absolute bit index
#ifdef __cplusplus
    uint32_t read_bit(uint8_t *out_bit);
    uint32_t read_bits(uint32_t bit_count, uint32_t *out_value);
    int32_t read_bits_chunked(int32_t total_bit_count, uint32_t *buffer);
    uint8_t write_bit(int32_t bit_value);
    uint8_t write_bits(uint32_t bit_count, uint32_t value);
    int32_t write_bits_chunked(const uint32_t *values, int32_t total_bit_count);
#endif
} bit_stream;                  // size 0x18

// bit-mask lookup tables shared by bit_stream_write_bits / bit_stream_read_bits
// global 0x0065c2b4: uint8_t bit_mask_clear[8]   entry i = (uint8_t)(0xff << i)
// global 0x0065c2c0: uint8_t bit_mask_keep[9]    entry i = (uint8_t)((1 << i) - 1), entry 8 = 0xff
//   (DAT_0065c2c8 is bit_mask_keep[8]; write_bits indexes it with a negative index)

// ---------------------------------------------------------------------------
// byte_stream  (byte_stream_write_string / read_long / read_string and the
// ranged-integer helpers at 0x4d0700 / 0x4d08a0)
// ---------------------------------------------------------------------------
typedef struct byte_stream {
    uint8_t *data;             // 0x00
    int32_t cursor;            // 0x04 bytes consumed/produced so far
    int32_t size;              // 0x08 capacity of data
    uint8_t overflow;          // 0x0c set once a read/write ran past size
    uint8_t pad_0d[3];         // 0x0d
#ifdef __cplusplus
    uint32_t read_long();
    uint32_t read_ranged_integer(int32_t maximum);
    char *read_string();
    uint32_t write_ranged_integer(int32_t maximum, uint32_t value);
    uint32_t write_string(char *string, int16_t max_length);
#endif
} byte_stream;                 // size 0x10

// ---------------------------------------------------------------------------
// circular_buffer  (circular_buffer_new/write/read)
// circular_buffer_new allocates 0x18 + capacity bytes and points data at the
// bytes directly following the header.
// ---------------------------------------------------------------------------
typedef struct circular_buffer {
    char *name;                // 0x00 constructor argument, not copied
    uint32_t signature;        // 0x04 'circ'
    int32_t read_cursor;       // 0x08 advanced by circular_buffer_read unless peeking
    int32_t write_cursor;      // 0x0c advanced by circular_buffer_write
    int32_t capacity;          // 0x10 requested size + 1 (one slot is kept empty)
    uint8_t *data;             // 0x14 == (uint8_t *)this + 0x18
#ifdef __cplusplus
    static void create(char *name, int32_t requested_size);
    uint32_t read(uint8_t *destination, uint32_t byte_count, char consume);
    uint32_t write(uint32_t byte_count, uint8_t *source);
#endif
} circular_buffer;             // size 0x18, followed by capacity bytes of storage

// ---------------------------------------------------------------------------
// crc32  (crc32_update / crc32_build_table), polynomial 0xedb88320
// ---------------------------------------------------------------------------
typedef struct crc32_table {
    uint32_t entries[256];
#ifdef __cplusplus
    void build();
#endif
} crc32_table;                 // size 0x400
// global 0x006b7b00: crc32_table crc32_lookup_table
// global 0x00719cd8: uint8_t crc32_lookup_table_initialized

// ---------------------------------------------------------------------------
// data_array / datum  (data_new, datum_new, datum_delete, data_delete_all,
// data_iterator_next, datum_get)
// The 0x38 header layout is confirmed by the byte-swap definition named
// data_array_header at 0x0068e39c (codes at 0x0068e364, declared size 56).
// Every element begins with a datum_header: identifier 0 means the slot is free.
// ---------------------------------------------------------------------------
typedef struct datum_header {
    int16_t identifier;        // 0x00 salt, never 0 while the slot is in use
} datum_header;                // size 0x02

typedef struct data_array {
    char name[32];             // 0x00 strncpy of 31 chars + NUL
    int16_t maximum_count;     // 0x20
    int16_t size;              // 0x22 element size in bytes
    uint8_t valid;             // 0x24 data_new leaves 0; cache_new sets 1
    uint8_t pad_25[3];         // 0x25
    uint32_t signature;        // 0x28 'd@t@'
    int16_t next_index;        // 0x2c first index datum_new will try
    int16_t last_index;        // 0x2e high-water mark, one past the last live slot
    int16_t actual_count;      // 0x30 number of live datums
    int16_t next_identifier;   // 0x32 salt counter, reseeded to 0x8000 on wrap
    void *data;                // 0x34 maximum_count*size bytes
#ifdef __cplusplus
    void delete_all();
    static data_array *create(int16_t element_size, char *name, int16_t maximum_count);
    void delete_datum(datum_index handle);
    void initialize_element(void *element);
    void *get(datum_index handle);
    datum_index new_datum();
    datum_index new_at_index(int16_t index);
    datum_index new_at_index_with_salt(datum_index requested_handle);
    datum_index next_datum(int16_t after_index);
#endif
} data_array;                  // size 0x38, data_new allocates 0x38 + maximum_count*size

// data_iterator: never built by a constructor function; every caller builds it inline on the
// stack as four stores (e.g. 0x430a23..0x430a44, 0x444417..0x444437, 0x45c6a3..0x45c6c6):
//   [+0x00] = data, WORD [+0x04] = 0, [+0x08] = -1, [+0x0c] = data ^ 'iter'
// data_iterator_next (0x4d05d0) reads [+0x00], reads and writes only WORD [+0x04], and writes
// [+0x08]; it never checks +0x0c (144 `xor reg,0x69746572` sites build the signature).
typedef struct data_iterator {
    data_array *data;          // 0x00
    int16_t next_index;        // 0x04 index data_iterator_next resumes from (word access only)
    uint8_t pad_06[2];         // 0x06 never written by the inline constructors
    datum_index index;         // 0x08 handle of the element last returned
    uint32_t signature;        // 0x0c (uint32_t)data ^ k_data_iterator_signature
#ifdef __cplusplus
    void *next();
#endif
} data_iterator;               // size 0x10

// ---------------------------------------------------------------------------
// struct_definition  (struct_definition_compute_size / _encode / _decode)
// Versioned serialization descriptors. The field list is a flat array of
// 5-int16 records terminated by a record of type _struct_field_terminator.
// A field is skipped (zero-filled on decode) unless
// minimum_version <= definition->version and
// (definition->version <= maximum_version || maximum_version == 0).
// ---------------------------------------------------------------------------
typedef enum struct_field_type {
    _struct_field_unused = 0,        // count bytes reserved, nothing transmitted
    _struct_field_data = 1,          // count raw bytes
    _struct_field_int16_array = 2,   // count 16-bit values, byte swapped
    _struct_field_int32_array = 3,   // count 32-bit values, byte swapped
    _struct_field_int64_array = 4,   // count 64-bit values, byte swapped
    _struct_field_string = 5,        // up to count chars plus a NUL (count+1 bytes)
    _struct_field_variable_data = 6, // int16 element count then that many bytes (count+2)
    _struct_field_struct_array = 7,  // int16 element count then count sub-structures;
                                     //   the sub-structure fields follow inline at field+1
    _struct_field_block = 8,         // count raw bytes, never byte swapped
    _struct_field_terminator = 9
} struct_field_type;

typedef struct struct_definition_field {
    int16_t type;              // 0x00 struct_field_type
    int16_t count;             // 0x02 element count / string capacity / byte count
    int16_t minimum_version;   // 0x04
    int16_t maximum_version;   // 0x06 0 means no upper bound
    int16_t computed_size;     // 0x08 filled in by struct_definition_compute_size
} struct_definition_field;     // size 0x0a

typedef struct struct_definition {
    char *name;                // 0x00
    int32_t unknown_04;        // 0x04 zero in every instance in the image
    int16_t size;              // 0x08 encoded size of the structure in bytes
    int16_t version;           // 0x0a current version, written as a byte when > 0
    struct_definition_field *fields; // 0x0c
    uint8_t size_computed;     // 0x10 set once compute_size has filled the fields
    uint8_t pad_11[3];         // 0x11
#ifdef __cplusplus
    void compute_size(int16_t *out_size, struct_definition_field *fields, int16_t *out_field_count);
    void decode(byte_stream *input, int16_t version, void *dest_instance, int16_t *out_dest_size, struct_definition_field *fields, int16_t *out_field_count);
    void encode(byte_stream *output, int16_t version, void *source, int16_t *out_source_size, struct_definition_field *fields, int16_t *out_field_count);
    uint8_t decode_packet_body(uint8_t *buffer, int16_t remaining_length, void *dest, uint16_t *out_version_used, int16_t *out_bytes_consumed);
    int32_t encode_packet_body(int16_t version, uint8_t *version_byte_dest, byte_stream *output, void *source, int16_t *out_wrote_version_byte, int16_t capacity_check);
#endif
} struct_definition;           // size 0x14

// ---------------------------------------------------------------------------
// byte_swap_definition  (struct_definition_byte_swap)
// A separate, unversioned descriptor family used purely for endian conversion.
// codes[0] is _byte_swap_begin_struct, codes[1] is the number of structure
// records in the table, and the field codes start at codes[2]:
//   n  > 0   skip n bytes
//   -2/-4/-8 swap a 16/32/64-bit value
//   -100     begin a nested structure record (count in the next int32)
//   -101     end of the current structure record
//   -102     the next int32 is a byte_swap_definition *; recurse into its codes
// ---------------------------------------------------------------------------
typedef enum byte_swap_code {
    _byte_swap_definition_reference = -102, // 0xffffff9a
    _byte_swap_end_struct = -101,           // 0xffffff9b
    _byte_swap_begin_struct = -100,         // 0xffffff9c
    _byte_swap_int64 = -8,
    _byte_swap_int32 = -4,
    _byte_swap_int16 = -2
} byte_swap_code;

typedef struct byte_swap_definition {
    char *name;                // 0x00
    int32_t size;              // 0x04 size of one structure in bytes
    int32_t *codes;            // 0x08
    uint32_t signature;        // 0x0c 'bysw'
    int32_t unknown_10;        // 0x10 zero in every instance in the image
#ifdef __cplusplus
    void swap(int32_t data, int32_t *codes, int32_t *out_size, int32_t *out_record_count);
#endif
} byte_swap_definition;        // size 0x14
// global 0x00696780: byte_swap_definition packet_header_byte_swap_definition
// global 0x00696770: int32_t packet_header_byte_swap_codes[4]
// global 0x0068e39c: byte_swap_definition data_array_header_byte_swap_definition
// global 0x0068e364: int32_t data_array_header_byte_swap_codes[14]

// ---------------------------------------------------------------------------
// data_packet_group  (data_packet_group_encode_packet / _decode_packet /
// _append_packet_header)
// A packet is [1 byte type][optional 1 byte version][body]. The type byte is
// run through the packet_header byte_swap_definition before use.
// ---------------------------------------------------------------------------
typedef struct data_packet_header {
    uint8_t type;              // 0x00
} data_packet_header;          // size 0x01

typedef struct data_packet_type {
    int16_t packet_class;      // 0x00 checked against the class the caller asked for
    int16_t pad_02;            // 0x02
    struct_definition *definition; // 0x04 NULL for a body-less packet
} data_packet_type;            // size 0x08

typedef struct data_packet_group {
    char *name;                // 0x00
    int16_t type_count;        // 0x04 39 for the network game message group
    int16_t class_count;       // 0x06 8; classes seen in the table are 0..7
    int32_t maximum_decoded_size; // 0x08 0x600
    int32_t maximum_encoded_size; // 0x0c 0x800, the append_packet_header limit
    data_packet_type *types;   // 0x10 type_count entries
    int32_t unknown_14;        // 0x14 0
    int32_t unknown_18;        // 0x18 0x35
    int32_t unknown_1c;        // 0x1c 0xffffffff
    int32_t unknown_20;        // 0x20 0xffffffff
    int32_t unknown_24;        // 0x24 0xffffffff
    int32_t unknown_28;        // 0x28 0xffffffff
    int32_t unknown_2c;        // 0x2c 0x10
#ifdef __cplusplus
    void compute_sizes();
    int32_t append_packet_header(uint8_t *buffer, int16_t *cursor, uint8_t header_byte);
    int32_t decode_packet(int16_t *remaining_length, void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    int32_t encode_packet(int16_t version, struct_definition *definition, uint8_t *version_byte_dest, byte_stream *output, uint8_t *buffer, int16_t *cursor, void *source, int16_t *out_wrote_version_byte, uint8_t packet_type);
#endif
} data_packet_group;           // size 0x30
// global 0x006994f8: data_packet_group network_game_messages_group
// global 0x006993c0: data_packet_type network_game_messages_types[39]
// global 0x006b7f00: char *data_packet_group_error  (last failure string, NULL on success)

// ---------------------------------------------------------------------------
// cache  (cache_new, cache_flush, cache_allocate_block @0x4d1840,
// cache_evict_entry, cache_build_status_bitmap @0x4d1ca0)
// A fixed byte region divided into 1 << block_shift byte blocks. Entries are
// datums in an embedded data_array and are kept in a doubly linked list ordered
// by block offset. The address of the backing region itself is NOT stored here;
// each user keeps it (e.g. the sound cache keeps it in DAT_006ac52c).
// ---------------------------------------------------------------------------
typedef struct cache_entry {
    int16_t identifier;        // 0x00 datum_header
    int16_t pad_02;            // 0x02
    int32_t size;              // 0x04 size in blocks
    int32_t offset;            // 0x08 offset in blocks from the start of the region
    datum_index next;          // 0x0c next entry by offset, k_datum_index_none at the end
    datum_index previous;      // 0x10 previous entry by offset
    uint32_t age;              // 0x14 cache->age when the entry was last touched
    uint32_t unknown_18;       // 0x18 never written by the module
} cache_entry;                 // size 0x1c

typedef struct cache {
    char name[32];             // 0x00
    void *release_procedure;   // 0x20 called by cache_evict_entry before the datum is freed
    void *in_use_procedure;    // 0x24 predicate: nonzero means the entry must not be evicted
    int32_t block_count;       // 0x28 capacity of the region, in blocks
    int32_t block_shift;       // 0x2c log2 of the block size in bytes
    uint32_t age;              // 0x30 monotonically increasing use counter, starts at 1
    datum_index first;         // 0x34 head of the by-offset entry list
    datum_index last;          // 0x38 tail of the by-offset entry list
    data_array *entries;       // 0x3c == &entry_data
    uint32_t signature;        // 0x40 'weee'
    data_array entry_data;     // 0x44 element size 0x1c, storage starts at 0x7c
#ifdef __cplusplus
    void initialize(char *name, int32_t block_count, int32_t block_shift, int16_t maximum_count, void *release_procedure, void *in_use_procedure);
    datum_index allocate_block(uint32_t requested_bytes);
    void build_status_bitmap(uint8_t *bitmap);
    void evict_entry(datum_index handle);
    void flush();
    cache_entry *entry_at(datum_index handle);
#endif
} cache;                       // size 0x7c, followed by maximum_count*0x1c bytes

// Per-byte status codes produced by cache_build_status_bitmap (0x4d1ca0).
typedef enum cache_block_status_flags {
    _cache_block_allocated_bit = 1,  // some entry covers this block
    _cache_block_current_bit = 2,    // entry age == cache->age
    _cache_block_stale_bit = 4,      // entry age + 30 < cache->age
    _cache_block_locked_bit = 8      // in_use_procedure said the entry is busy
} cache_block_status_flags;

// Stack-local record used by cache_allocate_block while it walks the region
// looking for (or making) a run of free blocks; 256 of them in a ring buffer.
typedef struct cache_allocation_gap {
    datum_index previous_entry; // 0x00 entry the gap starts after
    uint32_t newest_age;        // 0x04 age of the newest entry that must be evicted
    int32_t offset;             // 0x08 first block of the gap
    int32_t size;               // 0x0c blocks accumulated so far
} cache_allocation_gap;         // size 0x10

// ---------------------------------------------------------------------------
// memory_pool / block list  (block_list_allocate, block_list_reallocate,
// block_list_unlink, block_list_compact; constructed by game_state_new_pool
// at 0x00538150)
// Bump allocator over one arena. Blocks are sentinel tagged and carry a back
// pointer to the pointer variable owned by the caller, so compaction can relocate them.
// ---------------------------------------------------------------------------
typedef struct memory_pool_block {
    uint32_t head_signature;   // 0x00 'head'
    int32_t size;              // 0x04 whole block including this header, rounded up to 4
    void **address;            // 0x08 points at the owner pointer that holds data
    struct memory_pool_block *next;     // 0x0c higher address, NULL at the end
    struct memory_pool_block *previous; // 0x10 lower address, NULL at the start
    uint32_t tail_signature;   // 0x14 'tail'
} memory_pool_block;           // size 0x18 header, payload follows at 0x18

typedef struct memory_pool {
    uint32_t signature;        // 0x00 'pool'
    char name[32];             // 0x04
    void *base;                // 0x24 == (uint8_t *)this + 0x38
    int32_t size;              // 0x28 bytes of storage after the header
    int32_t free_bytes;        // 0x2c
    memory_pool_block *first_block; // 0x30 lowest address block
    memory_pool_block *last_block;  // 0x34 highest address block
#ifdef __cplusplus
    int32_t allocate(int32_t requested_size, void **owner);
    void compact();
    int32_t reallocate(void **owner_cell, int32_t new_size);
    void unlink(void **payload_ptr);
#endif
} memory_pool;                 // size 0x38, followed by size bytes of storage
// global 0x006b8cb4: memory_pool *object_memory_pool

// ---------------------------------------------------------------------------
// heap  (heap_allocate, heap_reallocate, heap_resize_block @0x4d2020,
// heap_unlink_block @0x4d20a0, heap_get_free_bytes @0x4d20f0,
// heap_find_first_free_slot @0x4d2110, heap_advance_free_slot @0x4d2140,
// heap_allocate_raw @0x4d2180, heap_compact @0x4d2310,
// heap_find_free_block @0x4d2370)
// Blocks live in one address-ordered doubly linked list inside a single
// region; the trailing pointer table has one slot per live block and a NULL
// slot means free, so it is an allocation slot table rather than size buckets.
// ---------------------------------------------------------------------------
typedef struct heap_block {
    uint32_t size;             // 0x00 total size including this header; bit 31 set while in use
    int32_t slot;              // 0x04 index of this block in heap::blocks
    struct heap_block *previous; // 0x08 lower address
    struct heap_block *next;     // 0x0c higher address
} heap_block;                  // size 0x10 header, payload follows at 0x10

// heap_block::size carries the in-use flag in bit 31 (0x80000000); mask it off
// with k_heap_block_size_mask to get the real size.
typedef enum heap_block_flags {
    k_heap_block_size_mask = 0x7fffffff
} heap_block_flags;

typedef struct heap {
    uint32_t unknown_00;       // 0x00 never touched by the module
    uint8_t *base;             // 0x04 start of the managed region
    int32_t size;              // 0x08 bytes in the region
    int32_t maximum_blocks;    // 0x0c number of entries in blocks[]
    int32_t next_free_slot;    // 0x10 cached first free blocks[] index, -1 when unknown/full
    int32_t bytes_allocated;   // 0x14 running total, block headers included
    int32_t peak_bytes_allocated;   // 0x18
    int32_t allocation_count;  // 0x1c
    int32_t peak_allocation_count;  // 0x20
    int32_t peak_allocation_size;   // 0x24 largest single block ever handed out
    uint8_t compaction_disabled;    // 0x28 heap_compact returns immediately when nonzero
    uint8_t pad_29[3];         // 0x29
    heap_block *first_block;   // 0x2c lowest address block
    heap_block *last_block;    // 0x30 highest address block
    heap_block *blocks[1];     // 0x34 maximum_blocks entries, NULL means the slot is free
#ifdef __cplusplus
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
#endif
} heap;                        // size 0x34 + maximum_blocks*4

#pragma pack(pop)
