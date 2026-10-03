#include "halo/memory/memory.hpp"

#include "tags.h"
#include <string.h>

/**
 * Walks a byte_swap_definition code table to swap one structure instance in place, or only to measure
 * it when data is zero. The total size is stored through out_size and the number of code entries
 * consumed through out_record_count, so callers can step over nested records.
 *
 * @address 0x4cfee0
 */
void byte_swap_definition::swap(int32_t data, int32_t *codes, int32_t *out_size, int32_t *out_record_count)
{
    int32_t *field;
    int32_t code;
    int32_t record_count;
    int32_t codes_consumed;
    int32_t offset;

    int32_t nested_size;
    int32_t nested_codes_consumed;
    uint32_t low;
    uint32_t high;
    int32_t field_data;

    record_count = codes[1];
    offset = 0;

    codes_consumed = 0;
    if (record_count < 1) {
        goto done;
    }

next_record:
    codes_consumed = 2;
    for (;;) {
        field = codes + codes_consumed;
        code = *field;
        switch (code) {
        case _byte_swap_definition_reference: {
            byte_swap_definition *referenced = (byte_swap_definition *)field[1];
            int32_t referenced_data = (data == 0) ? 0 : offset + data;
            referenced->swap(referenced_data, referenced->codes, &nested_size, 0);
            codes_consumed = codes_consumed + 2;
            offset = offset + nested_size;
            break;
        }
        case _byte_swap_end_struct:
            goto end_of_record;
        case _byte_swap_begin_struct: {
            int32_t nested_data = (data == 0) ? 0 : offset + data;
            this->swap(nested_data, field, &nested_size, &nested_codes_consumed);
            codes_consumed = codes_consumed + nested_codes_consumed;
            offset = offset + nested_size;
            break;
        }
        case _byte_swap_int64:
            if (data != 0) {
                low = *(uint32_t *)(offset + data);
                high = *(uint32_t *)(offset + 4 + data);
                *(uint32_t *)(offset + data) =
                    (high >> 0x10 | ((high & 0xff0000) >> 0x10 | high & 0xff00) << 0x10) >> 8 |
                    high << 0x18;
                *(uint32_t *)(offset + 4 + data) =
                    (low << 0x10 | ((low & 0xff00) << 0x10 | low & 0xff0000) >> 0x10) << 8 |
                    low >> 0x18;
            }
            codes_consumed = codes_consumed + 1;
            offset = offset + 8;
            break;
        case _byte_swap_int32:
            if (data != 0) {
                field_data = *(int32_t *)(offset + data);
                *(uint32_t *)(offset + data) =
                    ((uint32_t)field_data & 0xff0000 | (uint32_t)field_data >> 0x10) >> 8 |
                    ((uint32_t)field_data << 0x10 | (uint32_t)field_data & 0xff00) << 8;
            }
            codes_consumed = codes_consumed + 1;
            offset = offset + 4;
            break;
        case _byte_swap_int16:
            if (data != 0) {
                uint16_t *p = (uint16_t *)(offset + data);
                *p = (uint16_t)(((*p & 0xff) << 8) | ((*p >> 8) & 0xff));
            }
            codes_consumed = codes_consumed + 1;
            offset = offset + 2;
            break;
        default:
            if (0 < code) {
                codes_consumed = codes_consumed + 1;
                offset = offset + code;
            }
            break;
        }
    }

end_of_record:
    codes_consumed = codes_consumed + 1;
    record_count = record_count - 1;
    if (record_count == 0) {
        goto done;
    }
    goto next_record;

done:
    if (out_size != 0) {
        *out_size = offset;
    }
    if (out_record_count != 0) {
        *out_record_count = codes_consumed;
    }
}

/**
 * Computes and caches the encoded size of every field in the list starting at fields and stores the
 * total through out_size and the field count through out_field_count. Struct array fields recurse into
 * their nested list.
 *
 * A field that is skipped by the version test records the previous field's size, as the original does.
 *
 * @address 0x4d0d50
 */
void struct_definition::compute_size(int16_t *out_size, struct_definition_field *fields, int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    uint16_t total_size = 0;
    uint16_t field_size = (uint16_t)(uint32_t)fields;

    while (field->type != _struct_field_terminator) {
        if (field->minimum_version <= this->version &&
            (this->version <= field->maximum_version || field->maximum_version == 0)) {
            switch (field->type) {
            case _struct_field_unused:
            case _struct_field_data:
            case _struct_field_block:
                field_size = (uint16_t)field->count;
                break;
            case _struct_field_int16_array:
                field_size = (uint16_t)(field->count << 1);
                break;
            case _struct_field_int32_array:
                field_size = (uint16_t)(field->count << 2);
                break;
            case _struct_field_int64_array:
                field_size = (uint16_t)(field->count << 3);
                break;
            case _struct_field_string:
                field_size = (uint16_t)(field->count + 1);
                break;
            case _struct_field_variable_data:
                field_size = (uint16_t)(field->count + 2);
                break;
            case _struct_field_struct_array: {
                int16_t element_count = field->count;
                int16_t nested_size = 0;
                int16_t nested_field_count = 0;
                this->compute_size(&nested_size, field + 1, &nested_field_count);
                field = field + nested_field_count;
                field_size = (uint16_t)(element_count * nested_size + 2);
                break;
            }
            case _struct_field_terminator:
                field_size = 0;
                break;
            default:
                break;
            }
        }
        field->computed_size = (int16_t)field_size;
        field = field + 1;
        total_size = (uint16_t)(total_size + field_size);
    }

    if (out_field_count != 0) {
        int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
        *out_field_count = (int16_t)(distance / 10) + 1;
    }
    if (out_size != 0) {
        *out_size = (int16_t)total_size;
    }
}

/**
 * Decodes one structure instance of this definition from the byte stream input into dest_instance,
 * byte swapping big-endian values. Fields outside the wire version are zero filled and read nothing;
 * struct array fields recurse.
 *
 * The destination size and field count are reported through the two optional out parameters.
 *
 * @address 0x4d13c0
 */
void struct_definition::decode(byte_stream *input, int16_t version, void *dest_instance, int16_t *out_dest_size, struct_definition_field *fields, int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    uint8_t *dest_cursor = (uint8_t *)dest_instance;

    for (;;) {
        uint8_t *dest;

        if (field->type == _struct_field_terminator) {
            if (out_field_count != 0) {
                int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
                *out_field_count = (int16_t)(distance / 10) + 1;
            }
            if (out_dest_size != 0) {
                *out_dest_size = (int16_t)((int16_t)(int32_t)dest_cursor -
                    (int16_t)(int32_t)dest_instance);

            }
            return;
        }

        dest = dest_cursor;

        if (version < field->minimum_version ||
            (field->maximum_version < version && field->maximum_version != 0)) {

            memset(dest, 0, (uint32_t)(int32_t)field->computed_size);
        } else {
            switch (field->type) {
            case _struct_field_data:
            case _struct_field_block: {
                uint32_t byte_count = (uint32_t)(int32_t)field->count;
                int32_t new_cursor = input->cursor + (int32_t)byte_count;
                if (input->size < new_cursor || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    input->cursor = new_cursor;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_int16_array: {
                uint32_t byte_count = (uint32_t)field->count * 2;
                if (input->size < input->cursor + (int32_t)byte_count || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    uint16_t i;
                    for (i = 0; i < field->count; i = i + 1) {

                        uint16_t *slot = (uint16_t *)src + i;
                        *slot = (uint16_t)(((*slot & 0xff) << 8) | (*slot >> 8));
                    }
                    input->cursor += byte_count;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_int32_array: {
                uint32_t byte_count = (uint32_t)field->count * 4;
                if (input->size < input->cursor + (int32_t)byte_count || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    halo::memory::byte_swap_array(4, (uint32_t *)src, field->count);
                    input->cursor += byte_count;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_int64_array: {
                uint32_t byte_count = (uint32_t)field->count * 8;
                if (input->size < input->cursor + (int32_t)byte_count || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    halo::memory::byte_swap_array(8, (uint32_t *)src, field->count);
                    input->cursor += byte_count;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_string: {
                char *str = input->read_string();
                if (str != 0) {
                    strcpy((char *)dest, str);
                }
                break;
            }

            case _struct_field_variable_data: {
                uint16_t count = (uint16_t)input->read_ranged_integer(field->count);

                uint32_t byte_count = (uint32_t)(int32_t)(int16_t)count;
                int32_t new_cursor;
                *(uint16_t *)dest = count;
                new_cursor = input->cursor + (int32_t)byte_count;
                if (input->size < new_cursor || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    input->cursor = new_cursor;
                    if (src != 0) {
                        memcpy(dest + 2, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_struct_array: {
                int16_t count = (int16_t)input->read_ranged_integer(field->count);
                int16_t nested_field_count = 0;
                uint8_t *elem;
                this->compute_size(0, field + 1, &nested_field_count);
                if (count < 0 || field->count < count) {
                    count = 0;
                }
                *(uint16_t *)dest = (uint16_t)count;
                elem = dest + 2;
                if (0 < count) {
                    do {
                        int16_t nested_size = 0;
                        this->decode(input, version, elem, &nested_size, field + 1, 0);
                        elem = elem + nested_size;
                        count = count - 1;
                    } while (count != 0);
                }
                field = field + nested_field_count;

                break;
            }
            }
        }

        {
            int16_t field_size = field->computed_size;
            field = field + 1;
            dest_cursor = dest + field_size;
        }
    }
}

/**
 * Encodes one structure instance of this definition from source into the byte stream output, byte
 * swapping to big-endian. Fields outside the wire version still reserve their wire bytes; struct array
 * fields recurse.
 *
 * The source size and field count are reported through the two optional out parameters.
 *
 * @address 0x4d0e80
 */
void struct_definition::encode(byte_stream *output, int16_t version, void *source, int16_t *out_source_size, struct_definition_field *fields, int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    uint8_t *src_cursor = (uint8_t *)source;
    struct_definition_field *field_start;
    uint8_t *src_start;
    uint8_t *dest;
    uint32_t byte_count;
    uint16_t count;
    int16_t nested_field_count;
    int16_t nested_source_size;

    for (;;) {
        if (field->type == _struct_field_terminator) {
            if (out_field_count != 0) {
                int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
                *out_field_count = (int16_t)(distance / 10) + 1;

            }
            if (out_source_size != 0) {
                *out_source_size = (int16_t)((int16_t)(int32_t)src_cursor -
                    (int16_t)(int32_t)source);

            }
            return;
        }

        field_start = field;
        src_start = src_cursor;

        if (version < field->minimum_version ||
            (field->maximum_version < version && field->maximum_version != 0)) {

            switch (field->type) {
            case _struct_field_data:
            case _struct_field_int16_array:
            case _struct_field_int32_array:
            case _struct_field_int64_array:
            case _struct_field_block:
                byte_count = (uint32_t)(int32_t)field->count;

                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                memset(dest, 0, byte_count);
                output->cursor += byte_count;
                break;
            case _struct_field_string:
                if (output->size < output->cursor + 1 || output->overflow != 0) {
                    goto overflow;
                }
                output->data[output->cursor] = 0;
                output->cursor += 1;
                break;
            case _struct_field_variable_data:
            case _struct_field_struct_array:

                output->write_ranged_integer(field->count, 0);
                break;
            }
        } else {
            switch (field->type) {
            case _struct_field_data:
            case _struct_field_block:
                byte_count = (uint32_t)(int32_t)field->count;
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                if (src_cursor == 0) {
                    memset(dest, 0, byte_count);
                } else {
                    memcpy(dest, src_cursor, byte_count);
                }
                output->cursor += byte_count;
                break;

            case _struct_field_int16_array: {
                uint16_t i;
                byte_count = (uint32_t)field->count * 2;
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                if (src_cursor == 0) {
                    memset(dest, 0, byte_count);
                } else {
                    memcpy(dest, src_cursor, byte_count);
                }
                for (i = 0; i < field->count; i = i + 1) {
                    uint16_t *slot = (uint16_t *)dest + i;
                    *slot = (uint16_t)(((*slot & 0xff) << 8) | (*slot >> 8));
                }
                output->cursor += byte_count;
                break;
            }

            case _struct_field_int32_array:
                byte_count = (uint32_t)field->count * 4;
                if (output->size < (int32_t)(output->cursor + byte_count) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                if (src_cursor == 0) {
                    memset(dest, 0, byte_count);
                } else {
                    memcpy(dest, src_cursor, byte_count);
                }
                halo::memory::byte_swap_array(4, (uint32_t *)dest, field->count);
                output->cursor += byte_count;
                field = field_start;
                src_cursor = src_start;
                break;

            case _struct_field_int64_array:
                byte_count = (uint32_t)field->count * 8;
                if ((int32_t)(byte_count + output->cursor) <= output->size && output->overflow == 0) {
                    dest = output->data + output->cursor;
                    if (src_cursor == 0) {
                        memset(dest, 0, byte_count);
                    } else {
                        memcpy(dest, src_cursor, byte_count);
                    }
                    halo::memory::byte_swap_array(8, (uint32_t *)dest, field->count);
                    output->cursor += byte_count;
                    field = field_start;
                    src_cursor = src_start;
                    break;
                }
                goto overflow;

            case _struct_field_string:

                output->write_string((char *)src_cursor, field->count);
                break;

            case _struct_field_variable_data: {
                uint8_t *elements;
                count = *(uint16_t *)src_cursor;
                elements = src_cursor + 2;
                if ((int16_t)count < 0 || field->count < (int16_t)count) {
                    count = 0;
                }
                byte_count = count;

                output->write_ranged_integer(field->count, count);
                field = field_start;
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    output->overflow = 1;
                } else {
                    dest = output->data + output->cursor;
                    if (elements == 0) {

                        memset(dest, 0, byte_count);
                    } else {
                        memcpy(dest, elements, byte_count);
                    }
                    output->cursor += byte_count;
                }
                break;
            }

            case _struct_field_struct_array: {
                int16_t requested_count = *(int16_t *)src_cursor;
                src_cursor = src_cursor + 2;
                this->compute_size(0, field + 1, &nested_field_count);
                if (requested_count < 0 || field->count < requested_count) {
                    requested_count = 0;
                }

                output->write_ranged_integer(field->count, requested_count);
                if (0 < requested_count) {
                    int16_t remaining = requested_count;
                    do {
                        this->encode(output, version, src_cursor, &nested_source_size, field + 1, 0);
                        src_cursor = src_cursor + nested_source_size;
                        remaining = remaining - 1;
                    } while (remaining != 0);
                }
                field = field_start + nested_field_count;

                src_cursor = src_start;
                break;
            }
            }
        }

        goto advance;

    overflow:

        output->overflow = 1;

    advance:
        {
            int16_t field_size = field->computed_size;
            field = field + 1;
            src_cursor = src_cursor + field_size;
        }
    }
}

/**
 * Computes and caches the encoded size of every packet type of the group that has a struct definition
 * and has not been computed yet.
 *
 * @address 0x4d0980
 */
void data_packet_group::compute_sizes()
{
    int16_t i;
    int16_t discarded_field_count;

    int16_t discarded_size;

    for (i = 0; i < this->type_count; i = i + 1) {
        struct_definition *definition = this->types[i].definition;
        if (definition != 0 && definition->size_computed == 0) {
            definition->compute_size(&discarded_size, definition->fields, &discarded_field_count);
            definition->size_computed = 1;
        }
    }
}

/**
 * Writes header_byte, run through the packet header byte swap definition, at buffer[*cursor] and
 * advances the cursor. Fails with an error string in data_packet_group_error when the cursor would
 * pass the group's maximum encoded size.
 *
 * @address 0x4d0b60
 */
int32_t data_packet_group::append_packet_header(uint8_t *buffer, int16_t *cursor, uint8_t header_byte)
{
    uint8_t *dest = buffer + *cursor;

    if ((uint32_t)(*cursor + 1) < (uint32_t)this->maximum_encoded_size) {
        *dest = header_byte;
        packet_header_byte_swap_definition.swap((int32_t)dest, packet_header_byte_swap_definition.codes, 0, 0);
        *cursor = *cursor + 1;
        data_packet_group_error = 0;
        return 1;
    }
    data_packet_group_error = (char *)"couldn't append header to encoded packet";
    return 0;
}

/**
 * Decodes one packet from buffer. The header byte is the last byte of the packet; its type indexes the
 * group's packet types, whose class must match expected_class. The body is decoded from the start of
 * the buffer with the length reduced by the header.
 *
 * Returns 1 when no error string was set.
 *
 * @address 0x4d09d0
 */
int32_t data_packet_group::decode_packet(int16_t *remaining_length, void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class)
{
    uint8_t *header_byte;
    int8_t type;

    if ((uint16_t)*remaining_length < 1) {
        data_packet_group_error = (char *)"got packet with no header";
        return 0;
    }

    header_byte = buffer + (*remaining_length - 1);
    if (header_byte != 0) {
        packet_header_byte_swap_definition.swap((int32_t)header_byte, packet_header_byte_swap_definition.codes, 0, 0);
    }
    type = (int8_t)*header_byte;

    if (type < 0 || type >= this->type_count) {
        data_packet_group_error = (char *)"got packet with bad type";
        return 0;
    }
    {
        data_packet_type *entry = &this->types[(int)type];
        if (entry->packet_class != expected_class) {
            data_packet_group_error = (char *)"got packet with mismatched class";
            return 0;
        }
        *remaining_length = *remaining_length - 1;
        if (entry->definition != 0 &&
            entry->definition->decode_packet_body(buffer, *remaining_length, decoded_body, out_version_used, 0) == 0) {
            data_packet_group_error = (char *)"got packet which wouldn't decode";
            return 0;
        }
        *out_type = (int16_t)(int8_t)*header_byte;
        data_packet_group_error = 0;
        return 1;
    }
}

/**
 * Encodes one packet: the body through encode_packet_body of definition, then the header byte through
 * append_packet_header. Failures leave an error string in data_packet_group_error. Returns 1 on
 * success.
 *
 * @address 0x4d0ae0
 */
int32_t data_packet_group::encode_packet(int16_t version, struct_definition *definition, uint8_t *version_byte_dest, byte_stream *output, uint8_t *buffer, int16_t *cursor, void *source, int16_t *out_wrote_version_byte, uint8_t packet_type)
{
    char *error = 0;
    int32_t ok;

    ok = definition->encode_packet_body(version, version_byte_dest, output, source, out_wrote_version_byte, (int16_t)this->maximum_encoded_size);
    if (ok == 0) {
        error = (char *)"couldn't encode packet";
    } else {
        ok = this->append_packet_header(buffer, cursor, packet_type);
        if (ok == 0) {
            return data_packet_group_error == 0;
        }
    }
    data_packet_group_error = error;
    return error == 0;
}

/**
 * Decodes the body of a packet: wraps the buffer in a byte stream, reads the version byte when this
 * definition is versioned and decodes the fields when the version is not newer than the definition.
 * Reports the version used and the bytes consumed, and succeeds when the stream did not overflow.
 *
 * @address 0x4d0c70
 */
uint8_t struct_definition::decode_packet_body(uint8_t *buffer, int16_t remaining_length, void *dest, uint16_t *out_version_used, int16_t *out_bytes_consumed)
{
    byte_stream stream;
    uint16_t version = 0;
    uint8_t decoded = 0;

    if (this->size_computed == 0) {
        int16_t size, field_count;
        this->compute_size(&size, this->fields, &field_count);
        this->size_computed = 1;
    }

    stream.data = buffer;
    stream.cursor = 0;
    stream.size = (int32_t)remaining_length;
    stream.overflow = 0;

    if (this->version != 0) {
        if (stream.cursor + 1 > stream.size || stream.overflow != 0) {
            stream.overflow = 1;
            version = 0;
        } else {
            uint8_t *p = stream.data + stream.cursor;
            stream.cursor = stream.cursor + 1;
            version = (p != 0) ? *p : 0;
        }
    }

    if ((int16_t)version <= this->version) {
        this->decode(&stream, (int16_t)version, dest, 0, this->fields, 0);
        if (stream.overflow == 0) {
            decoded = 1;
        }
    }

    if (out_version_used != 0) {
        *out_version_used = version;
    }
    if (out_bytes_consumed != 0) {
        *out_bytes_consumed = (int16_t)stream.cursor;
    }
    return decoded;
}

/**
 * Encodes the body of a packet into output: computes the field sizes on first use, writes the version
 * byte when this definition is versioned and encodes the fields. The wrote-version flag is reported
 * through out_wrote_version_byte.
 *
 * @address 0x4d0bc0
 */
int32_t struct_definition::encode_packet_body(int16_t version, uint8_t *version_byte_dest, byte_stream *output, void *source, int16_t *out_wrote_version_byte, int16_t capacity_check)
{
    int32_t out_of_room = 0;
    int16_t wrote_version_byte = 0;

    if (this->size_computed == 0) {
        this->compute_size(0, this->fields, 0);
        this->size_computed = 1;
    }

    if (version == -1) {
        version = this->version;
    }
    if (0 < this->version) {
        if (capacity_check < 1) {
            out_of_room = 1;
        } else {
            *version_byte_dest = (uint8_t)version;
            wrote_version_byte = 1;
        }
    }

    this->encode(output, version, source, 0, this->fields, 0);

    *out_wrote_version_byte = wrote_version_byte;
    return !out_of_room;
}

namespace halo::memory {

/**
 * In-place byte-swaps `count` elements of size 8, 4, or 2 bytes (selected by `size_code`, one of
 * _byte_swap_int64/_byte_swap_int32/_byte_swap_int16) starting at `array`, for endian conversion. Any
 * other size_code is a no-op.
 *
 * @address 0x4cfd90
 */
void byte_swap_array(int32_t size_code, uint32_t *array, int32_t count)
{
    uint32_t low;
    uint32_t high;
    int32_t remaining;

    if (size_code == -8) {
        remaining = count;
        if (0 < count) {
            do {
                low = array[0];
                high = array[1];
                array[0] = (high >> 0x10 | ((high & 0xff0000) >> 0x10 | high & 0xff00) << 0x10) >> 8 |
                           high << 0x18;
                array[1] = (low << 0x10 | ((low & 0xff00) << 0x10 | low & 0xff0000) >> 0x10) << 8 |
                           low >> 0x18;
                array = array + 2;
                remaining = remaining - 1;
            } while (remaining != 0);
        }
    } else if (size_code == -4) {
        if (0 < count) {
            do {
                low = *array;
                *array = (low & 0xff0000 | low >> 0x10) >> 8 | (low << 0x10 | low & 0xff00) << 8;
                array = array + 1;
                count = count - 1;
            } while (count != 0);
        }
    } else if (size_code == -2 && 0 < count) {
        uint16_t *array16 = (uint16_t *)array;
        do {
            count = count - 1;
            *array16 = (uint16_t)(((*array16 & 0xff) << 8) | ((*array16 >> 8) & 0xff));
            array16 = array16 + 1;
        } while (count != 0);
    }
}

} // namespace halo::memory
