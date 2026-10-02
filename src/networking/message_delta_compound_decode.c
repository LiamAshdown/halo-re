// message_delta_compound_decode  (reached only through a .data code pointer; no C existed)
// address 0x4e97e0, size 486 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e97e0..0x4e99c5: without a previous record each binding decodes into
//   destination + its destination offset. Otherwise the flag block is skipped and per binding its flag is read
//   (seeking back to it) and, when set, the binding decodes from previous + source offset into destination +
//   destination offset; the bits plus the flags when any, else the (zero) bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_compound_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
