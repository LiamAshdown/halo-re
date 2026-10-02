// message_delta_structure_array_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e9130, size 509 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9130..0x4e932c: without a previous array every element is encoded in full.
//   Otherwise a block of changed flags (reserved bits) is skipped, each element is delta-encoded after it and its
//   flag written back (seeking absolutely from first_bit); the bits plus the flags, or -- nothing changed -- a rewind
//   to the flag block and 0.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_structure_array_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
