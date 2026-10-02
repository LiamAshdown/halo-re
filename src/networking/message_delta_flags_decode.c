// message_delta_flags_decode  (reached only through a .data code pointer; no C existed)
// address 0x4ea3b0, size 116 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea3b0..0x4ea423: each masked flag read as one bit into the dword (set or
//   cleared); the bits read.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_flags_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
