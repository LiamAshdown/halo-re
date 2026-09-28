exec(open(r'C:\Users\Liam-\halo-re\scratchpad\md_lib.py').read())
e = emit
DESC = 'int32_t *descriptor = (int32_t *)field_type->array_descriptor;\n'

# ---------------------------------------------------------------- kind 0: integer of a subtype width
e(0x4e89c0, 58, 'message_delta_integer_compute_size', 'bits by the descriptor  subtype (no range check): 0 byte 8, 1 short 16, 2 long 32, 3..6 packed 1, 3, 5, 6.', '''
int32_t message_delta_integer_compute_size(message_delta_field_type *field_type)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return 8;
    case 1:
        return 0x10;
    case 2:
        return 0x20;
    case 3:
        return 1;
    case 4:
        return 3;
    case 5:
        return 5;
    default:
        return 6;
    }
}
''')
e(0x4e8a20, 24, 'message_delta_integer_initialize', 'a subtype in 0..27 is valid (also the initializer of kind 11).', '''
uint8_t message_delta_integer_initialize(message_delta_field_type *field_type)
{
    int32_t subtype = *(int32_t *)field_type->array_descriptor;

    return subtype >= 0 && subtype < 0x1c;
}
''')
WIDTHS = [(0, 8, 'uint8_t'), (1, 16, 'uint16_t'), (2, 32, 'uint32_t'), (3, 1, 'uint8_t'), (4, 3, 'uint8_t'),
          (5, 5, 'uint8_t'), (6, 6, 'uint8_t')]
cases = ''.join('''    case %d:
        if (previous != 0 && *(%s *)previous == *(%s *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, %d);
''' % (k, t, t, w) for k, w, t in WIDTHS)
e(0x4e8a40, 272, 'message_delta_integer_encode', 'unchanged from the previous value (compared at its width): 0; else the value at the subtype width. Subtypes above 6: 0.', '''
int32_t message_delta_integer_encode''' + SIG + '''
{
    switch (*(int32_t *)field_type->array_descriptor) {
''' + cases + '''    }
    return 0;
}
''')
dcases = ''.join('''    case %d:
%s        return bit_stream_read_bits_chunked(%d, (uint32_t *)current, stream);
''' % (k, '        *(uint8_t *)current = 0;\n' if k >= 3 else '', w) for k, w, t in WIDTHS)
e(0x4e8b70, 200, 'message_delta_integer_decode', 'reads the subtype width into the destination (the packed widths clear the byte first); subtypes above 6: 0.', '''
int32_t message_delta_integer_decode''' + SIG + '''
{
    (void)previous;
    switch (*(int32_t *)field_type->array_descriptor) {
''' + dcases + '''    }
    return 0;
}
''')

# ---------------------------------------------------------------- constant sizes
for addr, name, bits, note in ((0x4e8c60, 'message_delta_compute_size_32', 0x20, 'real, time and the 32-bit kinds'),
                               (0x4e8cb0, 'message_delta_compute_size_1', 1, 'the boolean kind'),
                               (0x4e8d20, 'message_delta_compute_size_8', 8, 'the byte kind'),
                               (0x4e8d80, 'message_delta_compute_size_16', 0x10, 'the short kind'),
                               (0x4ea600, 'message_delta_compute_size_6', 6, 'the grenade counts kind')):
    e(addr, 6, name, '%d bits (%s).' % (bits, note), '''
int32_t %s(message_delta_field_type *field_type)
{
    (void)field_type;
    return %d;
}
''' % (name, bits))

e(0x4e8c70, 64, 'message_delta_real_encode', 'unchanged within +/-0.0001 of the previous value: 0; else its 32 bits.', '''
int32_t message_delta_real_encode''' + SIG + '''
{
    (void)field_type;
    if (previous != 0) {
        real delta = *(real *)previous - *(real *)current;

        if (!(delta < -0.0001f) && !(delta > 0.0001f)) {
            return 0;
        }
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 0x20);
}
''')
e(0x4e8cc0, 52, 'message_delta_boolean_encode', 'an unchanged byte: 0; else the byte as one bit (1 when written).', '''
int32_t message_delta_boolean_encode''' + SIG + '''
{
    uint8_t value = *(uint8_t *)current;

    (void)field_type;
    if (previous != 0 && *(uint8_t *)previous == value) {
        return 0;
    }
    return bit_stream_write_bit(value, stream) ? 1 : 0;
}
''')
e(0x4e8d00, 21, 'message_delta_boolean_decode', 'clears the byte and reads one bit into it.', '''
int32_t message_delta_boolean_decode''' + SIG + '''
{
    (void)field_type;
    (void)previous;
    *(uint8_t *)current = 0;
    return (int32_t)bit_stream_read_bit((uint8_t *)current, stream);
}
''')
e(0x4e8d30, 35, 'message_delta_byte_encode', 'an unchanged byte: 0; else 8 bits.', '''
int32_t message_delta_byte_encode''' + SIG + '''
{
    (void)field_type;
    if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 8);
}
''')
e(0x4e8d60, 23, 'message_delta_byte_decode', '8 bits into the byte.', '''
int32_t message_delta_byte_decode''' + SIG + '''
{
    (void)field_type;
    (void)previous;
    return bit_stream_read_bits_chunked(8, (uint32_t *)current, stream);
}
''')
e(0x4ea430, 35, 'message_delta_long_encode', 'an unchanged dword: 0; else 32 bits.', '''
int32_t message_delta_long_encode''' + SIG + '''
{
    (void)field_type;
    if (previous != 0 && *(uint32_t *)previous == *(uint32_t *)current) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 0x20);
}
''')
e(0x4ea460, 23, 'message_delta_long_decode', '32 bits into the dword.', '''
int32_t message_delta_long_decode''' + SIG + '''
{
    (void)field_type;
    (void)previous;
    return bit_stream_read_bits_chunked(0x20, (uint32_t *)current, stream);
}
''')

# ---------------------------------------------------------------- strings
e(0x4e8d90, 25, 'message_delta_string_compute_size', 'the length header (bits for 0..count) is the reserved-bit count; plus 8 bits per character.', DESC.replace('int32_t *descriptor = ', 'extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f\n\nint32_t message_delta_string_compute_size(message_delta_field_type *field_type)\n{\n    int32_t *descriptor = ') + '''    field_type->reserved_bits = message_delta_item_count_bits[descriptor[0] + 1];
    return field_type->reserved_bits + descriptor[0] * 8;
}
''')
e(0x4e8db0, 14, 'message_delta_count_initialize', 'a positive element count is valid (strings, blobs, points, vectors).', '''
uint8_t message_delta_count_initialize(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor > 0;
}
''')
e(0x4e8dc0, 242, 'message_delta_string_encode', 'unchanged (strcmp): 0; else the length in the reserved bits and 8 bits per character.', '''
int32_t message_delta_string_encode''' + SIG + '''
{
    const char *string = (const char *)current;
    int32_t length = (int32_t)strlen(string);
    int32_t bits;
    int32_t i;

    if (previous != 0 && strcmp((const char *)previous, string) == 0) {
        return 0;
    }
    bits = bit_stream_write_bits_chunked(stream, (const uint32_t *)&length, field_type->reserved_bits);
    for (i = 0; i < length; i++) {
        bits += bit_stream_write_bits_chunked(stream, (const uint32_t *)(string + i), 8);
    }
    return bits;
}
''')
e(0x4e8ec0, 103, 'message_delta_string_decode', 'the length from the reserved bits; within 0..count, that many 8-bit characters and a NUL.', '''
int32_t message_delta_string_decode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    char *string = (char *)current;
    int32_t length = 0;
    int32_t bits = bit_stream_read_bits_chunked(field_type->reserved_bits, (uint32_t *)&length, stream);
    int32_t i;

    (void)previous;
    if (length < 0 || length > descriptor[0]) {
        return bits;
    }
    for (i = 0; i < length; i++) {
        bits += bit_stream_read_bits_chunked(8, (uint32_t *)(string + i), stream);
    }
    string[length] = 0;
    return bits;
}
''')
e(0x4e8f30, 27, 'message_delta_wide_string_compute_size', 'the length header bits plus 16 per character.', '''
extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f

int32_t message_delta_wide_string_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    field_type->reserved_bits = message_delta_item_count_bits[descriptor[0] + 1];
    return descriptor[0] * 0x10 + field_type->reserved_bits;
}
''')
e(0x4e8f50, 202, 'message_delta_wide_string_encode', 'unchanged (wcscmp): 0; else the length and 16 bits per character.', '''
#include <wchar.h>

int32_t message_delta_wide_string_encode''' + SIG + '''
{
    const wchar_t *string = (const wchar_t *)current;
    int32_t length = (int32_t)wcslen(string);
    int32_t bits;
    int32_t i;

    if (previous != 0 && wcscmp((const wchar_t *)previous, string) == 0) {
        return 0;
    }
    bits = bit_stream_write_bits_chunked(stream, (const uint32_t *)&length, field_type->reserved_bits);
    for (i = 0; i < length; i++) {
        bits += bit_stream_write_bits_chunked(stream, (const uint32_t *)(string + i), 0x10);
    }
    return bits;
}
''')
e(0x4e9020, 114, 'message_delta_wide_string_decode', 'the length; within 0..count, that many 16-bit characters and a NUL.', '''
int32_t message_delta_wide_string_decode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint16_t *string = (uint16_t *)current;
    int32_t length = 0;
    int32_t bits = bit_stream_read_bits_chunked(field_type->reserved_bits, (uint32_t *)&length, stream);
    int32_t i;

    (void)previous;
    if (length < 0 || length > descriptor[0]) {
        return bits;
    }
    for (i = 0; i < length; i++) {
        bits += bit_stream_read_bits_chunked(0x10, (uint32_t *)(string + i), stream);
    }
    string[length] = 0;
    return bits;
}
''')
e(0x4e90a0, 13, 'message_delta_blob_compute_size', '8 bits per byte of the count.', '''
int32_t message_delta_blob_compute_size(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor << 3;
}
''')

# ---------------------------------------------------------------- kind 8: array of structures {count, element size, type}
e(0x4e90b0, 53, 'message_delta_structure_array_compute_size', 'the element type  size (cached in it); one changed flag per element (reserved bits = count); count * element + count.', '''
int32_t message_delta_structure_array_compute_size(message_delta_field_type *field_type)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    int32_t count = descriptor->count;
    int32_t element_bits = MESSAGE_DELTA_COMPUTE_SIZE(descriptor->field_type);

    descriptor->field_type->size_bits = element_bits;
    field_type->reserved_bits = count;
    return descriptor->count * element_bits + count;
}
''')
e(0x4e90f0, 52, 'message_delta_structure_array_initialize', 'a positive count and element size and an element type that initializes.', '''
uint8_t message_delta_structure_array_initialize(message_delta_field_type *field_type)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;

    if (descriptor->count <= 0 || descriptor->element_size <= 0 || descriptor->field_type == 0) {
        return 0;
    }
    return MESSAGE_DELTA_INITIALIZE(descriptor->field_type) == 1;
}
''')
e(0x4e9130, 509, 'message_delta_structure_array_encode', 'without a previous array every element is encoded in full. Otherwise a block of changed flags (reserved bits) is skipped, each element is delta-encoded after it and its flag written back (seeking absolutely from first_bit); the bits plus the flags, or -- nothing changed -- a rewind to the flag block and 0.', '''
int32_t message_delta_structure_array_encode''' + SIG + '''
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t block;
    int32_t flag;
    int32_t i;

    if (previous == 0) {
        for (i = 0; i < descriptor->count; i++) {
            total += MESSAGE_DELTA_ENCODE(descriptor->field_type, 0, (uint8_t *)current + descriptor->element_size * i,
                stream);
        }
        return total;
    }
    block = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
    message_delta_stream_seek(stream, message_delta_stream_position(stream), field_type->reserved_bits);
    flag = block;
    for (i = 0; i < descriptor->count; i++) {
        int32_t offset = descriptor->element_size * i;
        int32_t bits = MESSAGE_DELTA_ENCODE(descriptor->field_type, (uint8_t *)previous + offset,
            (uint8_t *)current + offset, stream);
        int32_t data;

        total += bits;
        data = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
        message_delta_stream_seek(stream, stream->first_bit, flag);
        bit_stream_write_bit(bits > 0, stream);
        message_delta_stream_seek(stream, stream->first_bit, data);
        flag++;
    }
    if (total > 0) {
        return total + field_type->reserved_bits;
    }
    message_delta_stream_seek(stream, stream->first_bit, block);
    return total;
}
''')

# ---------------------------------------------------------------- kind 9: compound record {count, bindings}
e(0x4e9530, 76, 'message_delta_compound_compute_size', 'each binding  type size (cached in it) summed; one flag per binding (reserved bits = count).', '''
int32_t message_delta_compound_compute_size(message_delta_field_type *field_type)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t i;

    for (i = 0; i < list->count; i++) {
        int32_t bits = MESSAGE_DELTA_COMPUTE_SIZE(list->fields[i].field_type);

        total += bits;
        list->fields[i].field_type->size_bits = bits;
    }
    field_type->reserved_bits = list->count;
    return list->count + total;
}
''')
e(0x4e9580, 94, 'message_delta_compound_initialize', 'a positive count and every binding  type present and initializing.', '''
uint8_t message_delta_compound_initialize(message_delta_field_type *field_type)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    uint8_t result = message_delta_field_type_table[9].unknown_00[4] != 1;
    int32_t i;

    if (list->count <= 0) {
        return 0;
    }
    for (i = 0; i < list->count; i++) {
        message_delta_field_binding *binding = &list->fields[i];

        if (binding == 0 || binding->field_type == 0) {
            return 0;
        }
        result = MESSAGE_DELTA_INITIALIZE(binding->field_type);
        if (result != 1) {
            return 0;
        }
    }
    return result;
}
''')
e(0x4e97e0, 486, 'message_delta_compound_decode', 'without a previous record each binding decodes into destination + its destination offset. Otherwise the flag block is skipped and per binding its flag is read (seeking back to it) and, when set, the binding decodes from previous + source offset into destination + destination offset; the bits plus the flags when any, else the (zero) bits.', '''
int32_t message_delta_compound_decode''' + SIG + '''
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    int32_t total = 0;
    int32_t flag;
    int32_t i;

    if (previous == 0) {
        for (i = 0; i < list->count; i++) {
            total += MESSAGE_DELTA_DECODE(list->fields[i].field_type, 0,
                (uint8_t *)current + list->fields[i].destination_offset, stream);
        }
        return total;
    }
    flag = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
    message_delta_stream_seek(stream, message_delta_stream_position(stream), field_type->reserved_bits);
    for (i = 0; i < list->count; i++) {
        message_delta_field_binding *binding = &list->fields[i];
        int32_t data = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
        uint8_t changed = 0;

        message_delta_stream_seek(stream, stream->first_bit, flag);
        bit_stream_read_bit(&changed, stream);
        message_delta_stream_seek(stream, stream->first_bit, data);
        if (changed) {
            total += MESSAGE_DELTA_DECODE(binding->field_type, (uint8_t *)previous + binding->source_offset,
                (uint8_t *)current + binding->destination_offset, stream);
        }
        flag++;
    }
    if (total > 0) {
        return total + field_type->reserved_bits;
    }
    return total;
}
''')

# ---------------------------------------------------------------- kind 10: pointer to a value of another type
e(0x4e99d0, 33, 'message_delta_pointer_compute_size', 'the pointed type  size, cached in it.', '''
int32_t message_delta_pointer_compute_size(message_delta_field_type *field_type)
{
    message_delta_field_type **descriptor = (message_delta_field_type **)field_type->array_descriptor;
    int32_t bits = MESSAGE_DELTA_COMPUTE_SIZE(descriptor[0]);

    descriptor[0]->size_bits = bits;
    return bits;
}
''')
e(0x4e9a00, 41, 'message_delta_pointer_initialize', 'the pointed type exists and initializes.', '''
uint8_t message_delta_pointer_initialize(message_delta_field_type *field_type)
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;

    if (pointed == 0) {
        return 0;
    }
    return MESSAGE_DELTA_INITIALIZE(pointed) == 1;
}
''')
for addr, name, which in ((0x4e9a30, 'message_delta_pointer_encode', 'ENCODE'), (0x4e9a60, 'message_delta_pointer_decode', 'DECODE')):
    e(addr, 46, name, 'the pointed type codes through the stored pointers (previous: its pointer, or NULL).', '''
int32_t %s''' % name + SIG + '''
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;
    void *previous_value = previous != 0 ? *(void **)previous : 0;

    return MESSAGE_DELTA_%s(pointed, previous_value, *(void **)current, stream);
}
''' % which)

# ---------------------------------------------------------------- kinds 11, 12
e(0x4e9a90, 35, 'message_delta_enum_width_compute_size', 'subtype 0: 1 bit, 1: 2 bits, else 4.', '''
int32_t message_delta_enum_width_compute_size(message_delta_field_type *field_type)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return 1;
    case 1:
        return 2;
    default:
        return 4;
    }
}
''')
e(0x4e9ac0, 20, 'message_delta_range_compute_size', 'the bits for maximum - minimum (item count table).', '''
extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f

int32_t message_delta_range_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return message_delta_item_count_bits[descriptor[1] - descriptor[0] + 1];
}
''')
e(0x4e9ae0, 16, 'message_delta_range_initialize', 'maximum above minimum.', '''
uint8_t message_delta_range_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return descriptor[1] > descriptor[0];
}
''')

# ---------------------------------------------------------------- kind 13: index through a translation cache
e(0x4e9af0, 20, 'message_delta_index_compute_size', 'the bits for the count, kept in the descriptor (+8) and returned.', '''
extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f

int32_t message_delta_index_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    descriptor[2] = message_delta_item_count_bits[descriptor[0] + 1];
    return descriptor[2];
}
''')
e(0x4e9b10, 125, 'message_delta_index_initialize', 'a positive count and bucket count; once per type: the hash table at +0x0c (bucket count), a GlobalAlloc  count-long table of -1 at +0x28 (+0x24 cleared), key -1 mapped to 0 and slot 0 taken (1).', '''
extern void hash_table_initialize(hash_table *table, int32_t bucket_count); // 0x4f0470, ESI table, EAX buckets
extern void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value); // 0x4f0530
extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);

uint8_t message_delta_index_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (descriptor[0] < 1 || descriptor[1] < 1) {
        return 0;
    }
    if (field_type->initialized == 0) {
        int32_t *table;

        hash_table_initialize((hash_table *)(descriptor + 3), descriptor[1]);
        table = (int32_t *)GlobalAlloc(0, descriptor[0] * 4);
        descriptor[10] = (int32_t)table;
        descriptor[9] = 0;
        memset(table, 0xff, descriptor[0] * 4);
        hash_table_set_or_remove((hash_table *)(descriptor + 3), -1, 0);
        *(int32_t *)descriptor[10] = 1;
    }
    return 1;
}
''')
e(0x4e9b90, 43, 'message_delta_index_teardown', 'frees the slot table (GlobalFree) and disposes the hash table (the descriptor is taken only when kind 13  table flag is 1).', '''
extern void hash_table_dispose(hash_table *table); // 0x4f04c0, EDI table
extern void *__stdcall GlobalFree(void *memory);

void message_delta_index_teardown(message_delta_field_type *field_type)
{
    int32_t *descriptor = message_delta_field_type_table[13].unknown_00[4] == 1 ? (int32_t *)field_type->array_descriptor : 0;

    GlobalFree((void *)descriptor[10]);
    hash_table_dispose((hash_table *)(descriptor + 3));
}
''')
e(0x4e9bc0, 47, 'message_delta_index_encode', 'an unchanged dword: 0; else its bits (descriptor +8).', '''
int32_t message_delta_index_encode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (previous != 0 && *(uint32_t *)current == *(uint32_t *)previous) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, descriptor[2]);
}
''')
e(0x4e9bf0, 46, 'message_delta_index_decode', 'reads the bits (descriptor +8) into a zeroed dword, stored in full.', '''
int32_t message_delta_index_decode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint32_t value = 0;
    int32_t bits = bit_stream_read_bits_chunked(descriptor[2], &value, stream);

    (void)previous;
    *(uint32_t *)current = value;
    return bits;
}
''')

# ---------------------------------------------------------------- kinds 14/15: {count} 32-bit scalars
e(0x4ea220, 20, 'message_delta_scalar_array_compute_size', 'one flag per component (reserved bits = count) and 32 bits each.', '''
int32_t message_delta_scalar_array_compute_size(message_delta_field_type *field_type)
{
    int32_t count = *(int32_t *)field_type->array_descriptor;

    field_type->reserved_bits = count;
    return (count << 5) + count;
}
''')
e(0x4ea240, 5, 'message_delta_vector_encode', 'jumps to message_delta_float_array_encode 0x4e9db0.', '''
extern int32_t message_delta_float_array_encode(message_delta_field_type *field_type, float *previous, float *values,
    bit_stream *stream); // 0x4e9db0

int32_t message_delta_vector_encode''' + SIG + '''
{
    return message_delta_float_array_encode(field_type, (float *)previous, (float *)current, stream);
}
''')
e(0x4ea250, 5, 'message_delta_vector_decode', 'jumps to message_delta_dword_array_decode 0x4ea040.', '''
extern int32_t message_delta_dword_array_decode''' + SIG + '''; // 0x4ea040

int32_t message_delta_vector_decode''' + SIG + '''
{
    return message_delta_dword_array_decode(field_type, previous, current, stream);
}
''')

# ---------------------------------------------------------------- kind 17: flag bits {count, mask[count]}
e(0x4ea260, 67, 'message_delta_flags_initialize', '1..32 flags, each mask byte 0 or 1.', '''
uint8_t message_delta_flags_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    int32_t i;

    if (descriptor[0] <= 0 || descriptor[0] > 0x20) {
        return 0;
    }
    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] != 1 && mask[i] != 0) {
            return 0;
        }
    }
    return 1;
}
''')
e(0x4ea2b0, 256, 'message_delta_flags_encode', 'every masked flag is written as one bit; when none differs from the previous value (with one) the cursor rewinds to where it started and 0 comes back, else the bits written.', '''
int32_t message_delta_flags_encode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    uint32_t value = *(uint32_t *)current;
    int32_t start = (int32_t)(message_delta_stream_position(stream) - stream->first_bit);
    int32_t total = 0;
    uint8_t changed = 0;
    int32_t i;

    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] != 1) {
            continue;
        }
        if (previous == 0 || changed ||
            ((*(uint32_t *)previous & (1u << i)) != 0) != ((value & (1u << i)) != 0)) {
            changed = 1;
        }
        total += bit_stream_write_bit((value & (1u << i)) != 0, stream) ? 1 : 0;
    }
    if (changed) {
        return total;
    }
    message_delta_stream_seek(stream, stream->first_bit, start);
    return 0;
}
''')
e(0x4ea3b0, 116, 'message_delta_flags_decode', 'each masked flag read as one bit into the dword (set or cleared); the bits read.', '''
int32_t message_delta_flags_decode''' + SIG + '''
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint8_t *mask = (uint8_t *)(descriptor + 1);
    uint32_t value = *(uint32_t *)current;
    int32_t total = 0;
    int32_t i;

    (void)previous;
    for (i = 0; i < descriptor[0]; i++) {
        if (mask[i] == 1) {
            uint8_t bit = 0;

            total += (int32_t)bit_stream_read_bit(&bit, stream);
            if (bit) {
                value |= 1u << i;
            } else {
                value &= ~(1u << i);
            }
        }
    }
    *(uint32_t *)current = value;
    return total;
}
''')

# ---------------------------------------------------------------- kind 19: grenade counts (two 3-bit signed bytes)
e(0x4ea610, 67, 'message_delta_grenade_counts_encode', 'the two bytes as (a << 3) | b (sign-extended); unchanged: 0; else 6 bits.', '''
int32_t message_delta_grenade_counts_encode''' + SIG + '''
{
    int8_t *counts = (int8_t *)current;
    int32_t packed = ((int32_t)counts[0] << 3) | (int32_t)counts[1];

    (void)field_type;
    if (previous != 0 && packed == ((((int32_t)((int8_t *)previous)[0]) << 3) | (int32_t)((int8_t *)previous)[1])) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)&packed, 6);
}
''')
e(0x4ea660, 56, 'message_delta_grenade_counts_decode', '6 bits: the high 3 into byte 0, the low 3 into byte 1.', '''
int32_t message_delta_grenade_counts_decode''' + SIG + '''
{
    uint32_t packed = 0;
    int32_t bits = bit_stream_read_bits_chunked(6, &packed, stream);

    (void)field_type;
    (void)previous;
    ((uint8_t *)current)[0] = (uint8_t)(packed >> 3);
    ((uint8_t *)current)[1] = (uint8_t)(packed & 7);
    return bits;
}
''')

# ---------------------------------------------------------------- kind 20: unit real quantized {bits, levels}
e(0x4ea4d0, 10, 'message_delta_first_dword_compute_size', 'the descriptor  first dword (the flag count for kind 17, the bit width for kind 20).', '''
int32_t message_delta_first_dword_compute_size(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor;
}
''')
e(0x4ea4e0, 25, 'message_delta_quantized_real_initialize', 'positive bits and levels.', '''
uint8_t message_delta_quantized_real_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return descriptor[0] > 0 && descriptor[1] > 0;
}
''')
e(0x4ea500, 174, 'message_delta_quantized_real_encode', 'a real in [0,1] as floor(levels * value + 0.5) clamped to levels (unsigned); unchanged from the previous value quantized the same way (message_delta_quantize_float_to_int 0..1): 0; else the level in the descriptor  bits.', '''
extern double floor(double x);
extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum); // 0x4ea480

int32_t message_delta_quantized_real_encode''' + SIG + '''
{
    uint32_t *descriptor = (uint32_t *)field_type->array_descriptor;
    uint32_t levels = descriptor[1];
    uint32_t level = (uint32_t)(int64_t)floor((double)((real)levels * *(real *)current + 0.5f));

    if (level > levels) {
        level = levels;
    }
    if (previous != 0 && level == message_delta_quantize_float_to_int(descriptor[1], *(real *)previous, 0.0f, 1.0f)) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, &level, (int32_t)descriptor[0]);
}
''')
e(0x4ea5b0, 79, 'message_delta_quantized_real_decode', 'level / levels (both unsigned) from the descriptor  bits.', '''
int32_t message_delta_quantized_real_decode''' + SIG + '''
{
    uint32_t *descriptor = (uint32_t *)field_type->array_descriptor;
    uint32_t level = 0;
    int32_t bits = bit_stream_read_bits_chunked((int32_t)descriptor[0], &level, stream);

    (void)previous;
    *(real *)current = (real)((double)level / (double)descriptor[1]);
    return bits;
}
''')

# ---------------------------------------------------------------- hash table helpers the index kind needs
HT = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "objects.h"\n'
emit(0x4f0470, 70, 'hash_table_initialize', 'ESI table, EAX bucket count: once (not yet initialized): GlobalAlloc the buckets (8 bytes each, cleared), no entries, freelist or blocks; initialized.', '''
extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);

void hash_table_initialize(hash_table *table, int32_t bucket_count)
{
    int32_t i;

    if (table->initialized != 0) {
        return;
    }
    table->bucket_count = bucket_count;
    table->buckets = (hash_bucket *)GlobalAlloc(0, bucket_count * 8);
    for (i = 0; i < table->bucket_count; i++) {
        table->buckets[i].count = 0;
        table->buckets[i].first = 0;
    }
    table->entry_count = 0;
    table->freelist = 0;
    table->blocks = 0;
    table->initialized = 1;
}
''', cc='ESI -> table, EAX -> bucket_count', module='objects', header=HT)
emit(0x4f04c0, 104, 'hash_table_dispose', 'EDI table: when initialized, clears the buckets, frees every node block and its nodes (GlobalFree), then the buckets; everything zero.', '''
extern void *__stdcall GlobalFree(void *memory);

void hash_table_dispose(hash_table *table)
{
    hash_node_block *block;
    int32_t i;

    if (table->initialized != 1) {
        return;
    }
    for (i = 0; i < table->bucket_count; i++) {
        table->buckets[i].first = 0;
        table->buckets[i].count = 0;
    }
    table->freelist = 0;
    block = table->blocks;
    while (block != 0) {
        hash_node_block *next = block->next;

        GlobalFree(block->nodes);
        block->nodes = 0;
        GlobalFree(block);
        block = next;
    }
    table->blocks = 0;
    GlobalFree(table->buckets);
    table->buckets = 0;
    table->bucket_count = 0;
    table->entry_count = 0;
    table->initialized = 0;
}
''', cc='EDI -> table', module='objects', header=HT)
print('ok')
