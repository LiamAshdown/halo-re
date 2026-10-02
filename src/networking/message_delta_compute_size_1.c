// message_delta_compute_size_1  (reached only through a .data code pointer; no C existed)
// address 0x4e8cb0, size 6 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8cb0..0x4e8cb5: 1 bits (the boolean kind).
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_compute_size_1(message_delta_field_type *field_type)
{
    (void)field_type;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
