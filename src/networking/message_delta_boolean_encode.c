// message_delta_boolean_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e8cc0, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8cc0..0x4e8cf3: an unchanged byte: 0; else the byte as one bit (1 when
//   written).
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_boolean_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint8_t value = *(uint8_t *)current;

    (void)field_type;
    if (previous != 0 && *(uint8_t *)previous == value) {
        return 0;
    }
    return bit_stream_write_bit(value, stream) ? 1 : 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
