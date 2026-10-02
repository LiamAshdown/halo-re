// message_delta_flags_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ea2b0, size 256 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea2b0..0x4ea3af: every masked flag is written as one bit; when none differs
//   from the previous value (with one) the cursor rewinds to where it started and 0 comes back, else the bits
//   written.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_flags_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
