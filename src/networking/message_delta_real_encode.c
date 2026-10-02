// message_delta_real_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e8c70, size 64 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8c70..0x4e8caf: unchanged within +/-0.0001 of the previous value: 0; else its
//   32 bits.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
