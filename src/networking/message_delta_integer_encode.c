// message_delta_integer_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e8a40, size 272 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8a40..0x4e8b4f: unchanged from the previous value (compared at its width): 0;
//   else the value at the subtype width. Subtypes above 6: 0.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_integer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 8);
    case 1:
        if (previous != 0 && *(uint16_t *)previous == *(uint16_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 16);
    case 2:
        if (previous != 0 && *(uint32_t *)previous == *(uint32_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 32);
    case 3:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 1);
    case 4:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 3);
    case 5:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 5);
    case 6:
        if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
            return 0;
        }
        return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 6);
    }
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
